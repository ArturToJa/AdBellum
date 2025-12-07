// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/VehicleSeatComponent.h"
#include "Character/ALSBaseCharacter.h"
#include "GunnerSeatComponent.generated.h"

class AALSBaseCharacter;

DECLARE_DELEGATE_RetVal_OneParam(AActor*, OnGunnerWeaponChanged, int)

UCLASS()
class ADBELLUM_API UGunnerSeatComponent : public UVehicleSeatComponent
{
public:
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	OnGunnerWeaponChanged GunnerWeaponChanged;

	UPROPERTY(Replicated, BlueprintReadWrite, EditDefaultsOnly, Meta = (UseComponentPicker))
	TArray<FComponentReference> WeaponArray;

	void SetWeaponArrayOwner(AALSBaseCharacter* InUnit);

	virtual bool SetOccupyingUnit(AALSBaseCharacter* InUnit) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BindDelegateToUnit(AALSBaseCharacter* InUnit);

	void GetControlledWeapon(int Id);
	AActor* GetWeapon(int Id);
};
