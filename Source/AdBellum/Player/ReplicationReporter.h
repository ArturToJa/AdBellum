// ReplicationReporter.h
// A per-client actor that acts as a conduit for reporting which replicated objects
// the client has received. Header prepared with API and RPC declarations; implementations
// will be provided later.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReplicationReporter.generated.h"

class APlayerController;
class AActor;

UCLASS(Blueprintable)
class ADBELLUM_API AReplicationReporter : public AActor
{
	GENERATED_BODY()

public:
	// Default constructor (definition will be implemented later)
	AReplicationReporter();

	// Called on the server to initialize the reporter and associate it with the
	// owning PlayerController. Implementation deferred.
	UFUNCTION(BlueprintCallable, Category = "ReplicationReporter")
	void InitializeReporter(APlayerController* OwningController);

	// Client-facing call to send a batched/chunked report of replicated object IDs
	// to the server. Implementation deferred; this should call the Server RPC
	// internally when implemented.
	UFUNCTION(BlueprintCallable, Category = "ReplicationReporter")
	void SendReplicatedObjectsReport(const TArray<int32>& ObjectIds, int32 ChunkIndex = 0, int32 TotalChunks = 1);

	// Convenience API for reporting a single actor that has been replicated to
	// this client. This will invoke a Server RPC (declared below) when
	// implemented. Declaration only; implementation deferred.
	UFUNCTION(BlueprintCallable, Category = "ReplicationReporter")
	void ReportActorReplicated(AActor* ReplicatedActor, int32 BatchId);

	// Server RPC called by the client when the client has finished receiving
	// all actors in a batch and has reported them locally.
	UFUNCTION(Server, Reliable)
	void Server_ReportBatchComplete(int32 BatchId);

	// Server RPC called by the client to indicate the reporter actor has been
	// successfully replicated to the owning client and is ready to receive
	// batch notifications.
	UFUNCTION(Server, Reliable)
	void Server_ReportReady();

	// Client RPC invoked by the server to notify this client about a new
	// replication batch and the actors that belong to it.
	UFUNCTION(Client, Reliable)
	void Client_ReceiveBatch(int32 BatchId, const TArray<AActor*>& ActorsInBatch);

protected:
	// Server RPC that receives a batch of object IDs from the client. The
	// server-side implementation (Server_ReportReplicatedObjects_Implementation)
	// will be added later.
	UFUNCTION(Server, Reliable)
	void Server_ReportReplicatedObjects(const TArray<int32>& ObjectIds, int32 ChunkIndex = 0, int32 TotalChunks = 1);

	// Server RPC receiving a single replicated actor reference from the client.
	// Implementation deferred.
	UFUNCTION(Server, Reliable)
	void Server_ReportActorReplicated(AActor* ReplicatedActor);

	// Optional replicated info: server-side id of the owning player (for server
	// bookkeeping). Will be replicated from server to clients.
	UPROPERTY(Replicated)
	int32 OwnerPlayerId = -1;

	// How long (seconds) the server should wait for reports before timing out.
	UPROPERTY(EditDefaultsOnly, Category = "ReplicationReporter")
	float ReportTimeoutSeconds = 10.0f;

public:
	// Replication setup. Implementation deferred.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// Expected number of actors in a batch (client-side), set by Client_ReceiveBatch
	TMap<int32, int32> ExpectedBatchCounts;

	// Set of actors that have reported locally for a given batch
	TMap<int32, TSet<TObjectPtr<AActor>>> ReportedActorsByBatch;
};
