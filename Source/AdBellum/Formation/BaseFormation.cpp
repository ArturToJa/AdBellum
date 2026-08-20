// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseFormation.h"
#include "Containers/ArrayView.h"
#include "OrderSystem/OrdersManager.h"
#include "OrderSystem/Orders/GeneralOrders.h"
#include "System/AdBellumGameState.h"
#include "Library/NetworkComponent.h"
#include "System/AdBellumPlayerState.h"
#include "Player/IPlayer.h"
#include "Player/BaseSpawnArea.h"
#include "Interfaces/ITargetable.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Player/RTS_HUD.h"

template void ABaseFormation::PerformOrder<OrderEnum::Attack>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::Enter>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::Follow>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::HoldPosition>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::Interact>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::Move>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::OccupyAOI>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::Patrol>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::Training>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::TakeCover>(AActor*, FVector);
template void ABaseFormation::PerformOrder<OrderEnum::Stop>(AActor*, FVector);

// Sets default values
ABaseFormation::ABaseFormation()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	OrdersManagerComponent = CreateDefaultSubobject<UOrdersManager>(TEXT("Order Manager Component"));
	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));
	BottleneckAssignmentComponent = CreateDefaultSubobject<UBottleneckAssignmentComponent>(TEXT("Bottleneck Assignment Component"));
	bReplicates = true;
	bAlwaysRelevant = true;
}

// Called when the game starts or when spawned
void ABaseFormation::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseFormation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (FirstActorOfInterest)
	{
		FindCovers(FirstActorOfInterest->GetActorLocation());
	}
}

void ABaseFormation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseFormation, TeamIndex);
	DOREPLIFETIME(ABaseFormation, OwningPlayer);
}

void ABaseFormation::ClearCurrentSelection()
{
	for (APawn* Unit : ActorsInFormation)
	{
		IFormable::Execute_SetFormation(Unit, nullptr);
	}
}

void ABaseFormation::SetCurrentSelection()
{
	for (APawn* Unit : ActorsInFormation)
	{
		IFormable::Execute_SetFormation(Unit, this);
	}
}

//Selectable
void ABaseFormation::SetSelection_Implementation(bool Visible)
{
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	for (APawn* Actor : ActorsInFormation)
	{
		GameState->SetSelectionCircle(Visible, Actor);
	}
}

class UOrdersManager* ABaseFormation::GetOrdersManagerComponent_Implementation()
{
	return OrdersManagerComponent;
}

AActor* ABaseFormation::GetOwningPlayer_Implementation()
{
	return OwningPlayer;
}

int32 ABaseFormation::GetTeamIndex_Implementation()
{
	return TeamIndex;
}

void ABaseFormation::SetOwningPlayer_Implementation(AActor* Player)
{
	OwningPlayer = Player;
}

void ABaseFormation::SetTeamIndex_Implementation(int32 Index)
{
	TeamIndex = Index;
}

