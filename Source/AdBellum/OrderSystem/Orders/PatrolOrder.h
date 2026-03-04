// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "Interfaces/ITargetable.h"
#include "MoveToLocationOrder.h"

class ADBELLUM_API PatrolOrder : public BaseOrder
{
public:
	PatrolOrder(AActor* inTargetUnit, FVector inLocation) : BaseOrder(nullptr, inLocation) {}

	virtual ~PatrolOrder() {}

	virtual void Execute() override
	{
		RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, targetPosition));
		targetPosition = owningController->GetPawn()->GetActorLocation();
	}

	virtual void Update() override
	{
		if (!HasSubOrders())
		{
			RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, targetPosition));
			targetPosition = owningController->GetPawn()->GetActorLocation();
		}
	}

	virtual void Finalize() override
	{

	}

	virtual bool IsFinished() const override
	{
		return !ITargetable::Execute_IsAlive(owningController->GetPawn()) || BaseOrder::IsFinished();
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Patrol;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Patrol>
{
	using OrderType = PatrolOrder;
};
