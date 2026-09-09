// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "OrderSystem/Orderable.h"
#include "Trainer/TrainingHelper.h"
#include "MoveToLocationOrder.h"
#include "AttackUnitOrder.h"

class ADBELLUM_API TrainingOrder : public BaseOrder
{
public:
	//inTargetUnit is in this case ShootingRange
	TrainingOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~TrainingOrder() {}

	virtual void Execute() override
	{
		BaseOrder::Execute();
		PositionIndex = ITrainingHelper::Execute_AssignStartPosition(targetUnit, owningController->GetPawn());
		if (PositionIndex == -1)
		{
			return;
		}
		FVector StartPosition = ITrainingHelper::Execute_GetStartPosition(targetUnit, PositionIndex);
		RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, StartPosition));
	}

	virtual void Finalize() override
	{
		ITrainingHelper::Execute_FinishTraining(targetUnit, PositionIndex);
	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished() && PositionIndex != -1;
	}

	virtual OrderEnum GetOrderType() const override
	{
		return HasSubOrders() ? subOrder->GetOrderType() : OrderEnum::Training;
	}
	
	virtual void Update() override
	{
		BaseOrder::Update();
		if (!HasSubOrders())
		{
			bIsInPosition = true;
			AActor* TargetActor = ITrainingHelper::Execute_GetTargetActor(targetUnit, PositionIndex);
			if (TargetActor)
			{
				RunSubOrder(MakeUnique<AttackUnitOrder>(TargetActor, FVector::ZeroVector));
			}
		}
	}

	int PositionIndex;
	bool bIsInPosition = false;
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Training>
{
	using OrderType = TrainingOrder;
};