//Orderable
void ABaseFormation::Stop_Implementation(FVector TargetPosition)
{
	for (APawn* Actor : ActorsInFormation)
	{
		TUniquePtr<GeneralOrder<OrderEnum::Stop>::OrderType> OrderToPerform = MakeUnique<GeneralOrder<OrderEnum::Stop>::OrderType>(nullptr, TargetPosition);
		IOrderable::Execute_GetOrdersManagerComponent(Actor->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
	}
}

void ABaseFormation::MoveOrder_Implementation(FVector TargetPosition)
{
	for (APawn* Actor : ActorsInFormation)
	{
		TUniquePtr<GeneralOrder<OrderEnum::Move>::OrderType> OrderToPerform = MakeUnique<GeneralOrder<OrderEnum::Move>::OrderType>(nullptr, TargetPosition);
		IOrderable::Execute_GetOrdersManagerComponent(Actor->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
	}
}

void ABaseFormation::AttackTarget_Implementation(UObject* TargetObject)
{
	for (APawn* Actor : ActorsInFormation)
	{
		TUniquePtr<GeneralOrder<OrderEnum::Attack>::OrderType> OrderToPerform = MakeUnique<GeneralOrder<OrderEnum::Attack>::OrderType>(TargetObject, FVector::ZeroVector);
		IOrderable::Execute_GetOrdersManagerComponent(Actor->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
	}
}

void ABaseFormation::AttackLocation_Implementation(FVector TargetPosition)
{

}

void ABaseFormation::SetActors(TArray<APawn*> Actors)
{
	ClearCurrentSelection();
	ActorsInFormation = Actors;
	SetCurrentSelection();
}

APawn* ABaseFormation::GetUnitForPossesion()
{
	if (ActorsInFormation.IsEmpty())
	{
		return nullptr;
	}
	else
	{
		for(APawn* Unit : ActorsInFormation)
		{
			if (Unit && IITargetable::Execute_IsAlive(Unit))
			{
				return Unit;
			}
		}
		return nullptr;
	}
}

template<OrderEnum T>
void ABaseFormation::PerformOrder(AActor* TargetUnit, FVector TargetPosition)
{
	for (APawn* Unit : ActorsInFormation)
	{
		if (IITargetable::Execute_IsAlive(Unit))
		{
			PerformOrder<T>(Unit, TargetUnit, TargetPosition);
		}
	}
}

template<OrderEnum T>
void ABaseFormation::PerformOrder(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<GeneralOrder<T>::OrderType> OrderToPerform = MakeUnique<GeneralOrder<T>::OrderType>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Attack>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<AttackUnitOrder> OrderToPerform = MakeUnique<AttackUnitOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Move>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<MoveLocationOrder> OrderToPerform = MakeUnique<MoveLocationOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Enter>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<EnterOrder> OrderToPerform = MakeUnique<EnterOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Follow>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<FollowUnitOrder> OrderToPerform = MakeUnique<FollowUnitOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::HoldPosition>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<HoldPositionOrder> OrderToPerform = MakeUnique<HoldPositionOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Interact>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<InteractOrder> OrderToPerform = MakeUnique<InteractOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::OccupyAOI>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<OccupyAOIOrder> OrderToPerform = MakeUnique<OccupyAOIOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Patrol>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<PatrolOrder> OrderToPerform = MakeUnique<PatrolOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Stop>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<StopOrder> OrderToPerform = MakeUnique<StopOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::TakeCover>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<TakeCoverOrder> OrderToPerform = MakeUnique<TakeCoverOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Training>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<TrainingOrder> OrderToPerform = MakeUnique<TrainingOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

template<>
void ABaseFormation::PerformOrder<OrderEnum::Reload>(APawn* UnitToOrder, AActor* TargetUnit, FVector TargetPosition)
{
	TUniquePtr<ReloadOrder> OrderToPerform = MakeUnique<ReloadOrder>(TargetUnit, TargetPosition);
	IOrderable::Execute_GetOrdersManagerComponent(UnitToOrder->GetController())->AddOrder(MoveTemp(OrderToPerform), false);
}

void ABaseFormation::OrderUnits(OrderEnum OrderType, AActor* TargetUnit, FVector TargetPosition)
{
	switch (OrderType)
	{
	case OrderEnum::Attack:
		PerformOrder<OrderEnum::Attack>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::Move:
		PerformOrder<OrderEnum::Move>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::Stop:
		PerformOrder<OrderEnum::Stop>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::HoldPosition:
		PerformOrder<OrderEnum::HoldPosition>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::Patrol:
		PerformOrder<OrderEnum::Patrol>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::Follow:
		PerformOrder<OrderEnum::Follow>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::Interact:
		PerformOrder<OrderEnum::Interact>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::Enter:
		PerformOrder<OrderEnum::Enter>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::TakeCover:
		PerformOrder<OrderEnum::TakeCover>(TargetUnit, TargetPosition);
		break;
	case OrderEnum::OccupyAOI:
		PerformOrder<OrderEnum::OccupyAOI>(TargetUnit, TargetPosition);
		break;
	}
}

void ABaseFormation::OrderUnit(APawn* UnitToOrder, OrderEnum OrderType, AActor* TargetUnit, FVector TargetPosition)
{
	switch (OrderType)
	{
	case OrderEnum::Attack:
		PerformOrder<OrderEnum::Attack>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::Move:
		PerformOrder<OrderEnum::Move>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::Stop:
		PerformOrder<OrderEnum::Stop>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::HoldPosition:
		PerformOrder<OrderEnum::HoldPosition>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::Patrol:
		PerformOrder<OrderEnum::Patrol>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::Follow:
		PerformOrder<OrderEnum::Follow>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::Interact:
		PerformOrder<OrderEnum::Interact>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::Enter:
		PerformOrder<OrderEnum::Enter>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::TakeCover:
		PerformOrder<OrderEnum::TakeCover>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	case OrderEnum::OccupyAOI:
		PerformOrder<OrderEnum::OccupyAOI>(UnitToOrder, TargetUnit, TargetPosition);
		break;
	}
}

void ABaseFormation::OnEnemyInSightChanged_Implementation(AActor* EnemyUnit, bool IsVisible)
{
	if (IsVisible)
	{
		if (EnemiesInSight.Contains(EnemyUnit))
		{
			EnemiesInSight[EnemyUnit]++;
		}
		else
		{
			EnemiesInSight.Add({ EnemyUnit, 1 });
			IITargetable::Execute_SetIsSeenBy(EnemyUnit, this, true);
			if (LastKnownPosition.Contains(EnemyUnit))
			{
				LastKnownPosition.Remove(EnemyUnit);
				ClearNoiseTimeoutTimer(EnemyUnit);
			}
		}

		if (FirstActorOfInterest == nullptr)
		{
			FirstActorOfInterest = EnemyUnit;
		}
	}
	else
	{
		EnemiesInSight[EnemyUnit]--;
		if (EnemiesInSight[EnemyUnit] == 0)
		{
			EnemiesInSight.Remove(EnemyUnit);
			LastKnownPosition.Add(EnemyUnit, EnemyUnit->GetActorLocation());
			SetNoiseTimeoutTimer(EnemyUnit);
			CheckFirstActorOfInterest(EnemyUnit);
		}
	}
}

void ABaseFormation::OnNoiseHeard_Implementation(AActor* NoiseSource)
{
	if (!EnemiesInSight.Contains(NoiseSource) && !LastKnownPosition.Contains(NoiseSource))
	{
		LastKnownPosition.Add({ NoiseSource, NoiseSource->GetActorLocation() });
		SetNoiseTimeoutTimer(NoiseSource);
		IITargetable::Execute_SetIsSeenBy(NoiseSource, this, true);
		if(FirstActorOfInterest == nullptr)
		{
			FirstActorOfInterest = NoiseSource;
		}
	}
}

int ABaseFormation::GetFormationCost_Implementation()
{
	return FormationCostInTickets;
}

void ABaseFormation::RespawnFormation_Implementation(ABaseSpawnArea* SpawnArea)
{
	AliveUnits = ActorsInFormation.Num();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Respawning formation %s, Units Alive: %d"), *GetName(), AliveUnits));
	SpawnArea->GenerateTransforms(ActorsInFormation.Num());

	for (APawn* Unit : ActorsInFormation)
	{
		FTransform SpawnTransform = SpawnArea->GetNextTransform();
		IFormable::Execute_RespawnUnit(Unit, SpawnTransform);
	}
}

AActor* ABaseFormation::GetFirstActorOfInterest_Implementation()
{
	return FirstActorOfInterest;
}

void ABaseFormation::OnNoiseTimeout(AActor* NoiseSource)
{
	if (LastKnownPosition.Contains(NoiseSource))
	{
		LastKnownPosition.Remove(NoiseSource);
		ClearNoiseTimeoutTimer(NoiseSource);
		IITargetable::Execute_SetIsSeenBy(NoiseSource, this, false);
		CheckFirstActorOfInterest(NoiseSource);
	}
}

void ABaseFormation::CheckFirstActorOfInterest(AActor* CompareActor)
{
	if (FirstActorOfInterest == CompareActor)
	{
		if (EnemiesInSight.IsEmpty())
		{
			if (LastKnownPosition.IsEmpty())
			{
				ClearCoverData();
			}
			else
			{
				FirstActorOfInterest = LastKnownPosition.begin()->Key;
			}
		}
		else
		{
			FirstActorOfInterest = EnemiesInSight.begin()->Key;
		}
	}
}

void ABaseFormation::EnemyDies(AActor* DyingEnemy)
{
	if (LastKnownPosition.Contains(DyingEnemy))
	{
		LastKnownPosition.Remove(DyingEnemy);
		ClearNoiseTimeoutTimer(DyingEnemy);
	}
	if (EnemiesInSight.Contains(DyingEnemy))
	{
		EnemiesInSight.Remove(DyingEnemy);
	}
	CheckFirstActorOfInterest(DyingEnemy);
}

// This function is run on server when Unit dies
void ABaseFormation::UnitDied(APawn* DyingUnit)
{
	AliveUnits--;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Unit %s died in formation %s, Units Alive: %d"), *DyingUnit->GetName(), *GetName(), AliveUnits));
	if (AliveUnits == 0)
	{
		// tell server to respawn formation if tickets are available
		IPlayerStateInterface::Execute_RespawnFormation(IIPlayer::Execute_GetPlayerStateActor(OwningPlayer), this);
	}
}

void ABaseFormation::SetFormationCost(int Cost)
{
	FormationCostInTickets = Cost;
}

void ABaseFormation::FindCovers(FVector CoverFromLocation)
{
	check(FindCoverQuery);
	SetMiddlePosition(1000.0f);

	FEnvQueryRequest QueryRequest(FindCoverQuery, this);
	QueryRequest.Execute(EEnvQueryRunMode::AllMatching, this, &ABaseFormation::OnCoverQueryFinished);
}

void ABaseFormation::OnCoverQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (Result->IsSuccessful())
	{
		CoverLocations.Empty();
		Result->GetAllAsLocations(CoverLocations);

		for(int i = 0; i < Result->Items.Num(); i++)
		{
			Result->GetItemAsLocation(i);
			Result->GetItemScore(i);
			FLinearColor Color = FLinearColor::LerpUsingHSV(FLinearColor::Red, FLinearColor::Green, Result->GetItemScore(i));
			DrawDebugSphere(GetWorld(), Result->GetItemAsLocation(i), 50.0f, 12, Color.ToFColor(true), false, PrimaryActorTick.TickInterval);
		}
		
		// limit ActorsInFormation or CoverLocations so that they both have the same size
		TArrayView<APawn*> LimitedActorsInFormation = MakeArrayView(ActorsInFormation.GetData(), ActorsInFormation.Num());
		TArrayView<FVector> LimitedCoverLocations = MakeArrayView(CoverLocations.GetData(), CoverLocations.Num());

		if(ActorsInFormation.Num() > CoverLocations.Num())
		{
			LimitedActorsInFormation = MakeArrayView(ActorsInFormation.GetData(), CoverLocations.Num());
		}
		else if (CoverLocations.Num() > ActorsInFormation.Num())
		{
			CoverLocations.Sort([this](const FVector& One, const FVector& Two)
			{
				return FVector::DistSquared(One, GetActorLocation()) < FVector::DistSquared(Two, GetActorLocation());
			});
			LimitedCoverLocations = MakeArrayView(CoverLocations.GetData(), ActorsInFormation.Num());
		}
		BottleneckAssignmentComponent->SolveAssignmentAsync(LimitedActorsInFormation, LimitedCoverLocations, [this](const TMap<APawn*, FVector> Assignment)
			{
				for (const auto& Pair : Assignment)
				{
					APawn* Unit = Pair.Key;
					FVector AssignedCover = Pair.Value;
					if (IsValid(Unit))
					{
						DrawDebugLine(GetWorld(), Unit->GetActorLocation(), AssignedCover, FColor::Blue, false, PrimaryActorTick.TickInterval);
					}
				}
			});
	}
}

void ABaseFormation::FinalizeFormation()
{
	IIPlayer::Execute_InitializeRTSHUD(OwningPlayer, this);
}

TMap<AActor*, int> ABaseFormation::GetEnemiesInSight_Implementation()
{
	return EnemiesInSight;
}

TMap<AActor*, FVector> ABaseFormation::GetLastKnownPosition_Implementation()
{
	return LastKnownPosition;
}

TArray<APawn*> ABaseFormation::GetUnitsInFormation_Implementation()
{
	return ActorsInFormation;
}

void ABaseFormation::AddUnitToFormation_Implementation(APawn* UnitToAdd)
{
	ActorsInFormation.AddUnique(UnitToAdd);
	AliveUnits++;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Unit %s added to formation %s, Units Alive: %d"), *UnitToAdd->GetName(), *GetName(), AliveUnits));
	IFormable::Execute_SetFormation(UnitToAdd, this);
}

void ABaseFormation::SetNoiseTimeoutTimer(AActor* EnemyUnit)
{
	ClearNoiseTimeoutTimer(EnemyUnit);
	LastPositionTimerMap.Add({ EnemyUnit, FTimerHandle() });
	FTimerDelegate NoiseTimeoutDelegate = FTimerDelegate::CreateUObject(this, &ABaseFormation::OnNoiseTimeout, EnemyUnit);
	GetWorldTimerManager().SetTimer(LastPositionTimerMap[EnemyUnit], NoiseTimeoutDelegate, 60.0f, false);
}

void ABaseFormation::ClearNoiseTimeoutTimer(AActor* EnemyUnit)
{
	if (LastPositionTimerMap.Contains(EnemyUnit))
	{
		GetWorldTimerManager().ClearTimer(LastPositionTimerMap[EnemyUnit]);
		LastPositionTimerMap.Remove(EnemyUnit);
	}
}

void ABaseFormation::SetMiddlePosition(float DistanceThreshold)
{
	if (ActorsInFormation.IsEmpty()) return;
	FVector InitialSum = FVector::ZeroVector;
	int32 ValidCount = 0;

	for(AActor* Actor : ActorsInFormation)
	{
		if (IsValid(Actor))
		{
			InitialSum += Actor->GetActorLocation();
			ValidCount++;
		}
	}

	if(ValidCount == 0) return;

	FVector InitialCenter = InitialSum / ValidCount;
	float ThresholdSqr = FMath::Square(DistanceThreshold);

	FVector FilteredSum = FVector::ZeroVector;
	int32 FilteredCount = 0;

	for(AActor* Actor : ActorsInFormation)
	{
		if (IsValid(Actor))
		{
			FVector Location = Actor->GetActorLocation();
			if ((Location - InitialCenter).SizeSquared() <= ThresholdSqr)
			{
				FilteredSum += Location;
				FilteredCount++;
			}
		}
	}

	if(FilteredCount == 0) return;

	SetActorLocation(FilteredSum / FilteredCount);
}

void ABaseFormation::ClearCoverData()
{
	FirstActorOfInterest = nullptr;
	CoverLocations.Empty();
}