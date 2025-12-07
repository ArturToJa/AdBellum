// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ALSCharacterEnumLibrary.h"
#include "ALSMobilityDataInterface.generated.h"

UINTERFACE(MinimalAPI)
class UALSMobilityDataInterface : public UInterface
{
	GENERATED_BODY()
};

class ALSV4_CPP_API IALSMobilityDataInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ALS|MobilityData")
	EALSGait GetGait();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ALS|MobilityData")
	EALSStance GetStance();
};
