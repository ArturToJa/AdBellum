// Fill out your copyright notice in the Description page of Project Settings.


#include "AdBellumGameMode.h"
#include "Player/SpawnArea.h"
#include "Player/RandomSpawnArea.h"
#include "AdBellumGameState.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Interfaces/OwnershipInterface.h"
#include "Formation/BaseFormation.h"
#include "AdBellumPlayerState.h"
#include "Player/IPlayer.h"
#include "GameModes/TeamDeathmatchGameMode.h"

void AAdBellumGameMode::BeginPlay()
{}

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

TMap<FString, bool> AAdBellumGameMode::InitializePlayerIsReplicatedMap()
{
	TMap<FString, bool> IsReplicatedForPlayerMap;

	if (AAdBellumGameState* AdBellumGameState = GetGameState<AAdBellumGameState>())
	{
		for (APlayerState* PlayerState : AdBellumGameState->PlayerArray)
		{
			if (PlayerState)
			{
				FUniqueNetIdPtr UserId = PlayerState->GetUniqueId().GetUniqueNetId();
				if (UserId.IsValid())
				{
					#if !UE_SERVER
					if (UserId->ToString() != AdminNetId)
					{
					#endif
						IsReplicatedForPlayerMap.Add(UserId->ToString(), false);
					#if !UE_SERVER
					}
					#endif
				}
			}
		}
	}
	return IsReplicatedForPlayerMap;
}

void AAdBellumGameMode::OnNewPlayerArrived(FString PlayerNetId)
{
	for (TPair<AActor*, TMap<FString, bool>>& IsReplicatedMap : AllActorsMap)
	{
		IsReplicatedMap.Value.Add(PlayerNetId, false);
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
		SpawnFormationForPlayer(AIPlayer, TeamId, PlayerId, PlayerData.PlayerSquad.UnitPrefabDataArray, nullptr, true);
	}
}

void AAdBellumGameMode::SpawnFormationForPlayer(AActor* Player, int TeamId, int PlayerId, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea, bool bIsDefault)
{
	ABaseFormation* Formation = GetWorld()->SpawnActorDeferred<ABaseFormation>(DefaultFormationClass, FTransform(), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Formation->SetOwner(Player);
	Formation->FinishSpawning(FTransform());
	AllActorsMap.Add(Formation, InitializePlayerIsReplicatedMap());
	IOwnershipInterface::Execute_SetOwningPlayer(Formation, Player);
	IOwnershipInterface::Execute_SetTeamIndex(Formation, TeamId);
	SpawnUnitsForPlayer(Player, TeamId, PlayerId, Formation, UnitPrefabs, SpawnArea, bIsDefault);
}

void AAdBellumGameMode::SpawnUnitsForPlayer(AActor* Player, int TeamId, int PlayerId, ABaseFormation* Formation, TArray<FMeshCreatorPrefabStruct>& UnitPrefabs, ABaseSpawnArea* SpawnArea, bool bIsDefault)
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
						SpawnWeaponsForUnit(SpawnedUnit, UnitData.WeaponPrefabData, bIsDefault);
						if (bIsDefault)
						{
							AllUnits.Add(SpawnedUnit);
							AllUnitsPrefabs.Add(UnitData);
						}
						else
						{
							AllUnitsReady.Add(SpawnedUnit);
							AllUnitsPrefabsReady.Add(UnitData);
							ScheduleSpawnTimer();
						}
						IFormationInterface::Execute_AddUnitToFormation(Formation, SpawnedUnit);
						AllActorsMap.Add(SpawnedUnit, InitializePlayerIsReplicatedMap());
					}

				}
			}
		}
		Formation->SetFormationCost(TotalCost);
	}
}

void AAdBellumGameMode::SpawnWeaponsForUnit(AActor* Unit, TArray<FWeaponPrefabDataStruct> WeaponPrefabData, bool bIsDefault)
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
					if (bIsDefault)
					{
						AllWeapons.Add(SpawnedWeapon);
						AllWeaponsPrefabs.Add(UnitWeaponDataStruct);
					}
					else
					{
						AllWeaponsReady.Add(SpawnedWeapon);
						AllWeaponsPrefabsReady.Add(UnitWeaponDataStruct);
						ScheduleSpawnTimer();
					}
					AllActorsMap.Add(SpawnedWeapon, InitializePlayerIsReplicatedMap());
				}
				else
				{
					GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, "AAdBellumGameMode::SpawnWeaponsForUnit weapon not found in data table: " + WeaponPrefab.WeaponClass.ToString());
				}
			}
		}
	}
}

