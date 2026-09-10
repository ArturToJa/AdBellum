// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkComponent.h"
#include "Player/ReplicationReporter.h"
#include "Player/AdBellumPlayerController.h"
#include "System/AdBellumPlayerState.h"
#include "Kismet/GameplayStatics.h"

static void PollReplicationReporter(TWeakObjectPtr<UNetworkComponent> SelfPtr, int32 Attempts, int32 MaxAttempts)
{
	if (!SelfPtr.IsValid())
	{
		return;
	}

	UNetworkComponent* Self = SelfPtr.Get();
	// If we've already reported for this component, nothing to do.
	if (Self->bHasReportedLocal)
	{
		return;
	}
	UWorld* World = Self->GetWorld();
	if (!World)
	{
		return;
	}

	AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(World->GetFirstPlayerController());
	if (PC && PC->ReplicationReporterActor && Self->ReplicationBatchId >= 0)
	{
		PC->ReplicationReporterActor->ReportActorReplicated(Self->GetOwner(), Self->ReplicationBatchId);
		Self->bHasReportedLocal = true;
		return;
	}

	// Not ready yet - either the reporter hasn't arrived, or (notably for a
	// locally-controlled owner, e.g. the listen-server host) the batch id
	// hasn't been assigned by AAdBellumGameMode::AddActorToBatch yet, since
	// that runs after FinishSpawning/BeginPlay for units. A host has no
	// replicated proxy of its own actors, so there is no later OnRep to
	// retry from; this poll loop is the only chance, so it must keep
	// retrying on either condition, not just a missing reporter.
	Attempts++;
	if (Attempts >= MaxAttempts)
	{
		// Give up silently; we intentionally removed the legacy IIPlayer fallback.
		return;
	}

	FTimerDelegate Delegate = FTimerDelegate::CreateStatic(&PollReplicationReporter, SelfPtr, Attempts, MaxAttempts);
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, Delegate, 0.1f, false);
}

// Sets default values for this component's properties
UNetworkComponent::UNetworkComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// GetLifetimeReplicatedProps below replicates ReplicationBatchId, but a
	// UActorComponent only actually replicates its own properties once its
	// component-level bReplicates flag is enabled. Without this call,
	// ReplicationBatchId never reaches any client, OnRep_ReplicationBatchId
	// never fires, PollReplicationReporter's "ReplicationBatchId >= 0" check
	// never passes, and no client ever calls ReportActorReplicated - so no
	// replication batch can ever be acknowledged by anyone, for any actor.
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UNetworkComponent::BeginPlay()
{
	Super::BeginPlay();

	// Try immediate reporting; if the per-client ReplicationReporter hasn't
	// replicated to this client yet, start polling for a short while.
	// Use a static helper below to schedule short polls without requiring
	// header changes to the component class.
	PollReplicationReporter(TWeakObjectPtr<UNetworkComponent>(this), 0, 50);
}

void UNetworkComponent::OnRep_ReplicationBatchId()
{
	// When batch id arrives, attempt to report if not already reported
	if (bHasReportedLocal)
	{
		return;
	}

	// If we have a valid batch id, start immediate reporting attempts.
	if (ReplicationBatchId >= 0)
	{
		PollReplicationReporter(TWeakObjectPtr<UNetworkComponent>(this), 0, 50);
	}
}

void UNetworkComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNetworkComponent, ReplicationBatchId);
}

bool UNetworkComponent::HasAuthority() const
{
	AActor* Owner = GetOwner();
	check(Owner);
	return Owner->HasAuthority();
}

void UNetworkComponent::SetReplicationBatchId(int32 NewBatchId)
{
	if (!HasAuthority())
	{
		return;
	}
	ReplicationBatchId = NewBatchId;
}