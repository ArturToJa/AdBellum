// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IAreaOfInterest.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIAreaOfInterest : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IIAreaOfInterest
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IAreaOfInterest")
		TArray<AActor *> GetUnoccupiedPOI();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IAreaOfInterest")
		bool OccupyPOI(APawn * Pawn, AActor * POI);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IAreaOfInterest")
		bool UnoccupyPOI(APawn* Pawn, AActor* POI);
};
