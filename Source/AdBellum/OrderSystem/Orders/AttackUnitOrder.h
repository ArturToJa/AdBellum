// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "ReloadOrder.h"
#include "OrderSystem/Orderable.h"
#include "Interfaces/ITargetable.h"
#include "Weapon/IWeapon.h"
#include "Unit/ArmedUnitInterface.h"
#include "Unit/UnitAIController.h"
#include "Library/UnitCombatData.h"
#include "Library/WeaponCombatData.h"

class ADBELLUM_API AttackUnitOrder : public BaseOrder
{
private:
	AActor* WeaponObject;
	float AttackDelay = 0.5f;
	FUnitCombatDataStruct UnitCombatData;
	FWeaponCombatDataStruct WeaponCombatData;
public:
	AttackUnitOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~AttackUnitOrder() {}

	virtual void Execute() override
	{
		WeaponObject = IArmedUnitInterface::Execute_GetWeapon(owningController->GetPawn());

		// Initialize combat data structures for unit and weapon
	
		IArmedUnitInterface::Execute_GetUnitCombatDataStruct(owningController->GetPawn(), UnitCombatData);

		if (WeaponObject)
		{
			IIWeapon::Execute_GetWeaponCombatData(WeaponObject, WeaponCombatData);
		}

		// Try to obtain initial delay from owning unit's combat data
		if (owningController && owningController->GetPawn())
		{
			AttackDelay = UnitCombatData.InitialDelay;
		}

		AUnitAIController* UnitController = Cast<AUnitAIController>(owningController);
		if (UnitController)
		{
			UnitController->SetTarget(Cast<AActor>(targetUnit));
		}
	}

	virtual void Finalize() override
	{
		AUnitAIController* UnitController = Cast<AUnitAIController>(owningController);
		if (UnitController)
		{
			UnitController->ClearTarget();
		}
		if (WeaponObject)
		{
			IIWeapon::Execute_ResetAim(WeaponObject);
		}
	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished() || (!ITargetable::Execute_IsAlive(targetUnit) || WeaponObject == nullptr);
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Attack;
	}
	//
	virtual void Update() override
	{
		BaseOrder::Update();
		if (!WeaponObject)
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder: No weapon available"));
			return;
		}

		if (IIWeapon::Execute_IsReloading(WeaponObject) || IIWeapon::Execute_IsShooting(WeaponObject))
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder: Weapon is reloading or already shooting, cannot attack"));
			return;
		}

		if (AttackDelay > 0.f)
		{
			AttackDelay -= owningController->GetWorld()->GetDeltaSeconds();
			return;
		}

		if (IIWeapon::Execute_HasAmmo(WeaponObject))
		{
			IOrderable::Execute_AttackTarget(owningController, targetUnit);

			// After initiating an attack, set delay between bursts from weapon/unit combat data
			if (owningController && owningController->GetPawn())
			{
				AActor* TargetActor = Cast<AActor>(targetUnit);
				AttackDelay = CalculateBurstDelay(TargetActor);
			}
			else
			{
				AttackDelay = WeaponCombatData.MinBurstDelay;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder: No ammo, reloading"));
			RunSubOrder(MakeUnique<ReloadOrder>(nullptr, FVector::ZeroVector));
		}
	}

	// Calculate burst delay (seconds) using linear model from weapon data and unit multipliers
	float CalculateBurstDelay(AActor* TargetActor) const
	{
		// Fallbacks
		if (!WeaponObject) return 0.5f;
		// Use previously-initialized combat data; if absent, try to compute basic defaults
		float Distance = 0.0f;
		
		Distance = FVector::Dist(owningController->GetPawn()->GetActorLocation(), TargetActor->GetActorLocation()) / 100.0f; // match UnitAIController logic
	
		float PerceivedDistance = Distance * UnitCombatData.DistanceMultiplier;

		// Quadratic distance model: BurstDelay = a * (PerceivedDistance)^2 + b
		float DistanceSquared = PerceivedDistance * PerceivedDistance;
		float BaseDelay = WeaponCombatData.BurstDelaySlope * DistanceSquared + WeaponCombatData.BurstDelayIntercept;

		if (BaseDelay < 0.0f)
		{
			BaseDelay = WeaponCombatData.MinBurstDelay;
		}
		float Clamped = FMath::Clamp(BaseDelay, WeaponCombatData.MinBurstDelay, WeaponCombatData.MaxBurstDelay);

		float Result = Clamped * UnitCombatData.DelayBetweenBurstsMultiplier;

		if (Result <= 0.0f)
		{
			Result = WeaponCombatData.MinBurstDelay;
		}

		if (Result > WeaponCombatData.MaxBurstDelay)
		{
			Result = WeaponCombatData.MaxBurstDelay;
		}

		// Apply slight random variation to avoid perfectly uniform timing
		const float RandMultiplier = FMath::FRandRange(0.85f, 1.15f);
		Result *= RandMultiplier;

		return Result;
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Attack>
{
	using OrderType = AttackUnitOrder;
};
