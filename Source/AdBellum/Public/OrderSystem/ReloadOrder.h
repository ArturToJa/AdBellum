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
		TScriptInterface<IIWeapon> Weapon = IOrderable::Execute_GetWeapon(owningUnit);
		WeaponObject = Weapon.GetObject();
		if (WeaponObject)
		{
			if (IIWeapon::Execute_HasAmmoToReload(WeaponObject))
			{
				IALSInputInterface::Execute_ReloadAction(owningUnit);
			}
			else
			{
				IIWeapon::Execute_ChangeWeapon(WeaponObject);
			}
		}
	}

	virtual void Finalize() override
	{

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

	UObject* WeaponObject;
};

template<>
struct ADBELLUM_API GeneralOrder<OrderEnum::Reload>
{
	using OrderType = ReloadOrder;
};
