// Fill out your copyright notice in the Description page of Project Settings.


#include "OrdersManager.h"
#include "Orderable.h"
#include "BrainComponent.h"
#include "Orders/GeneralOrders.h"
#include "AIController.h"


DEFINE_LOG_CATEGORY(OrdersManager);

// Sets default values for this component's properties
UOrdersManager::UOrdersManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UOrdersManager::BeginPlay()
{
	if (!GetOwner()->GetClass()->ImplementsInterface(UOrderable::StaticClass()))
	{
		UE_LOG(OrdersManager, Log, TEXT("This Actor doesn't implement Orderable interface"));
	}

	CurrentOrder = MakeUnique<StopOrder>(nullptr, GetOwner()->GetActorLocation());
	CurrentOrder->SetOwner(Cast<AAIController>(GetOwner()));
	CurrentOrder->OnOrderCompleted.AddUObject(this, &UOrdersManager::NotifyCurrentOrderCompleted);
	Super::BeginPlay();
}

void UOrdersManager::RunAILogic()
{
	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StartLogic();
		}
	}
}

void UOrdersManager::StopAILogic()
{
	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic("");
		}
	}
}

void UOrdersManager::SetStopOrder()
{
	AddOrder(MakeUnique<StopOrder>(nullptr, GetOwner()->GetActorLocation()), false);
}

void UOrdersManager::ProcessNextOrder()
{
	if (!OrderQueue.IsEmpty())
	{
		PerformOrder(MoveTemp(OrderQueue[0]));
		OrderQueue.RemoveAt(0);
	}
}

bool UOrdersManager::ShouldAttackEnemies()
{
	return CurrentOrder->IsAggresive();
}

void UOrdersManager::NotifyCurrentOrderCompleted()
{
	if (CurrentOrder->HasSubOrders())
	{
		CurrentOrder->FinishSubOrder();
	}
	else
	{
		NotifyMainOrderCompleted();
	}
	OnHUDNotify.ExecuteIfBound();
}

void UOrdersManager::NotifyMainOrderCompleted()
{
	CurrentOrder->Finalize();
	SetStopOrder();
	ProcessNextOrder();
}

void UOrdersManager::NotifySubOrderStarted()
{
	OnHUDNotify.ExecuteIfBound();
}

bool UOrdersManager::IsCurrentOrderOfType(OrderEnum Type)
{
	return CurrentOrder->GetSubOrderType() == Type;
}
bool UOrdersManager::IsOrderOfType(OrderEnum Type)
{
	return CurrentOrder->GetOrderType() == Type;
}

void UOrdersManager::PerformOrder(TUniquePtr<BaseOrder> OrderToPerform)
{
	if (CurrentOrder)
	{
		CurrentOrder->Cleanup();
	}
	CurrentOrder.Reset();
	CurrentOrder = MoveTemp(OrderToPerform);
	CurrentOrder->SetOwner(Cast<AAIController>(GetOwner()));
	CurrentOrder->OnOrderCompleted.AddUObject(this, &UOrdersManager::NotifyCurrentOrderCompleted);
	CurrentOrder->OnSubOrderStarted.BindUObject(this, &UOrdersManager::NotifySubOrderStarted);
	CurrentOrder->Execute();
	OnHUDNotify.ExecuteIfBound();
}

void UOrdersManager::UpdateOrder()
{
	if (CurrentOrder->IsFinished())
	{
		NotifyCurrentOrderCompleted();
	}
	else
	{
		CurrentOrder->Update();
	}
}

bool UOrdersManager::HasOrders()
{
	return CurrentOrder && CurrentOrder->GetOrderType() != OrderEnum::Stop;
}

void UOrdersManager::MoveOrder(FVector TargetPosition)
{
	CurrentOrder->RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, TargetPosition));
}

void UOrdersManager::HideBehindCover(FVector TargetPosition)
{
	CurrentOrder->RunSubOrder(MakeUnique<TakeCoverOrder>(nullptr, TargetPosition));
}

void UOrdersManager::AddOrder(TUniquePtr<BaseOrder> OrderToPerform, bool bIsQueued)
{
	if (bIsQueued)
	{
		if (!OrderQueue.IsEmpty() && OrderQueue.Last() == OrderToPerform)
		{
			SetAsNonAggressiveOrderInQueue();
		}
		else
		{
			bool WasEmpty = OrderQueue.IsEmpty();
			OrderQueue.Emplace(MoveTemp(OrderToPerform));
			if (WasEmpty)
			{
				ProcessNextOrder();
			}
		}
	}
	else
	{
		if (CurrentOrder == OrderToPerform)
		{
			SetAsNonAggressiveOrder();
		}
		else
		{
			OrderQueue.Empty();
			PerformOrder(MoveTemp(OrderToPerform));
		}
	}
}

bool UOrdersManager::IsRunningSubOrders()
{
	return CurrentOrder->HasSubOrders();
}

void UOrdersManager::SetAsNonAggressiveOrder()
{
	CurrentOrder->SetNonAggresive();
}

void UOrdersManager::SetAsNonAggressiveOrderInQueue()
{
	OrderQueue.Last()->SetNonAggresive();
}

OrderEnum UOrdersManager::GetOrderType()
{
	return CurrentOrder->GetOrderType();
}

OrderEnum UOrdersManager::GetCurrentOrderType()
{
	return CurrentOrder->GetSubOrderType();
}

BaseOrder* UOrdersManager::GetOrder() 
{
	if (HasOrders()) 
	{
		return CurrentOrder.Get();
	}
	return nullptr;
}

BaseOrder* UOrdersManager::GetSubOrder()
{
	if (HasOrders()) 
	{
		return CurrentOrder->GetSubOrder();
	}
	return nullptr;
}

void UOrdersManager::BP_AddOrder(OrderEnum OrderType, bool bIsQueued, AActor* TargetObject, FVector TargetPosition)
{
	switch (OrderType)
	{
	case OrderEnum::Move:
	{
		TUniquePtr<MoveLocationOrder> OrderToPerform = MakeUnique<MoveLocationOrder>(nullptr, TargetPosition);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	case OrderEnum::TakeCover:
	{
		TUniquePtr<TakeCoverOrder> OrderToPerform = MakeUnique<TakeCoverOrder>(nullptr, TargetPosition);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	case OrderEnum::Reload:
	{
		TUniquePtr<ReloadOrder> OrderToPerform = MakeUnique<ReloadOrder>(nullptr, FVector::ZeroVector);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	case OrderEnum::Stop:
	{
		TUniquePtr<StopOrder> OrderToPerform = MakeUnique<StopOrder>(nullptr, TargetPosition);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	case OrderEnum::Attack:
	{
		TUniquePtr<AttackUnitOrder> OrderToPerform = MakeUnique<AttackUnitOrder>(TargetObject, FVector::ZeroVector);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	case OrderEnum::HoldPosition:
	{
		TUniquePtr<HoldPositionOrder> OrderToPerform = MakeUnique<HoldPositionOrder>(nullptr, TargetPosition);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	case OrderEnum::Follow:
	{
		TUniquePtr<FollowUnitOrder> OrderToPerform = MakeUnique<FollowUnitOrder>(TargetObject, FVector::ZeroVector);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	case OrderEnum::Patrol:
	{
		TUniquePtr<PatrolOrder> OrderToPerform = MakeUnique<PatrolOrder>(nullptr, TargetPosition);
		AddOrder(MoveTemp(OrderToPerform), bIsQueued);
		return;
	}
	default:
		break;
	}
}