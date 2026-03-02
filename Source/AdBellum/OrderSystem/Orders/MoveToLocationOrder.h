// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Unit/Selectable.h"
#include "OrderSystem/OrderSystem.h"

class ADBELLUM_API MoveLocationOrder : public BaseOrder
{
public:
	MoveLocationOrder(AActor* inTargetUnit, FVector inLocation) : BaseOrder(nullptr, inLocation) {}

	virtual ~MoveLocationOrder() {}

	virtual void Execute() override
	{
		IOrderable::Execute_DoStandUp(owningController);
		IOrderable::Execute_MoveOrder(owningController, targetPosition);
	}

	virtual void Finalize() override
	{
		IOrderable::Execute_Stop(owningController, targetPosition);
	}

	virtual bool IsFinished() const override
	{
		return !ISelectable::Execute_IsAlive(owningController->GetPawn()) || BaseOrder::IsFinished();
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Move;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Move>
{
	using OrderType = MoveLocationOrder;
};
