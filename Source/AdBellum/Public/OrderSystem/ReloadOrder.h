// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OrderSystem/OrderSystem.h"
#include "Orderable.h"
#include "Weapon/IWeapon.h"
#include "Character/ALSInputInterface.h"

class ADBELLUM_API ReloadOrder : public BaseOrder
{
public:
	ReloadOrder(UObject* inTargetUnit, FVector inTargetPosition) : BaseOrder(nullptr, FVector::ZeroVector) {}

	virtual ~ReloadOrder() {}

	virtual void Execute() override
	{
		UE_LOG(LogTemp, Verbose, TEXT("ReloadOrder::Execute: starting for owner=%s"), owningUnit ? *owningUnit->GetName() : TEXT("<null>"));
		TScriptInterface<IIWeapon> Weapon = IOrderable::Execute_GetWeapon(owningUnit);
		WeaponObject = Weapon.GetObject();
		if (WeaponObject)
		{
			UE_LOG(LogTemp, Verbose, TEXT("ReloadOrder::Execute: Weapon found = %s"), *WeaponObject->GetName());
			if (IIWeapon::Execute_HasAmmoToReload(WeaponObject))
			{
				UE_LOG(LogTemp, Verbose, TEXT("ReloadOrder::Execute: Weapon has ammo to reload - invoking ReloadAction on owner"));
				IALSInputInterface::Execute_ReloadAction(owningUnit);
			}
			else
			{
				UE_LOG(LogTemp, Verbose, TEXT("ReloadOrder::Execute: Weapon has NO ammo to reload - will attempt ChangeWeapon"));
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
		UE_LOG(LogTemp, Verbose, TEXT("ReloadOrder::Finalize called for owner=%s"), owningUnit ? *owningUnit->GetName() : TEXT("<null>"));

	}

	virtual bool IsFinished() const override
	{
		return BaseOrder::IsFinished() || (WeaponObject == nullptr || !IIWeapon::Execute_IsReloading(WeaponObject));
	}

	virtual OrderEnum GetOrderType() const override
	{
		return HasSubOrders() ? subOrder->GetOrderType() : OrderEnum::Reload;
	}
	
	virtual void Update() override
	{
		UE_LOG(LogTemp, Verbose, TEXT("ReloadOrder::Update called for owner=%s"), owningUnit ? *owningUnit->GetName() : TEXT("<null>"));
		BaseOrder::Update();
	}

	UObject* WeaponObject;
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Reload>
{
	using OrderType = ReloadOrder;
};
