// Fill out your copyright notice in the Description page of Project Settings.


#include "AdBellumGameMode.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Interfaces/OwnershipInterface.h"
#include "Formation/BaseFormation.h"
#include "GameModes/TeamDeathmatchGameMode.h"
#include "AdBellumGameState.h"
#include "AdBellumPlayerState.h"
#include "Player/SpawnArea.h"
#include "Player/RandomSpawnArea.h"
#include "Player/ReplicationReporter.h"
#include "Library/NetworkComponent.h"
#include "Player/AdBellumPlayerController.h"
#include "Player/IPlayer.h"
#include "Player/RTSPlayer.h"

void AAdBellumGameMode::BeginPlay()
{}

void AAdBellumGameMode::CreateRTSCameraForPlayer(APlayerController* PC, const FTransform& SpawnTransform)
{
	if (!PC || !PC->HasAuthority())
	{
		return;
	}

	// Use GameMode-configured camera class
	TSubclassOf<ARTSPlayer> CameraClass = RTSCameraClass;
	if (!CameraClass)
	{
		return;
	}

	AAdBellumPlayerController* AdPC = Cast<AAdBellumPlayerController>(PC);
	if (!AdPC)
	{
		return;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = PC;
	SpawnInfo.Instigator = PC->GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;

	ARTSPlayer* SpawnedCamera = GetWorld()->SpawnActor<ARTSPlayer>(CameraClass, SpawnTransform, SpawnInfo);
	if (!SpawnedCamera)
	{
		return;
	}

	// Ensure it is only relevant to its owner on the network
	SpawnedCamera->SetReplicates(true);
	SpawnedCamera->SetOwner(PC);
	SpawnedCamera->bOnlyRelevantToOwner = true;

	// Assign to controller and possess
	// Assign to controller and possess. If the concrete controller type is
	// AAdBellumPlayerController, keep its RTSCameraPawn reference in sync.
	

	AdPC->RTSCameraPawn = SpawnedCamera;
	//PC->Possess(SpawnedCamera);

	// OnRep_RTSPlayerSpawned (which binds the RTS camera's right-click order
	// delegate and spawns the RTS HUD) is only ever invoked by replication
	// landing on a remote proxy - it never fires for a locally-controlled
	// owner, since there is no separate proxy to replicate to. Without this,
	// the host would never get its RTS order delegate bound at all, even
	// though the client (a genuine remote proxy) works correctly.
	if (AdPC->IsLocalController())
	{
		AdPC->OnRep_RTSPlayerSpawned();
	}

	// Add camera to a per-player-only batch so only the owner will be expected to report it
	TArray<FString> ExpectedPlayers;
	if (PC->PlayerState)
	{
		FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
		if (Id.IsValid())
		{
			ExpectedPlayers.Add(Id->ToString());
		}
	}

	int32 BatchId = CreateOpenBatch(1, 0.1f, ExpectedPlayers, false);
	// Ensure the batch exists and add actor
	ReplicationBatches.FindOrAdd(BatchId).Actors.Add(SpawnedCamera);
	// assign to network component if present so clients also receive batch id
	if (UNetworkComponent* NetComp = SpawnedCamera->FindComponentByClass<UNetworkComponent>())
	{
		NetComp->SetReplicationBatchId(BatchId);
	}
	// Immediately flush this small per-player batch so the owner is told about it
	ForceFlushBatch(BatchId);

	// Schedule the same client timer behavior as before
	// Uses its own handle - AdPC->RTSSpawnTimerHandle is claimed by the
	// unrelated TryInitializeRTSHUD retry-poll and must not be shared,
	// or whichever timer is armed last silently blocks the other.
	FTransform TransformCopy = SpawnTransform;
	FTimerHandle CameraManagerTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(CameraManagerTimerHandle, [AdPC, TransformCopy]() {
		AdPC->Client_SetPlayerCameraManager(TransformCopy.GetRotation());
	}, 1.0f, false);

}

void AAdBellumGameMode::OnReporterReady(const FString& PlayerNetId)
{
	// Mark the player as having a ready reporter
	ReporterReadyPlayers.Add(PlayerNetId);

	// Any flushed batches that were waiting for this reporter should be sent
	// now. Iterate through flushed batches and resend the batch to this
	// reporter if the batch expected this player.
	for (const TPair<int32, FReplicationBatch>& Pair : ReplicationBatches)
	{
		int32 BatchId = Pair.Key;
		const FReplicationBatch& Batch = Pair.Value;
		if (!Batch.bFlushed || !Batch.ExpectedPlayers.Contains(PlayerNetId))
		{
			continue;
		}

		// find player controller for this NetId
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(It->Get());
			if (PC && PC->PlayerState)
			{
				FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
				if (Id.IsValid() && Id->ToString() == PlayerNetId)
				{
					// Rebuild the actor list for this batch and send
					if (PC->ReplicationReporterActor)
					{
						TArray<AActor*> ActorsToSend;
						for (const TWeakObjectPtr<AActor>& Weak : Batch.Actors)
						{
							if (AActor* A = Weak.Get())
							{
								ActorsToSend.Add(A);
							}
						}
						PC->ReplicationReporterActor->Client_ReceiveBatch(BatchId, ActorsToSend);
					}
					break;
				}
			}
		}
	}
	// If all connected players have reported their reporter ready, complete
	// the reporter-ready phase and finalize replication (flush any remaining
	// open batch that may contain actors created during initialization).
	int32 Connected = ConnectedPlayers;
	int32 ReadyCount = ReporterReadyPlayers.Num();
	if (!bReportersReadyPhaseComplete && Connected > 0 && ReadyCount >= Connected)
	{
		bReportersReadyPhaseComplete = true;
		// Now that all reporters are ready, flush any open batch and finalize
		// replication so ReadyToStartMatch will consider batches completed.
		if (ActiveOpenBatchId != 0)
		{
			ForceFlushBatch(ActiveOpenBatchId);
		}
		FinalizeReplication();
		bIsInitializing = false;
	}
}

