// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UnitCombatData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FUnitCombatDataStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InitialDelay;
};