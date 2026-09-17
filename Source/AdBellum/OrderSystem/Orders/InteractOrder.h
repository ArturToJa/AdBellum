// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "MoveToLocationOrder.h"
#include "Character/ALSInteractionInterface.h"

class ADBELLUM_API InteractOrder : public BaseOrder
{
public:
	InteractOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~InteractOrder() {}

	virtual void Execute() override
	{
		if (bSilent) return;
		BaseOrder::Execute();
		RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, IALSInteractionInterface::Execute_InteractLocation(targetUnit)));
	}

	virtual void Finalize() override
	{
		IALSInteractionInterface::Execute_Interact(targetUnit, Cast<AALSBaseCharacter>(owningController->GetPawn()));
	}

	virtual bool IsFinished() const override
	{
		FVector OwnerLocation = owningController->GetPawn()->GetActorLocation();
		float Distance = FVector::Distance(OwnerLocation, IALSInteractionInterface::Execute_InteractLocation(targetUnit));
		return BaseOrder::IsFinished() || Distance <= IALSInteractionInterface::Execute_InteractRange(targetUnit);
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Interact;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Interact>
{
	using OrderType = InteractOrder;
};
