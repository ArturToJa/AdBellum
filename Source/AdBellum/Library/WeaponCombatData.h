// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponCombatData.generated.h"

/**
 * Per-weapon combat parameters (AIWeaponCombatParams)
 */
USTRUCT(BlueprintType)
struct FWeaponCombatDataStruct
{
	GENERATED_BODY()

	// Base spread for weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseSpread = 0.0f;

	// Tendency to bias spread (weapon-specific modifier)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadBias = 0.0f;

	// Linear model for burst duration by distance: BurstDuration = a * Distance + b
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Linear slope 'a' for BurstDuration = a*Distance + b"))
	float BurstDurationSlope = 0.0f;

	// Linear intercept 'b' for burst duration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Intercept 'b' for BurstDuration = a*Distance + b"))
	float BurstDurationIntercept = 0.0f;

	// Clamp minimum burst duration (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Minimum burst duration in seconds"))
	float MinBurstDuration = 0.0f;

	// Clamp maximum burst duration (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ToolTip="Maximum burst duration in seconds"))
	float MaxBurstDuration = 10.0f;
};