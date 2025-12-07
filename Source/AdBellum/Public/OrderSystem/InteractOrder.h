// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "OrderSystem/MoveToLocationOrder.h"
#include "Character/ALSInteractionInterface.h"

class ADBELLUM_API InteractOrder : public BaseOrder
{
public:
	InteractOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~InteractOrder() {}

	virtual void Execute() override
	{
		RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, IALSInteractionInterface::Execute_InteractLocation(targetUnit)));
	}

	virtual void Finalize() override
	{
		IALSInteractionInterface::Execute_Interact(targetUnit, Cast<AALSBaseCharacter>(owningUnit));
	}

	virtual bool IsFinished() const override
	{
		FVector OwnerLocation = owningUnit->GetActorLocation();
		float Distance = FVector::Distance(OwnerLocation, IALSInteractionInterface::Execute_InteractLocation(targetUnit));
		return BaseOrder::IsFinished() || Distance <= IALSInteractionInterface::Execute_InteractRange(targetUnit);
	}

	virtual OrderEnum GetOrderType() const override
	{
		return HasSubOrders() ? subOrder->GetOrderType() : OrderEnum::Interact;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Interact>
{
	using OrderType = InteractOrder;
};