void AAdBellumGameMode::OnPlayerReplicationFinished(const FString& PlayerNetId)
{
	// Per-player replication finished: do per-player continuation.
	UE_LOG(LogTemp, Log, TEXT("Player %s finished replication."), *PlayerNetId);
	// Find the player controller and call a continuation hook if needed.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(It->Get());
		if (PC && PC->PlayerState)
		{
			FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
			if (Id.IsValid() && Id->ToString() == PlayerNetId)
			{
				// Example: initialize HUD or enable player-specific gameplay.
				if (PC->RTSHUD == nullptr)
				{
					PC->SpawnRTSHud();
				}
				// If there is a per-player initialization that should now run,
				// call it here. This is intentionally minimal; expand as needed.
				break;
			}
		}
	}
}

int32 AAdBellumGameMode::CreateOpenBatch(int32 TargetSize, float TimeoutSeconds, const TArray<FString>& ExpectedPlayers, bool bMustWait)
{
	int32 BatchId = NextReplicationBatchId++;
	FReplicationBatch& Batch = ReplicationBatches.Add(BatchId);
	Batch.TargetSize = TargetSize;
	Batch.TimeoutSeconds = TimeoutSeconds;
	Batch.bMustWait = bMustWait;
	Batch.bIncludeFuturePlayers = ExpectedPlayers.Num() == 0;

	if (Batch.bIncludeFuturePlayers)
	{
		// include current players; future players will be added in OnNewPlayerArrived
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(It->Get());
			if (PC && PC->PlayerState)
			{
				FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
				if (Id.IsValid())
				{
					Batch.ExpectedPlayers.Add(Id->ToString());
				}
			}
		}
	}
	else
	{
		for (const FString& IdStr : ExpectedPlayers)
		{
			Batch.ExpectedPlayers.Add(IdStr);
		}
	}
	return BatchId;
}

int32 AAdBellumGameMode::AddActorToBatch(AActor* Actor)
{
	if (!Actor)
	{
		return 0;
	}

	if (ActiveOpenBatchId == 0)
	{
		ActiveOpenBatchId = CreateOpenBatch(DefaultBatchSize, DefaultBatchTimeout);
	}
	// NOTE: BatchId is read here, after ActiveOpenBatchId is guaranteed non-zero.
	// The previous version captured "BatchId = ActiveOpenBatchId" at the top of
	// the function, before a new batch could be created, and never refreshed
	// it - so the first actor added right after a batch closed was filed under
	// a stale id 0 with no target size or timeout, and was silently never
	// flushed or reported to any client.
	const int32 BatchId = ActiveOpenBatchId;

	FReplicationBatch& Batch = ReplicationBatches.FindOrAdd(BatchId);
	if (Batch.bFlushed)
	{
		return BatchId;
	}

	Batch.Actors.Add(Actor);

	// assign to network component so clients receive the batch id
	if (UNetworkComponent* NetComp = Actor->FindComponentByClass<UNetworkComponent>())
	{
		NetComp->SetReplicationBatchId(BatchId);
	}

	// If first actor, start timer
	if (Batch.Actors.Num() == 1)
	{
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &AAdBellumGameMode::OnBatchFlushTimer, BatchId);
		GetWorld()->GetTimerManager().SetTimer(Batch.TimerHandle, Delegate, Batch.TimeoutSeconds, false);
	}

	if (Batch.TargetSize > 0 && Batch.Actors.Num() >= Batch.TargetSize)
	{
		ForceFlushBatch(BatchId);
	}

	return BatchId;
}

void AAdBellumGameMode::AddUnitToBatch(AActor* Unit, const FMeshCreatorPrefabStruct& Prefab)
{
	int32 BatchId = AddActorToBatch(Unit);
	if (FReplicationBatch* Batch = ReplicationBatches.Find(BatchId))
	{
		Batch->UnitActors.Add(Unit);
		Batch->UnitPrefabsData.Add(Prefab);
	}
}

void AAdBellumGameMode::AddWeaponToBatch(AActor* Weapon, const FUnitWeaponDataStruct& Prefab)
{
	int32 BatchId = AddActorToBatch(Weapon);
	if (FReplicationBatch* Batch = ReplicationBatches.Find(BatchId))
	{
		Batch->WeaponActors.Add(Weapon);
		Batch->WeaponPrefabsData.Add(Prefab);
	}
}

