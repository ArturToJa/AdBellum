// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Orderable.h"
#include "AIController.h"
#include "OrderSystem/OrderType.h"
#include "Templates/SharedPointer.h"

class ADBELLUM_API BaseOrder
{
public:
	BaseOrder(UObject* inTargetUnit, FVector inTargetPosition)
		: targetUnit(inTargetUnit)
		, targetPosition(inTargetPosition)
	{}

	BaseOrder(BaseOrder& other)
	{
		owningController = other.owningController;
		targetUnit = other.targetUnit;
		targetPosition = other.targetPosition;
		bIsAggresive = other.bIsAggresive;
		subOrder = MoveTemp(other.subOrder);
	}

	BaseOrder(BaseOrder&& other)
	{
		owningController = other.owningController;
		targetUnit = other.targetUnit;
		targetPosition = other.targetPosition;
		bIsAggresive = other.bIsAggresive;
		subOrder = MoveTemp(other.subOrder);
	}

	virtual ~BaseOrder()
	{
		Finalize();
	}

	virtual void Execute() {}
	virtual void Finalize() {}
	virtual void Update()
	{
		if (HasSubOrders())
		{
			subOrder->Update();
		}
		else
		{
			if (IsFinished())
			{
				if (OnOrderCompleted.IsBound())
				{
					OnOrderCompleted.Broadcast();
				}
			}
		}
	}
	virtual OrderEnum GetSubOrderType() const
	{
		return HasSubOrders() ? subOrder->GetSubOrderType() : GetOrderType();
	}

	virtual OrderEnum GetOrderType() const
	{
		return OrderEnum::Stop;
	}

	virtual bool IsFinished() const
	{
		// If there's an active sub-order, check if it's finished
		return !owningController || HasSubOrders() ? subOrder->IsFinished() : false;
	}

	void SetOwner(AAIController* inOwningController)
	{
		owningController = inOwningController;
	}

	UObject* GetTargetUnit() const
	{
		return targetUnit;
	}

	FVector GetTargetPosition() const
	{
		return targetPosition;
	}

	bool IsAggresive() const
	{
		return bIsAggresive;
	}

	void SetNonAggresive()
	{
		bIsAggresive = false;
	}

	void RunSubOrder(TUniquePtr<BaseOrder> inSubOrder)
	{
		if (HasSubOrders())
		{
			// If an existing sub-order is present, assign the new sub-order to the existing one
			UE_LOG(LogTemp, Verbose, TEXT("BaseOrder::RunSubOrder: Delegating to existing suborder. Owner=%s"), owningController ? *owningController->GetName() : TEXT("<null>"));
			subOrder->RunSubOrder(MoveTemp(inSubOrder));
		}
		else
		{
			// If no existing sub-order is present, assign the new sub-order directly
			subOrder = MoveTemp(inSubOrder);
			subOrder->SetOwner(owningController);
			UE_LOG(LogTemp, Verbose, TEXT("BaseOrder::RunSubOrder: Created new suborder for owner=%s Type=%d"), owningController ? *owningController->GetName() : TEXT("<null>"), (int)subOrder->GetOrderType());
			subOrder->OnOrderCompleted.AddRaw(this, &BaseOrder::FinishSubOrder);
			subOrder->OnSubOrderStarted.BindRaw(this, &BaseOrder::StartSubOrder);
			StartSubOrder();
			subOrder->Execute(); // Trigger Execute function on sub-order
		}
	}

	bool operator==(const TUniquePtr<BaseOrder>& OtherOrder)
	{
		return this->GetOrderType() == OtherOrder->GetOrderType() && this->GetTargetUnit() == OtherOrder->GetTargetUnit() && this->GetTargetPosition() == OtherOrder->GetTargetPosition();
	}

	bool HasSubOrders() const
	{
		return subOrder.IsValid();
	}

	void StartSubOrder()
	{
		OnSubOrderStarted.ExecuteIfBound();
	}

	void FinishSubOrder()
	{
		if (HasSubOrders())
		{
			if (subOrder->HasSubOrders())
			{
				subOrder->FinishSubOrder();
			}
			else
			{
				subOrder->Finalize();
				subOrder.Reset();
			}
		}
	}

	virtual void Cleanup()
	{
		Finalize();
		if (HasSubOrders())
		{
			subOrder->Cleanup();
			subOrder.Reset();
		}
	}

	virtual BaseOrder* GetSubOrder() 
	{
		if (HasSubOrders()) 
		{
			return subOrder->GetSubOrder();
		}
		else 
		{
			return this;
		}
	}

public:
	FSimpleMulticastDelegate OnOrderCompleted;
	FSimpleDelegate OnSubOrderStarted;

protected:
	AAIController* owningController;
	UObject* targetUnit;
	FVector targetPosition;
	bool bIsAggresive = true;
	TUniquePtr<BaseOrder> subOrder;
};

template<OrderEnum Order>
struct ADBELLUM_API GeneralOrder;
