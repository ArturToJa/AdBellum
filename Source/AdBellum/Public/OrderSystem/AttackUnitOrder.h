// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "ReloadOrder.h"
#include "Orderable.h"
#include "Weapon/IWeapon.h"
#include "Unit/UnitAIController.h"

class ADBELLUM_API AttackUnitOrder : public BaseOrder
{
private:
	UObject* WeaponObject;
	float AttackDelay = 0.5f;
public:
	AttackUnitOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(inTargetUnit, FVector::ZeroVector) {}

	virtual ~AttackUnitOrder() {}

	virtual void Execute() override
	{
		TScriptInterface<IIWeapon> Weapon = IOrderable::Execute_GetWeapon(owningUnit);
		WeaponObject = Weapon.GetObject();

		AttackDelay = 0.5f; // set this delay based on unit and weapon stats

		if (APawn* PawnUnit = Cast<APawn>(owningUnit))
		{
			AUnitAIController* UnitController = Cast<AUnitAIController>(PawnUnit->GetController());
			if (UnitController)
			{
				UnitController->SetTarget(Cast<AActor>(targetUnit));
			}
		}
	}

	virtual void Finalize() override
	{
		if (APawn* PawnUnit = Cast<APawn>(owningUnit))
		{
			AUnitAIController* UnitController = Cast<AUnitAIController>(PawnUnit->GetController());
			if (UnitController)
			{
				UnitController->ClearTarget();
			}
		}
		if (WeaponObject)
		{
			IIWeapon::Execute_ResetAim(WeaponObject);
		}
	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished() || (!ISelectable::Execute_IsAlive(targetUnit) || WeaponObject == nullptr);
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Attack;
	}
	//
	virtual void Update() override
	{
		BaseOrder::Update();
		if (IIWeapon::Execute_IsReloading(WeaponObject))
		{
			UE_LOG(LogTemp, Warning, TEXT("AttackUnitOrder: Weapon is reloading, cannot attack"));
			return;
		}

		if (AttackDelay > 0.f)
		{
			AttackDelay -= owningUnit->GetWorld()->GetDeltaSeconds();
			return;
		}

		if (IIWeapon::Execute_HasAmmo(WeaponObject))
		{
			IOrderable::Execute_AttackTarget(owningUnit, targetUnit);
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