void AAdBellumGameMode::ForceFlushBatch(int32 BatchId)
{
	FReplicationBatch* BatchPtr = ReplicationBatches.Find(BatchId);
	if (!BatchPtr || BatchPtr->bFlushed)
	{
		return;
	}
	FReplicationBatch& Batch = *BatchPtr;

	// Cancel timer
	GetWorld()->GetTimerManager().ClearTimer(Batch.TimerHandle);

	// Flush now
	TArray<AActor*> ActorsToSend;
	for (TWeakObjectPtr<AActor>& Weak : Batch.Actors)
	{
		if (AActor* A = Weak.Get())
		{
			ActorsToSend.Add(A);
		}
	}

	// Mark flushed
	Batch.bFlushed = true;

	// If this was the active open batch, clear it so next spawn will create a new one
	if (ActiveOpenBatchId == BatchId)
	{
		ActiveOpenBatchId = 0;
	}

	// initialize report tracking
	Batch.ReportsPerPlayer.Empty();

	// Phase 1: fully rebuild ExpectedPlayers from every currently connected
	// player BEFORE sending anything to anyone.
	//
	// This has to be a separate pass from sending. Sending Client_ReceiveBatch
	// to a locally-controlled recipient (the listen-server host) executes
	// synchronously in-process, which can cascade through
	// Client_ReceiveBatch_Implementation's "already locally reported" check
	// straight into Server_ReportBatchComplete -> OnClientReportedBatchComplete,
	// which removes this batch from ReplicationBatches the moment
	// ReportsPerPlayer.Num() >= ExpectedPlayers.Num(). If sending happened
	// interleaved with building ExpectedPlayers, the host (processed first)
	// could satisfy that condition using only ITS OWN entry - before any
	// other player had been added - erasing the batch out from under this
	// loop and turning every subsequent Batch.ExpectedPlayers.Add() for the
	// remaining players into a write through a dangling reference. Building
	// the complete set first, then sending in a second pass that only reads
	// from a local snapshot, avoids that reentrancy hazard entirely.
	Batch.ExpectedPlayers.Empty();
	TArray<AAdBellumPlayerController*> RecipientCandidates;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(It->Get());
		if (!PC)
		{
			continue;
		}
		RecipientCandidates.Add(PC);

		if (PC->PlayerState)
		{
			FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
			if (Id.IsValid())
			{
				Batch.ExpectedPlayers.Add(Id->ToString());
			}
		}
	}

	// Phase 2: ExpectedPlayers is now complete and won't be touched again for
	// this batch, so it's safe to start sending - even though a send below
	// may cause the batch to be completed and removed from ReplicationBatches
	// partway through this loop. Only send the notification if the reporter
	// is known to be ready on that client; if not, keep the batch flushed but
	// skip the notification for now (OnReporterReady will resend it once that
	// client's reporter signals ready).
	//
	// NOTE: Batch/BatchPtr must not be touched anywhere below this point -
	// only the local snapshots captured above (ActorsToSend, RecipientCandidates,
	// BatchId) and GameMode members (ReporterReadyPlayers) are used from here on.
	for (AAdBellumPlayerController* PC : RecipientCandidates)
	{
		if (!PC->ReplicationReporterActor)
		{
			continue;
		}
		FUniqueNetIdPtr Id = PC->PlayerState ? PC->PlayerState->GetUniqueId().GetUniqueNetId() : nullptr;
		if (Id.IsValid() && ReporterReadyPlayers.Contains(Id->ToString()))
		{
			PC->ReplicationReporterActor->Client_ReceiveBatch(BatchId, ActorsToSend);
		}
	}
}

void AAdBellumGameMode::OnBatchFlushTimer(int32 BatchId)
{
	// Timer triggered; flush whatever we have
	ForceFlushBatch(BatchId);
}

void AAdBellumGameMode::OnClientReportedBatchComplete(const FString& PlayerNetId, int32 BatchId)
{
	FReplicationBatch* BatchPtr = ReplicationBatches.Find(BatchId);
	if (!BatchPtr)
	{
		return;
	}

	BatchPtr->ReportsPerPlayer.Add(PlayerNetId);

	// This player has now actually received every actor in this batch -
	// only now is it safe to tell them how to configure any units/weapons
	// it contained, instead of racing ahead of replication.
	SendBatchConfigurationToPlayer(*BatchPtr, PlayerNetId);

	if (BatchPtr->ReportsPerPlayer.Num() >= BatchPtr->ExpectedPlayers.Num())
	{
		// A bIncludeFuturePlayers batch (the common case: it was created
		// before every expected player had connected) can still gain MORE
		// expected players later, via OnNewPlayerArrived - which only works
		// by finding this batch in ReplicationBatches. If we removed it here
		// just because everyone connected SO FAR has reported it, it would
		// vanish before a not-yet-connected player ever had a chance to be
		// added, be sent Client_ReceiveBatch, or receive
		// SendBatchConfigurationToPlayer for the actors in it - which is
		// exactly why a host's own squad (flushed and "complete" the instant
		// the host itself reports, since no one else had connected yet) was
		// never being configured for a client who connected afterward. Only
		// actually retire the batch once we know no more players are still
		// expected to connect.
		bool bMoreConnectionsExpected = BatchPtr->bIncludeFuturePlayers && ConnectedPlayers < PendingConnections;
		if (!bMoreConnectionsExpected)
		{
			UE_LOG(LogTemp, Log, TEXT("Replication batch %d fully reported by all players"), BatchId);
			GetWorld()->GetTimerManager().ClearTimer(BatchPtr->TimerHandle);
			// Clean up: a single Remove() now retires everything about this batch
			// (target size, timeout, timer, flushed flag, expected/reported
			// players) in one step, instead of the nine separate map removals
			// this used to require - which had already drifted: the old code
			// never removed the batch's entry from BatchMustWait, so a stale
			// bool accumulated there for every completed batch for the lifetime
			// of the GameMode.
			ReplicationBatches.Remove(BatchId);
			BatchPtr = nullptr;
		}
	}

	// If this batch was marked as must-wait, and the reporting player has
	// now reported all batches they were expected to, we can mark that
	// individual player as finished and allow per-player initialization to
	// proceed without waiting for other players.
	// Check whether this player has any remaining pending must-wait batches.
	bool bHasPending = false;
	for (const TPair<int32, FReplicationBatch>& Pair : ReplicationBatches)
	{
		const FReplicationBatch& Batch = Pair.Value;
		if (!Batch.bMustWait)
			continue;

		if (!Batch.ExpectedPlayers.Contains(PlayerNetId))
			continue;

		// If this player hasn't been counted in the reports for this batch,
		// that means there is still pending work for them.
		if (!Batch.ReportsPerPlayer.Contains(PlayerNetId) || Batch.ReportsPerPlayer.Num() < Batch.ExpectedPlayers.Num())
		{
			bHasPending = true;
			break;
		}
	}

	if (!bHasPending)
	{
		PlayersFinishedReplication.Add(PlayerNetId);
		OnPlayerReplicationFinished(PlayerNetId);
	}
}

