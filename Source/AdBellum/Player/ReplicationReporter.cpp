// ReplicationReporter.cpp
// C++ file with function declarations/placeholders for ReplicationReporter.
// Implementations are intentionally omitted for now; this file will be filled
// in a follow-up change.

#include "Player/ReplicationReporter.h"
#include "Net/UnrealNetwork.h"
#include "System/AdBellumPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "System/AdBellumGameMode.h"
#include "Player/AdBellumPlayerController.h"


// NOTE: Implementations are intentionally left out. The following are the
// declarations/placeholders for the functions that will be implemented later.

AReplicationReporter::AReplicationReporter()
{
	// Make sure this actor replicates and is owned by a PlayerController on spawn
	bReplicates = true;
	SetReplicates(true);
}

void AReplicationReporter::Server_ReportReady_Implementation()
{
	// Inform GameMode that this player's reporter is ready to receive batches.
	APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
	if (!OwnerPC || !OwnerPC->PlayerState)
	{
		return;
	}
	FUniqueNetIdPtr UserId = OwnerPC->PlayerState->GetUniqueId().GetUniqueNetId();
	if (!UserId.IsValid())
	{
		return;
	}
	FString UserIdString = UserId->ToString();
	AAdBellumGameMode* GM = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnReporterReady(UserIdString);
	}
}

void AReplicationReporter::InitializeReporter(APlayerController* OwningController)
{
	// If this runs on the client it means the reporter actor has been
	// replicated to the owning client. Notify the server that this reporter
	// is now ready to receive batch notifications.
	if (!HasAuthority())
	{
		Server_ReportReady();
		return;
	}

	// Server-side initialization called right after spawn. We may store the
	// owning player's id for bookkeeping. Implemented minimally here.
	if (OwningController && OwningController->PlayerState)
	{
		// store an integer id if desired; leave as default for now
	}

	// A locally-controlled owner (the listen-server host, or the sole player
	// in Standalone) has no separate client-side proxy for their
	// PlayerController, so OnRep_ReplicationReporter - the only other path
	// that leads to Server_ReportReady() - will never fire for them, and
	// they would never be marked ready, permanently blocking
	// ReadyToStartMatch. There is no round trip to wait for in this case, so
	// report ready immediately.
	if (OwningController && OwningController->IsLocalController())
	{
		Server_ReportReady();
	}
}

void AReplicationReporter::SendReplicatedObjectsReport(const TArray<int32>& ObjectIds, int32 ChunkIndex, int32 TotalChunks)
{
	// Client-side caller: forward to the server RPC. The _Implementation
	// method below will be invoked on the server.
	if (!HasAuthority())
	{
		Server_ReportReplicatedObjects(ObjectIds, ChunkIndex, TotalChunks);
	}
	else
	{
		// If called on server, handle locally (could validate/process immediately)
		Server_ReportReplicatedObjects(ObjectIds, ChunkIndex, TotalChunks);
	}
}

void AReplicationReporter::ReportActorReplicated(AActor* ReplicatedActor, int32 BatchId)
{
	if (!ReplicatedActor)
	{
		return;
	}

	// Forward to the server via RPC. Calling a Server RPC on this actor is only
	// valid if this actor is owned by the calling client.
	// Record locally for the batch and send batch-complete when all expected actors arrived.
	if (BatchId >= 0)
	{
		TSet<TObjectPtr<AActor>>& Set = ReportedActorsByBatch.FindOrAdd(BatchId);
		Set.Add(ReplicatedActor);

		int32* Expected = ExpectedBatchCounts.Find(BatchId);
		if (Expected && Set.Num() >= *Expected)
		{
			// notify server that this client finished this batch
			if (!HasAuthority())
			{
				Server_ReportBatchComplete(BatchId);
			}
			else
			{
				Server_ReportBatchComplete(BatchId);
			}
		}
	}
}

void AReplicationReporter::Server_ReportReplicatedObjects_Implementation(const TArray<int32>& ObjectIds, int32 ChunkIndex, int32 TotalChunks)
{
	// Server-side: for now just forward each reported object id via game mode
	// if possible. The mapping from int id -> actor is project-specific and is
	// left to the game mode / game state. Here we simply log receipt.
#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Verbose, TEXT("Server_ReportReplicatedObjects received %d ids (chunk %d/%d)"), ObjectIds.Num(), ChunkIndex, TotalChunks);
#endif
	// TODO: map ids to actors and call NotifyActorReplicated for each
}

void AReplicationReporter::Server_ReportActorReplicated_Implementation(AActor* ReplicatedActor)
{
	// Per-actor reporting path removed; use batch-based reporting instead.
}

void AReplicationReporter::Client_ReceiveBatch_Implementation(int32 BatchId, const TArray<AActor*>& ActorsInBatch)
{
	ExpectedBatchCounts.Add(BatchId, ActorsInBatch.Num());
	// Optionally store actors if you want to validate which specific actors are expected

	// Some actors in this batch may have already individually reported
	// themselves via ReportActorReplicated before this notification arrived -
	// the common case for a locally-controlled owner (e.g. the listen-server
	// host), whose local polling in UNetworkComponent has no network round
	// trip to wait for and typically finishes well before the server's
	// batch timer flushes this notification out. ReportActorReplicated only
	// checks for completion when a NEW actor reports, so without checking
	// again here, a batch that was already fully (locally) reported before
	// this notification arrived would never be marked complete.
	TSet<TObjectPtr<AActor>>* AlreadyReported = ReportedActorsByBatch.Find(BatchId);
	int32 AlreadyReportedCount = AlreadyReported ? AlreadyReported->Num() : 0;
	if (AlreadyReportedCount >= ActorsInBatch.Num())
	{
		Server_ReportBatchComplete(BatchId);
	}
}

void AReplicationReporter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AReplicationReporter, OwnerPlayerId);
}

void AReplicationReporter::Server_ReportBatchComplete_Implementation(int32 BatchId)
{
	// Server-side: inform GameMode that this player completed a batch
	APlayerController* OwnerPC = Cast<APlayerController>(GetOwner());
	if (!OwnerPC || !OwnerPC->PlayerState)
	{
		return;
	}
	FUniqueNetIdPtr UserId = OwnerPC->PlayerState->GetUniqueId().GetUniqueNetId();
	if (!UserId.IsValid())
	{
		return;
	}
	FString UserIdString = UserId->ToString();
	AAdBellumGameMode* GM = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		GM->OnClientReportedBatchComplete(UserIdString, BatchId);
	}
}
