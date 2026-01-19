// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "OrderSystem/InteractOrder.h"
#include "Vehicle/Enterable.h"

class ADBELLUM_API EnterOrder : public BaseOrder
{
public:
	EnterOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, inTargetPosition) {}

	virtual ~EnterOrder() {}

	virtual void Execute() override
	{
		UVehicleAccessPoint* accessPoint = IEnterable::Execute_TryBookSeat(targetUnit, Cast<AALSBaseCharacter>(owningUnit));
		if (accessPoint)
		{
			RunSubOrder(MakeUnique<InteractOrder>(accessPoint, FVector::ZeroVector));
		}
	}

	virtual void Finalize() override
	{

	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished() || !HasSubOrders();
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Enter;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Enter>
{
	using OrderType = EnterOrder;
};