AAdBellumPlayerController* AAdBellumGameMode::FindPlayerControllerByNetId(const FString& PlayerNetId) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(It->Get());
		if (PC && PC->PlayerState)
		{
			FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
			if (Id.IsValid() && Id->ToString() == PlayerNetId)
			{
				return PC;
			}
		}
	}
	return nullptr;
}

void AAdBellumGameMode::SendBatchConfigurationToPlayer(const FReplicationBatch& Batch, const FString& PlayerNetId)
{
	AAdBellumPlayerController* PC = FindPlayerControllerByNetId(PlayerNetId);
	if (!PC)
	{
		return;
	}

	if (!Batch.UnitActors.IsEmpty())
	{
		TArray<AActor*> Units;
		Units.Reserve(Batch.UnitActors.Num());
		for (const TWeakObjectPtr<AActor>& Weak : Batch.UnitActors)
		{
			Units.Add(Weak.Get());
		}
		PC->Client_SetUnitPrefab(Units, Batch.UnitPrefabsData);
	}

	if (!Batch.WeaponActors.IsEmpty())
	{
		TArray<AActor*> Weapons;
		Weapons.Reserve(Batch.WeaponActors.Num());
		for (const TWeakObjectPtr<AActor>& Weak : Batch.WeaponActors)
		{
			Weapons.Add(Weak.Get());
		}
		PC->Client_OnWeaponCreated(Weapons, Batch.WeaponPrefabsData);
	}
}

void AAdBellumGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	ConnectedPlayers = 0;
	InitializeSpawnAreas();

	PlayersData.SetNum(2);
	if (UAdBellumGameInstance* GameInstance = GetGameInstance<UAdBellumGameInstance>())
	{
		bIsInitializing = true;
		GameInstance->SetIsStartingNewMap(false);
		MultiplayerTeams = GameInstance->GetTeams();
		GameModeType = GameInstance->GetGameMode();
		InitialTickets = GameInstance->GetTickets();
		CountPendingConnections();
		bTraveledFromLobby = !MultiplayerTeams.IsEmpty();
		if (bTraveledFromLobby)
		{
			for (int TeamIndex = 0; TeamIndex < MultiplayerTeams.Num(); ++TeamIndex)
			{
				TArray<FMultiplayerDataStruct>& MultiplayerTeam = MultiplayerTeams[TeamIndex];
				for(int PlayerIndex = 0; PlayerIndex < MultiplayerTeam.Num(); ++PlayerIndex)
				{
					FMultiplayerDataStruct& MultiplayerData = MultiplayerTeam[PlayerIndex];
					SpawnAIPlayer(TeamIndex, PlayerIndex, MultiplayerData);
				}
			}
		}
		else
		{
			PendingConnections = NetworkPlayers;
			if (bShouldSpawnDefaultAIPlayer && !DefaultPlayerSquads[1].PlayerSquadDataArray.IsEmpty())
			{
				FPlayerSquadDataStruct& PlayerSquadData = DefaultPlayerSquads[1].PlayerSquadDataArray[0];
				FMultiplayerDataStruct MultiplayerData;
				MultiplayerData.IsAI = true;
				MultiplayerData.PlayerName = "Easy AI";
				MultiplayerData.PlayerSquad = PlayerSquadData;
				SpawnAIPlayer(1, 0, MultiplayerData);
			}
		}
	}
}

void AAdBellumGameMode::InitializeSpawnAreas()
{
	SpawnAreas.SetNum(2);

	for (TActorIterator<ABaseSpawnArea> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		ABaseSpawnArea* BaseSpawnArea = *Iterator;
		if (ASpawnArea* SpawnArea = Cast<ASpawnArea>(BaseSpawnArea))
		{
			if (SpawnArea)
			{
				SpawnAreas[SpawnArea->TeamId].Emplace(SpawnArea);
			}
		}
		else if (ARandomSpawnArea* RandomSpawnArea = Cast<ARandomSpawnArea>(BaseSpawnArea))
		{
			if (SpawnAreas[0].Num() == SpawnAreas[1].Num())
			{
				SpawnAreas[0].Emplace(RandomSpawnArea);
			}
			else
			{
				SpawnAreas[1].Emplace(RandomSpawnArea);
			}
		}
	}
#if !UE_BUILD_SHIPPING
	if (SpawnAreas[0].IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, "No spawn areas found for team 0");
	}
	if (SpawnAreas[1].IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, "No spawn areas found for team 1");
	}
#endif
}




void AAdBellumGameMode::OnNewPlayerArrived(FString PlayerNetId)
{


	// Also send any already-flushed batches to the new player so they can
	// report completion for those batches as well.
	APlayerController* TargetPC = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC->PlayerState)
		{
			FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
			if (Id.IsValid() && Id->ToString() == PlayerNetId)
			{
				TargetPC = PC;
				break;
			}
		}
	}

	if (!TargetPC)
	{
		return;
	}

	AAdBellumPlayerController* NewPC = Cast<AAdBellumPlayerController>(TargetPC);
	if (!NewPC || !NewPC->ReplicationReporterActor)
	{
		return;
	}

	for (TPair<int32, FReplicationBatch>& Pair : ReplicationBatches)
	{
		int32 BatchId = Pair.Key;
		FReplicationBatch& Batch = Pair.Value;
		if (!Batch.bFlushed)
		{
			continue;
		}

		// Only send this flushed batch to the new player if the batch was
		// created as global (include future players) or it explicitly
		// included this player in the expected set.
		bool bShouldSend = false;
		if (Batch.bIncludeFuturePlayers)
		{
			bShouldSend = true;
			// also add this new player to expected set so they must report
			Batch.ExpectedPlayers.Add(PlayerNetId);
		}
		else if (Batch.ExpectedPlayers.Contains(PlayerNetId))
		{
			bShouldSend = true;
		}

		if (!bShouldSend)
		{
			continue;
		}

		// send the batch contents to the new player's reporter
		TArray<AActor*> ActorsToSend;
		for (const TWeakObjectPtr<AActor>& Weak : Batch.Actors)
		{
			if (AActor* A = Weak.Get())
			{
				ActorsToSend.Add(A);
			}
		}
		NewPC->ReplicationReporterActor->Client_ReceiveBatch(BatchId, ActorsToSend);
	}
}

