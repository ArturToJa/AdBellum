// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OwnershipInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UOwnershipInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IOwnershipInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "OwnershipInterface")
		void SetOwningPlayer(AActor* Player);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "OwnershipInterface")
		AActor* GetOwningPlayer();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "OwnershipInterface")
		void SetTeamIndex(int32 Index);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "OwnershipInterface")
		int32 GetTeamIndex();
};
