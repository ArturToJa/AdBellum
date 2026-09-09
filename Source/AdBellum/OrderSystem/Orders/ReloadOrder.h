// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "OrderSystem/Orderable.h"
#include "Weapon/IWeapon.h"
#include "Unit/ArmedUnitInterface.h"
#include "Character/ALSInputInterface.h"

class ADBELLUM_API ReloadOrder : public BaseOrder
{
public:
	ReloadOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(nullptr, FVector::ZeroVector) {}

	virtual ~ReloadOrder() {}

	virtual void Execute() override
	{
		BaseOrder::Execute();
		WeaponObject = IArmedUnitInterface::Execute_GetWeapon(owningController->GetPawn());
		if (WeaponObject)
		{
			if (IIWeapon::Execute_HasAmmoToReload(WeaponObject))
			{
				IALSInputInterface::Execute_ReloadAction(owningController->GetPawn());
			}
			else
			{
				IIWeapon::Execute_ChangeWeapon(WeaponObject);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ReloadOrder::Execute: No weapon returned from owningUnit"));
		}
	}

	virtual void Finalize() override
	{
		UE_LOG(LogTemp, Verbose, TEXT("ReloadOrder::Finalize called for owner=%s"), owningController ? *owningController->GetName() : TEXT("<null>"));

	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished() || (WeaponObject == nullptr || !IIWeapon::Execute_IsReloading(WeaponObject));
	}

	virtual OrderEnum GetOrderType() const override
	{
		return OrderEnum::Reload;
	}
	
	virtual void Update() override
	{
		BaseOrder::Update();
	}

	AActor* WeaponObject;
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Reload>
{
	using OrderType = ReloadOrder;
};