void AAdBellumGameMode::SpawnAIPlayer(int TeamId, int PlayerId, FMultiplayerDataStruct& PlayerData)
{
	if (PlayerData.IsAI)
	{
		FActorSpawnParameters SpawnInfoRTSCamera;
		FTransform AIPlayerSpawnTransform = GetDefaultSpawnArea(TeamId)->GetPlayerSpawnPoint();

		AActor* AIPlayer = GetWorld()->SpawnActor<AActor>(AIPlayerClass, AIPlayerSpawnTransform, SpawnInfoRTSCamera);
		const TArray<FMeshCreatorPrefabStruct>& UnitsData = PlayerData.PlayerSquad.UnitPrefabDataArray;
		AddPlayer(TeamId, AIPlayer);
		AIPlayers.Emplace(AIPlayer);
		SpawnFormationForPlayer(AIPlayer, TeamId, PlayerId, PlayerData.PlayerSquad.UnitPrefabDataArray, nullptr);
	}
}

void AAdBellumGameMode::SpawnFormationForPlayer(AActor* Player, int TeamId, int PlayerId, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea)
{
	ABaseFormation* Formation = GetWorld()->SpawnActorDeferred<ABaseFormation>(DefaultFormationClass, FTransform(), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Formation->SetOwner(Player);
	Formation->FinishSpawning(FTransform());
	// Batch system handles replication tracking now.
	AddActorToBatch(Formation);
	IOwnershipInterface::Execute_SetOwningPlayer(Formation, Player);
	IOwnershipInterface::Execute_SetTeamIndex(Formation, TeamId);
	SpawnUnitsForPlayer(Player, TeamId, PlayerId, Formation, UnitPrefabs, SpawnArea);
	Formation->FinalizeFormation();

	TArray<ABaseFormation*> CreatedFormations;
	CreatedFormations.Add(Formation);
	if (Player->GetClass()->ImplementsInterface(UIPlayer::StaticClass()))
	{
		IIPlayer::Execute_SetOwnedFormations(Player, CreatedFormations);
	}
}

void AAdBellumGameMode::SpawnUnitsForPlayer(AActor* Player, int TeamId, int PlayerId, ABaseFormation* Formation, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea)
{
	if (!UnitPrefabs.IsEmpty())
	{
		if (SpawnArea == nullptr)
		{
			SpawnArea = SpawnAreas[TeamId][0];
		}

		SpawnArea->GenerateTransforms(UnitPrefabs.Num() * UnitSpawnMultiplier);

		int TotalCost = 0;
		for (int i = 0; i < UnitSpawnMultiplier; ++i)
		{
			for (const FMeshCreatorPrefabStruct& UnitData : UnitPrefabs)
			{
				TotalCost += UnitData.TicketCost;
			}
		}
		
		if (IPlayerStateInterface::Execute_TryConsumeTickets(IIPlayer::Execute_GetPlayerStateActor(Player), TotalCost))
		{
			for (int i = 0; i < UnitSpawnMultiplier; ++i)
			{
				for (const FMeshCreatorPrefabStruct& UnitData : UnitPrefabs)
				{
					TSubclassOf<APawn> Class = UnitData.UnitClass.LoadSynchronous();
					FTransform SpawnTransform = SpawnArea->GetNextTransform();
					APawn* SpawnedUnit = GetWorld()->SpawnActorDeferred<APawn>(Class, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
					if (SpawnedUnit)
					{
						SpawnedUnit->SetOwner(Player);
						SpawnedUnit->FinishSpawning(SpawnTransform);
						AddUnitForPlayer(SpawnedUnit, Player, UnitData);

						// Records this unit's prefab data against whichever
						// batch it lands in, so it gets configured on each
						// client only once that client actually reports
						// having received it (see OnClientReportedBatchComplete).
						AddUnitToBatch(SpawnedUnit, UnitData);

						SpawnWeaponsForUnit(SpawnedUnit, UnitData.WeaponPrefabData);
						IFormationInterface::Execute_AddUnitToFormation(Formation, SpawnedUnit);
					}

				}
			}
		}
		Formation->SetFormationCost(TotalCost);
	}
}

void AAdBellumGameMode::SpawnWeaponsForUnit(AActor* Unit, TArray<FWeaponPrefabDataStruct> WeaponPrefabData)
{
	if (!WeaponPrefabData.IsEmpty())
	{
		for (const FWeaponPrefabDataStruct& WeaponPrefab : WeaponPrefabData)
		{
			WeaponData.RowName = WeaponPrefab.WeaponClass;
			FWeaponPrefabData* WeaponPrefabStruct = WeaponData.GetRow<FWeaponPrefabData>("Spawning Weapon");
			if (WeaponPrefabStruct)
			{
				TSubclassOf<ABaseWeapon> WeaponClass = WeaponPrefabStruct->WeaponClass.LoadSynchronous();
				FActorSpawnParameters SpawnInfoWeapon;
				SpawnInfoWeapon.Owner = Unit;
				AActor* SpawnedWeapon = GetWorld()->SpawnActor<AActor>(WeaponClass, SpawnInfoWeapon);
				if (SpawnedWeapon)
				{
					FWeaponDataStruct WeaponDataStruct;
					WeaponDataStruct.WeaponSocketType = WeaponPrefab.WeaponSocketType;
					WeaponDataStruct.WeaponCustomizationData = WeaponPrefab.WeaponCustomizationData;
					FUnitWeaponDataStruct UnitWeaponDataStruct;
					UnitWeaponDataStruct.OwningUnit = Unit;
					UnitWeaponDataStruct.Weapon = WeaponDataStruct;
					// Records this weapon's prefab data against whichever
					// batch it lands in, so it gets configured on each
					// client only once that client actually reports having
					// received it (see OnClientReportedBatchComplete).
					AddWeaponToBatch(SpawnedWeapon, UnitWeaponDataStruct);
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, "AAdBellumGameMode::SpawnWeaponsForUnit weapon not found in data table: " + WeaponPrefab.WeaponClass.ToString());
				}
			}
		}
	}
}

void AAdBellumGameMode::CountPendingConnections()
{
	for (const TArray<FMultiplayerDataStruct>& Team : MultiplayerTeams)
	{
		for (const FMultiplayerDataStruct& Player : Team)
		{
			if (!Player.UniqueNetIdString.IsEmpty())
			{
				PendingConnections++;
			}
		}
	}
}

APlayerController* AAdBellumGameMode::SpawnPlayerController(ENetRole InRemoteRole, const FString& Options)
{
	// calling the deprecated functions for backward compatibility, should call SpawnPlayerControllerCommon directly in the future.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (Options.Contains(FString(TEXT("SpectatorOnly=1"))) && ReplaySpectatorPlayerControllerClass != nullptr)
	{
		return SpawnReplayPlayerController(InRemoteRole, FVector::ZeroVector, FRotator::ZeroRotator);
	}

	if (Options.Contains(FString(TEXT("Android"))))
	{
		return SpawnPlayerControllerCommon(InRemoteRole, FVector::ZeroVector, FRotator::ZeroRotator, VRPlayerControllerClass);
	}
	else if(Options.Contains(FString(TEXT("Windows"))))
	{
		return SpawnPlayerControllerCommon(InRemoteRole, FVector::ZeroVector, FRotator::ZeroRotator, PlayerControllerClass);
	}
	else
	{
#if defined(PLATFORM_WINDOWS)
		return SpawnPlayerControllerCommon(InRemoteRole, FVector::ZeroVector, FRotator::ZeroRotator, PlayerControllerClass);
#elif defined(PLATFORM_ANDROID)
		return SpawnPlayerControllerCommon(InRemoteRole, FVector::ZeroVector, FRotator::ZeroRotator, VRPlayerControllerClass);
#endif
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

bool AAdBellumGameMode::ReadyToStartMatch_Implementation()
{
	// Ensure all players have a ready reporter before considering batches.
	// If any player doesn't yet have a ready reporter, we're not ready.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = Cast<APlayerController>(It->Get());
		if (PC && PC->PlayerState)
		{
			FUniqueNetIdPtr Id = PC->PlayerState->GetUniqueId().GetUniqueNetId();
			if (Id.IsValid())
			{
				if (!ReporterReadyPlayers.Contains(Id->ToString()))
				{
					return false;
				}
			}
		}
	}

	// Check all batches that are marked as must-wait. For each such batch, it
	// must be flushed and all expected players must have reported completion.
	for (const TPair<int32, FReplicationBatch>& Pair : ReplicationBatches)
	{
		const FReplicationBatch& Batch = Pair.Value;
		if (!Batch.bMustWait)
		{
			continue;
		}

		if (!Batch.bFlushed)
		{
			return false;
		}

		if (Batch.ReportsPerPlayer.Num() < Batch.ExpectedPlayers.Num())
		{
			return false;
		}
	}

	return PendingConnections == ConnectedPlayers;
}

void AAdBellumGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	// Unit/weapon configuration is no longer sent from here: it is sent
	// per-player from OnClientReportedBatchComplete, the moment that specific
	// player confirms they've actually received the relevant batch - which,
	// for the initial squad, has already happened for every player by the
	// time ReadyToStartMatch allows us to get here.
	InitializeGameMode();
	AutoPossessPlayers();
	// We now enter the reporter-ready phase: wait for per-client reporters to
	// be replicated and the clients to acknowledge readiness. Only then will
	// we finalize replication (flush outstanding batches to reporters).
	bIsInitializing = true;
	bReportersReadyPhaseComplete = false;
	// FinalizeReplication will be called once all reporters report ready.
}

void AAdBellumGameMode::EndMatch()
{
	delete(GameMode);
}

void AAdBellumGameMode::TeamWins(int32 TeamIndex)
{
	for (int32 Index = 0; Index < PlayersData.Num(); ++Index)
	{
		if (Index != TeamIndex)
		{
			for (const FPlayerTeamData& PlayerData : PlayersData[Index])
			{
				IIPlayer::Execute_GameEnded(PlayerData.playerReference, MatchResult::Win);
			}
		}
		else
		{
			for (const FPlayerTeamData& PlayerData : PlayersData[Index])
			{
				IIPlayer::Execute_GameEnded(PlayerData.playerReference, MatchResult::Defeat);
			}
		}
	}
}

void AAdBellumGameMode::Draw()
{
	for (int32 Index = 0; Index < PlayersData.Num(); ++Index)
	{
		for (const FPlayerTeamData& PlayerData : PlayersData[Index])
		{
			IIPlayer::Execute_GameEnded(PlayerData.playerReference, MatchResult::Draw);
		}
	}
}

void AAdBellumGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	ConnectedPlayers++;
	if (APlayerState* PlayerState = (NewPlayer != NULL) ? ToRawPtr(NewPlayer->PlayerState) : NULL)
	{
		// Spawn a per-client ReplicationReporter actor on the server and store
		// a replicated pointer to it on the player's PlayerState so the client
		// will receive the reference and other client-side actors can discover
		// the reporter via their PlayerState.
		if (true)
		{
			// Prefer assigning the reporter to the PlayerController for fast
			// owner-only replication to the owning client.
			AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(NewPlayer);
			if (PC && PC->ReplicationReporterActor == nullptr)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.Owner = NewPlayer;
				SpawnParams.Instigator = NewPlayer->GetPawn();
				SpawnParams.ObjectFlags |= RF_Transient;
				AReplicationReporter* Reporter = GetWorld()->SpawnActor<AReplicationReporter>(AReplicationReporter::StaticClass(), FTransform::Identity, SpawnParams);
				if (Reporter)
				{
					Reporter->SetOwner(NewPlayer);
					PC->ReplicationReporterActor = Reporter;
				Reporter->InitializeReporter(NewPlayer);
				// Do NOT start sending batches until the client confirms the reporter
				// was replicated and is ready. The client will call Server_ReportReady
				// when it receives the reporter actor.
					// No-op patch: maintain exact context location for future patches.
				}
			}
		}
		FUniqueNetIdPtr UserId = PlayerState->GetUniqueId().GetUniqueNetId();
		if (UserId.IsValid())
		{
			FString UserIdString = UserId->ToString();
			if (ConnectedPlayers == 1)
			{
				AdminNetId = UserIdString;
			}
			else
			{
				OnNewPlayerArrived(UserIdString);
			}

			if (bTraveledFromLobby)
			{
				for (int TeamIndex = 0; TeamIndex < MultiplayerTeams.Num(); ++TeamIndex)
				{
					TArray<FMultiplayerDataStruct>& MultiplayerTeam = MultiplayerTeams[TeamIndex];
					if (!MultiplayerTeam.IsEmpty())
					{
						for (FMultiplayerDataStruct& MultiplayerData : MultiplayerTeam)
						{
							if (MultiplayerData.UniqueNetIdString == UserIdString)
							{
								int PlayerIndex = AddPlayer(TeamIndex, NewPlayer);
								CreateRTSCameraForPlayer(NewPlayer, GetDefaultSpawnArea(TeamIndex)->GetPlayerSpawnPoint());
								SpawnFormationForPlayer(NewPlayer, TeamIndex, PlayerIndex, MultiplayerData.PlayerSquad.UnitPrefabDataArray, nullptr);
								return;
							}
						}
					}
				}
			}
			else
			{
				// Default behavior for adding new player to the team
				//if (PlayersData[0].Num() <= PlayersData[1].Num())
				if (PlayersData[0].Num() < DefaultPlayerSquads[0].PlayerSquadDataArray.Num())
				{
					int PlayerIndex = AddPlayer(0, NewPlayer);
					CreateRTSCameraForPlayer(NewPlayer, GetDefaultSpawnArea(0)->GetPlayerSpawnPoint());
					SpawnFormationForPlayer(NewPlayer, 0, PlayerIndex, DefaultPlayerSquads[0].PlayerSquadDataArray[PlayerIndex].UnitPrefabDataArray, nullptr);
				}
				else if (PlayersData[1].Num() < DefaultPlayerSquads[1].PlayerSquadDataArray.Num())
				{
					int PlayerIndex = AddPlayer(1, NewPlayer);
					CreateRTSCameraForPlayer(NewPlayer, GetDefaultSpawnArea(1)->GetPlayerSpawnPoint());
					SpawnFormationForPlayer(NewPlayer, 1, PlayerIndex, DefaultPlayerSquads[1].PlayerSquadDataArray[PlayerIndex].UnitPrefabDataArray, nullptr);
				}
				else
				{
					int PlayerIndex = AddPlayer(0, NewPlayer);
					CreateRTSCameraForPlayer(NewPlayer, GetDefaultSpawnArea(0)->GetPlayerSpawnPoint());
				}
			}
		}
	}
}

