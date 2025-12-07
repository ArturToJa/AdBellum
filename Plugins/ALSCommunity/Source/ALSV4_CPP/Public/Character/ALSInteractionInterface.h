// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Character/ALSBaseCharacter.h"
#include "AlsInteractionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UALSInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class ALSV4_CPP_API IALSInteractionInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Interaction")
		void Interact(AALSBaseCharacter* Actor);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Interaction")
		float InteractRange();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ALS|Interaction")
		FVector InteractLocation();
};