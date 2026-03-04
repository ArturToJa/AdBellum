// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnitCombatData.generated.h"

/**
 * Per-soldier combat parameters (AICombatParams)
 */
USTRUCT(BlueprintType)
struct FUnitCombatDataStruct
{
	GENERATED_BODY()

	// Per-soldier spread multiplier (skill/veterancy modifies base weapon spread)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpreadMultiplier = 1.0f;

	// Delay before first shot/burst in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InitialDelay = 0.25f;

	// Delay between bursts in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DelayBetweenBursts = 1.0f;

	// Scales computed burst duration (skill effect)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BurstDurationMultiplier = 1.0f;

	// Scales perceived distance when evaluating burst duration (skill effect)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceMultiplier = 1.0f;
};