void AAdBellumGameMode::Logout(AController* ExitingPlayer)
{

}

int AAdBellumGameMode::AddPlayer(int Team, AActor* Player)
{
	FPlayerTeamData NewPlayerData;
	NewPlayerData.playerNumber = PlayersData[Team].Num();
	NewPlayerData.playerReference = Player;
	int32 Index = PlayersData[Team].Add(NewPlayerData);
	IIPlayer::Execute_SetPlayerIndex(Player, Index);
	IIPlayer::Execute_SetTeamIndex(Player, Team);

	IPlayerStateInterface::Execute_SetInitialTickets(IIPlayer::Execute_GetPlayerStateActor(Player), InitialTickets);
	return NewPlayerData.playerNumber;
}

void AAdBellumGameMode::AddUnitForPlayer(AActor* Unit, AActor* Player, FMeshCreatorPrefabStruct UnitPrefab)
{
	int32 PlayerIndex = IIPlayer::Execute_GetPlayerIndex(Player);
	int32 TeamIndex = IIPlayer::Execute_GetTeamIndex(Player);
	FUnitDataStruct UnitData;
	UnitData.ownedUnit = Unit;
	UnitData.UnitPrefab = UnitPrefab;
	PlayersData[TeamIndex][PlayerIndex].ownedUnits.Add(UnitData);
	IOwnershipInterface::Execute_SetOwningPlayer(Unit, Player);
	IOwnershipInterface::Execute_SetTeamIndex(Unit, TeamIndex);
}

