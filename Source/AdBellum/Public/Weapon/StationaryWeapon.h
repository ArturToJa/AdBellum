// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/SimpleWeapon.h"
#include "StationaryWeapon.generated.h"

/**
 * 
 */
UCLASS()
class ADBELLUM_API AStationaryWeapon : public ASimpleWeapon
{
	GENERATED_BODY()

public:
	AStationaryWeapon();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;
};
