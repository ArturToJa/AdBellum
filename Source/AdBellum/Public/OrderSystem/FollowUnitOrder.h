// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "OrderSystem/MoveToLocationOrder.h"

class ADBELLUM_API FollowUnitOrder : public BaseOrder
{
public:
	FollowUnitOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~FollowUnitOrder() {}

	virtual void Execute() override
	{
		RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, Cast<AActor>(targetUnit)->GetActorLocation()));
	}

	virtual void Finalize() override
	{

	}

	virtual void Update() override
	{
		if (!HasSubOrders())
		{
			RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, Cast<AActor>(targetUnit)->GetActorLocation()));
		}
	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished();
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Follow;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Follow>
{
	using OrderType = FollowUnitOrder;
};
