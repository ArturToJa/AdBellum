// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Selectable.h"
#include "Formable.generated.h"

class ABaseFormation;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFormable : public USelectable//, public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IFormable : public ISelectable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Formable")
	void SetFormation(ABaseFormation* Formation);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Formable")
	ABaseFormation* GetFormation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Formable")
	void RespawnUnit(FTransform RespawnTransform);
};
