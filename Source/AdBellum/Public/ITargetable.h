// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Library/AdBellumEnumLibrary.h"
#include "ITargetable.generated.h"

class ABaseFormation;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UITargetable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IITargetable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		FVector GetHeadLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		FVector GetChestLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		FVector GetLeftArmLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		FVector GetRightArmLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		FVector GetLeftLegLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		FVector GetRightLegLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		FVector GetWeaponLocation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		TArray<AActor*> IsTargetedBy();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		void SetIsTargetedBy(AActor* Actor, bool IsTargeted);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		void SetIsSeenBy(ABaseFormation* Actor, bool IsSeen);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		float GetArmourParamValueForBodyPart(EBodyPart BodyPart);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ITargetable")
		void ClearTarget();
};
