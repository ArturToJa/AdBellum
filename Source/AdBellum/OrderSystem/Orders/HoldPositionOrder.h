// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"

class ADBELLUM_API HoldPositionOrder : public BaseOrder
{
public:
	HoldPositionOrder(AActor* inTargetUnit, FVector inLocation) : BaseOrder(nullptr, inLocation) {}

	virtual ~HoldPositionOrder() {}

	virtual void Execute() override
	{
		IOrderable::Execute_Stop(owningController, targetPosition);
	}

	virtual void Finalize() override
	{

	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished();
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::HoldPosition;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::HoldPosition>
{
	using OrderType = HoldPositionOrder;
};
