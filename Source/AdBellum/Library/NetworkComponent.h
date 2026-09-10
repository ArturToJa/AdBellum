// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "NetworkComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADBELLUM_API UNetworkComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UNetworkComponent();

	// Server-side setter for assigning the replication batch id for this component.
	void SetReplicationBatchId(int32 NewBatchId);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Polling helper for waiting until the per-client ReplicationReporter is
	// replicated to this client.
	void TryReportToReporter();

	FTimerHandle ReplicationReporterPollTimerHandle;
	int32 ReplicationReporterPollAttempts = 0;
	const int32 MaxReplicationReporterPollAttempts = 50; // ~5 seconds at 0.1s per poll

private:
	bool HasAuthority() const;

	UFUNCTION()
	void OnRep_ReplicationBatchId();
public:
	UPROPERTY(ReplicatedUsing=OnRep_ReplicationBatchId)
	int32 ReplicationBatchId = -1;


	bool bHasReportedLocal = false;
};
