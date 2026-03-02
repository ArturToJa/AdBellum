// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "VehicleStructs.generated.h"

class ABaseVehicle;

USTRUCT(BlueprintType)
struct ADBELLUM_API FVehicleData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<ABaseVehicle> VehicleClass;
};