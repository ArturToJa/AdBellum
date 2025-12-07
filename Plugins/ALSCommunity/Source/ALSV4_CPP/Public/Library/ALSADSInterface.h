// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ALSADSInterface.generated.h"

UINTERFACE(MinimalAPI)
class UALSADSInterface : public UInterface
{
	GENERATED_BODY()
};

class ALSV4_CPP_API IALSADSInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ALS|ADS")
	FVector getADSTarget();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ALS|ADS")
	FRotator getADSRotation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ALS|Vehicle camera")
	FTransform getCameraTransform(EALSStationaryRole Role);
};