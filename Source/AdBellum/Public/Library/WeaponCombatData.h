// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponCombatData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FWeaponCombatDataStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseSpread;
};