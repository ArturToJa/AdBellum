// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OwnershipInterface.h"
#include "HUDInformationInterface.generated.h"

class UOrdersManager;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UHUDInformationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IHUDInformationInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
		FString& GetPrimaryInfo();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
		FString& GetSecondaryInfo();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
		FString& GetStatusInfo();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
		UTexture2D* GetIcon();
};
