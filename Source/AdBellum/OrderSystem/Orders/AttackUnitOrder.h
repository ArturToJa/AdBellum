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

class ADBELLUM_API AttackUnitOrder : public BaseOrder
{
private:
	AActor* WeaponObject;
	float AttackDelay = 0.5f;
public:
	AttackUnitOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~AttackUnitOrder() {}

	virtual void Execute() override
	{
		WeaponObject = IArmedUnitInterface::Execute_GetWeapon(owningController->GetPawn());

		// Try to obtain initial delay from owning unit's combat data
		FUnitCombatDataStruct UnitData;
		if (owningController->GetPawn())
		{
			IArmedUnitInterface::Execute_GetUnitCombatDataStruct(owningController->GetPawn(), UnitData);
			AttackDelay = UnitData.InitialDelay;
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

			// After initiating an attack, set delay between bursts from unit combat data
			if (owningController && owningController->GetPawn())
			{
				FUnitCombatDataStruct UnitData;
				IArmedUnitInterface::Execute_GetUnitCombatDataStruct(owningController->GetPawn(), UnitData);
				AttackDelay = UnitData.DelayBetweenBursts > 0.f ? UnitData.DelayBetweenBursts : 0.5f;
			}
			else
			{
				AttackDelay = 0.5f;
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder: No ammo, reloading"));
			RunSubOrder(MakeUnique<ReloadOrder>(nullptr, FVector::ZeroVector));
		}
	}
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Attack>
{
	using OrderType = AttackUnitOrder;
};
