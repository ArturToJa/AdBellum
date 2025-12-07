// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TrainingHelper.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTrainingHelper : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API ITrainingHelper
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Training")
	AActor* GetTargetActor(int PositionIndex);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Training")
	bool IsFinished(int PositionIndex);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Training")
	FVector GetStartPosition(int PositionIndex);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Training")
	int AssignStartPosition(AActor* TrainingUnit);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Training")
	void FinishTraining(int PositionIndex);
};
