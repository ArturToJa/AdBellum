// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OwnershipInterface.h"
#include "Selectable.generated.h"

class UOrdersManager;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class USelectable : public UOwnershipInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API ISelectable : public IOwnershipInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Selectable")
		void SetSelectionCircle(bool Visible);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Selectable")
		void SetInstanceIndex(int32 Index);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Selectable")
		int32 GetInstanceIndex();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Selectable")
		FVector GetSelectionCircleScale();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Selectable")
		FVector GetSelectionCircleLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Selectable")
		int GetUnitType();
};
