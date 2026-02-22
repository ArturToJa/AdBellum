// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"

class ADBELLUM_API StopOrder : public BaseOrder
{
public:
	StopOrder(AActor* inTargetUnit, FVector inLocation) : BaseOrder(nullptr, inLocation) {}

	virtual ~StopOrder() {}

	virtual void Execute() override
	{
		IOrderable::Execute_Stop(owningUnit, targetPosition);
	}

	virtual void Update() override
	{
		BaseOrder::Update();
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
		return OrderEnum::Stop;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Stop>
{
	using OrderType = StopOrder;
};
