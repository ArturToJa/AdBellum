// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Orderable.h"
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
		owningUnit = other.owningUnit;
		targetUnit = other.targetUnit;
		targetPosition = other.targetPosition;
		bIsAggresive = other.bIsAggresive;
		subOrder = MoveTemp(other.subOrder);
	}

	BaseOrder(BaseOrder&& other)
	{
		owningUnit = other.owningUnit;
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
				OnOrderCompleted.ExecuteIfBound();
			}
		}
	}
	virtual OrderEnum GetOrderType() const
	{
		return HasSubOrders() ? subOrder->GetOrderType() : OrderEnum::Stop;
	}

	virtual bool IsFinished() const
	{
		// If there's an active sub-order, check if it's finished
		return !owningUnit || HasSubOrders() ? subOrder->IsFinished() : false;
	}

	void SetOwner(AActor* inOwningUnit)
	{
		owningUnit = inOwningUnit;
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
			UE_LOG(LogTemp, Verbose, TEXT("BaseOrder::RunSubOrder: Delegating to existing suborder. Owner=%s"), owningUnit ? *owningUnit->GetName() : TEXT("<null>"));
			subOrder->RunSubOrder(MoveTemp(inSubOrder));
		}
		else
		{
			// If no existing sub-order is present, assign the new sub-order directly
			subOrder = MoveTemp(inSubOrder);
			subOrder->SetOwner(owningUnit);
			UE_LOG(LogTemp, Verbose, TEXT("BaseOrder::RunSubOrder: Created new suborder for owner=%s Type=%d"), owningUnit ? *owningUnit->GetName() : TEXT("<null>"), (int)subOrder->GetOrderType());
			subOrder->OnOrderCompleted.BindRaw(this, &BaseOrder::FinishSubOrder);
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

public:
	FSimpleDelegate OnOrderCompleted;

protected:
	AActor* owningUnit;
	UObject* targetUnit;
	FVector targetPosition;
	bool bIsAggresive = true;
	TUniquePtr<BaseOrder> subOrder;
};

template<OrderEnum Order>
struct ADBELLUM_API GeneralOrder;
