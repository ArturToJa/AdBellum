// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VehicleAccessPoint.h"
#include "Enterable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEnterable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IEnterable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Enterable")
		UVehicleAccessPoint* TryBookSeat(AALSBaseCharacter* InUnit);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Enterable")
		TArray<UVehicleAccessPoint*> GetAllAccessPoints();
};