void AAdBellumGameMode::RemoveUnitForPlayer(AActor* Unit, AActor* Player)
{
	int32 PlayerIndex = IIPlayer::Execute_GetPlayerIndex(Player);
	int32 TeamIndex = IIPlayer::Execute_GetTeamIndex(Player);
	//FUnitDataStruct* FoundUnitData = PlayersData[TeamIndex][PlayerIndex].ownedUnits.FindByPredicate([Unit](const FUnitDataStruct& UnitData) { return UnitData.ownedUnit == Unit; });
	PlayersData[TeamIndex][PlayerIndex].ownedUnits.Remove({ FMeshCreatorPrefabStruct(), Unit});
	IOwnershipInterface::Execute_SetOwningPlayer(Unit, nullptr);
}

ABaseSpawnArea* AAdBellumGameMode::GetDefaultSpawnArea(int32 TeamIndex)
{
	int Priority = INT_MIN;
	ABaseSpawnArea* SelectedArea = nullptr;
	for (TArray<ABaseSpawnArea*> TeamSpawnAreas : SpawnAreas)
	{
		for (ABaseSpawnArea* SpawnArea : TeamSpawnAreas)
		{
			int TempPriority = SpawnArea->GetPriority(TeamIndex);
			if (Priority < TempPriority)
			{
				Priority = TempPriority;
				SelectedArea = SpawnArea;
			}
		}
	}
	return SelectedArea;
}

