// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "Player/IAreaOfInterest.h"
#include "MoveToLocationOrder.h"
#include "Interfaces/ITargetable.h"
#include "StopOrder.h"
#include "Math/UnrealMathUtility.h"

class ADBELLUM_API OccupyAOIOrder : public BaseOrder
{
public:
	OccupyAOIOrder(UObject* inTargetUnit, FVector inLocation) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~OccupyAOIOrder() {}

	virtual void Execute() override
	{
		BaseOrder::Execute();
		TArray<AActor*> AOIs = IIAreaOfInterest::Execute_GetUnoccupiedPOI(targetUnit);
		if (!AOIs.IsEmpty())
		{
			int32 RandomIndex = FMath::RandRange(0, AOIs.Num() - 1);
			OccupiedPOI = AOIs[RandomIndex];
			IIAreaOfInterest::Execute_OccupyPOI(targetUnit, owningController->GetPawn(), OccupiedPOI);
			RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, OccupiedPOI->GetActorLocation()));
		}
		else
		{
			RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, Cast<AActor>(targetUnit)->GetActorLocation()));
		}
	}

	virtual void Update() override
	{
		if (!OccupiedPOI)
		{
			TArray<AActor*> AOIs = IIAreaOfInterest::Execute_GetUnoccupiedPOI(targetUnit);
			if (!AOIs.IsEmpty())
			{
				FinishSubOrder();
				int32 RandomIndex = FMath::RandRange(0, AOIs.Num() - 1);
				OccupiedPOI = AOIs[RandomIndex];
				IIAreaOfInterest::Execute_OccupyPOI(targetUnit, owningController->GetPawn(), OccupiedPOI);
				RunSubOrder(MakeUnique<MoveLocationOrder>(nullptr, OccupiedPOI->GetActorLocation()));
			}
		}
		else if (!HasSubOrders())
		{
			RunSubOrder(MakeUnique<StopOrder>(nullptr, FVector::ZeroVector));
		}
	}

	virtual void Finalize() override
	{
		if (OccupiedPOI)
		{
			IIAreaOfInterest::Execute_UnoccupyPOI(targetUnit, owningController->GetPawn(), OccupiedPOI);
		}
	}

	virtual bool IsFinished() const override
	{
		return !ITargetable::Execute_IsAlive(owningController->GetPawn()) || BaseOrder::IsFinished();
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::OccupyAOI;
	}

	virtual void Cleanup() override
	{
		BaseOrder::Cleanup();
		Finalize();
	}

private:
	AActor* OccupiedPOI = nullptr;
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::OccupyAOI>
{
	using OrderType = OccupyAOIOrder;
};