void AAdBellumGameMode::SetupUnits()
{
	if (AAdBellumGameState* AdBellumGameState = GetGameState<AAdBellumGameState>())
	{
		AdBellumGameState->SetUnitPrefab(AllUnitsReady, AllUnitsPrefabsReady);
		AdBellumGameState->OnWeaponCreated(AllWeaponsReady, AllWeaponsPrefabsReady);
		AllUnitsReady.Empty();
		AllUnitsPrefabsReady.Empty();
		AllWeaponsReady.Empty();
		AllWeaponsPrefabsReady.Empty();
		if (AllActorsMap.IsEmpty())
		{
			GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		}
	}
}

void AAdBellumGameMode::ScheduleSpawnTimer()
{
	if (!GetWorldTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AAdBellumGameMode::SetupUnits, 1.0f, false);
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
	for (const TPair<AActor*, TMap<FString, bool>>& UnitForPlayers : AllActorsMap)
	{
		for (const TPair<FString, bool>& IsReplicatedMap : UnitForPlayers.Value)
		{
			if (!IsReplicatedMap.Value)
			{
				return false;
			}
		}
	}
	return PendingConnections == ConnectedPlayers;
}

void AAdBellumGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	AllUnitsReady = AllUnits;
	AllUnitsPrefabsReady = AllUnitsPrefabs;
	AllWeaponsReady = AllWeapons;
	AllWeaponsPrefabsReady = AllWeaponsPrefabs;
	SetupUnits();
	InitializeGameMode();
	AutoPossessPlayers();
	bIsInitializing = false;
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
								IIPlayer::Execute_SpawnRTSCamera(NewPlayer, GetDefaultSpawnArea(TeamIndex)->GetPlayerSpawnPoint());
								SpawnFormationForPlayer(NewPlayer, TeamIndex, PlayerIndex, MultiplayerData.PlayerSquad.UnitPrefabDataArray, nullptr, true);
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
					IIPlayer::Execute_SpawnRTSCamera(NewPlayer, GetDefaultSpawnArea(0)->GetPlayerSpawnPoint());
					SpawnFormationForPlayer(NewPlayer, 0, PlayerIndex, DefaultPlayerSquads[0].PlayerSquadDataArray[PlayerIndex].UnitPrefabDataArray, nullptr, true);
				}
				else if (PlayersData[1].Num() < DefaultPlayerSquads[1].PlayerSquadDataArray.Num())
				{
					int PlayerIndex = AddPlayer(1, NewPlayer);
					IIPlayer::Execute_SpawnRTSCamera(NewPlayer, GetDefaultSpawnArea(1)->GetPlayerSpawnPoint());
					SpawnFormationForPlayer(NewPlayer, 1, PlayerIndex, DefaultPlayerSquads[1].PlayerSquadDataArray[PlayerIndex].UnitPrefabDataArray, nullptr, true);
				}
				else
				{
					int PlayerIndex = AddPlayer(0, NewPlayer);
					IIPlayer::Execute_SpawnRTSCamera(NewPlayer, GetDefaultSpawnArea(0)->GetPlayerSpawnPoint());
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
	if (AllActorsMap.Contains(ReplicatedActor) && AllActorsMap[ReplicatedActor].Contains(NetId))
	{
		AllActorsMap[ReplicatedActor][NetId] = true;
		if (!bIsInitializing)
		{
			bool bAllActorsReady = true;
			for (const TPair<FString, bool>& IsReplicatedMap : AllActorsMap[ReplicatedActor])
			{
				if (!IsReplicatedMap.Value)
				{
					bAllActorsReady = false;
					break;
				}
			}
			if (bAllActorsReady)
			{
				AllActorsMap.Remove(ReplicatedActor);
				//if (ReplicatedActor->GetClass()->ImplementsInterface(USelectable::StaticClass()))
				//{
				//	AllUnitsReady.Add(ReplicatedActor);
				//	AllUnitsPrefabsReady.Add(AllUnitsPrefabs[AllUnits.Find(ReplicatedActor)]);
				//}
				//else if(ReplicatedActor->GetClass()->ImplementsInterface(UIWeapon::StaticClass()))
				//{
				//	AllWeaponsReady.Add(ReplicatedActor);
				//	AllWeaponsPrefabsReady.Add(AllWeaponsPrefabs[AllWeapons.Find(ReplicatedActor)]);
				//}
				// setup the replicated actor
			}
		}
	}
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