bool AAdBellumGameMode::IsEnemyUnit_Implementation(AActor* Unit, AActor* Player)
{
	if (Unit->GetClass()->ImplementsInterface(UOwnershipInterface::StaticClass()))
	{
		AActor* OwningPlayer = IOwnershipInterface::Execute_GetOwningPlayer(Unit);
		int32 PlayerTeamIndex = IIPlayer::Execute_GetTeamIndex(OwningPlayer);
		int32 EnemyTeamIndex = IIPlayer::Execute_GetTeamIndex(Player);
		return PlayerTeamIndex != EnemyTeamIndex;
	}
	else
	{
		return false;
	}
}

TArray<AActor*> AAdBellumGameMode::GetPlayerEnemyUnits_Implementation(AActor* PlayerActor)
{
	TArray<AActor*> EnemyUnits;
	if (PlayerActor->GetClass()->ImplementsInterface(UIPlayer::StaticClass()))
	{
		int32 PlayerIndex = IIPlayer::Execute_GetPlayerIndex(PlayerActor);
		int32 TeamIndex = IIPlayer::Execute_GetTeamIndex(PlayerActor);
		FPlayerTeamData& PlayerData = PlayersData[TeamIndex][PlayerIndex];
		for (int32 i = 0; i < PlayersData.Num(); ++i)
		{
			if (i == TeamIndex)
			{
				continue;
			}
			else
			{
				for (FPlayerTeamData& EnemyData : PlayersData[i])
				{
					for (const FUnitDataStruct& UnitData : EnemyData.ownedUnits)
					{
						EnemyUnits.Add(UnitData.ownedUnit);
					}
				}
			}
		}
	}
	else
	{
		for (int32 i = 0; i < PlayersData.Num(); ++i)
		{
			for (FPlayerTeamData& EnemyData : PlayersData[i])
			{
				for (const FUnitDataStruct& UnitData : EnemyData.ownedUnits)
				{
					EnemyUnits.Add(UnitData.ownedUnit);
				}
			}
		}
	}
	return EnemyUnits;
}

void AAdBellumGameMode::NotifyActorReplicated(FString NetId, AActor* ReplicatedActor)
{
	// Compatibility stub: per-actor replication tracking removed in favor of
	// batch-driven reporting. This function no longer performs bookkeeping.
	(void)NetId;
	(void)ReplicatedActor;
}

void AAdBellumGameMode::InitializeGameMode()
{
	switch (GameModeType)
	{
		case EGameMode::TeamDeathmatch:
		{
			TArray<uint8> Players;
			Players.SetNum(PlayersData.Num());
			for (int32 Index = 0; Index < PlayersData.Num(); ++Index)
			{
				Players[Index] = PlayersData[Index].Num();
			}
			TeamDeathmatchGameMode* TDGameMode = new TeamDeathmatchGameMode(Players);
			GameMode = TDGameMode;
			GameMode->DrawDelegate.BindUObject(this, &AAdBellumGameMode::Draw);
			GameMode->TeamWinsDelegate.BindUObject(this, &AAdBellumGameMode::TeamWins);

			for (const TArray< FPlayerTeamData> & TeamData : PlayersData)
			{
				for (const FPlayerTeamData& PlayerData : TeamData)
				{
					IPlayerStateInterface* PSInterface = Cast<IPlayerStateInterface>(IIPlayer::Execute_GetPlayerStateActor(PlayerData.playerReference));
					check(PSInterface);
					PSInterface->PlayerDiedDelegate.BindRaw(TDGameMode, &TeamDeathmatchGameMode::PlayerLost);
				}
			}
			break;
		}
	}
}

void AAdBellumGameMode::AutoPossessPlayers()
{
	for (TArray<FPlayerTeamData> PlayerDataArray : PlayersData)
	{
		for (const FPlayerTeamData& PlayerData : PlayerDataArray)
		{
			if (APlayerController* PC = Cast<APlayerController>(PlayerData.playerReference))
			{
				if (!PlayerData.ownedUnits.IsEmpty())
				{
					PC->Possess(Cast<APawn>(PlayerData.ownedUnits[0].ownedUnit));
				}
			}
		}
	}
}

void AAdBellumGameMode::FinalizeReplication()
{

}