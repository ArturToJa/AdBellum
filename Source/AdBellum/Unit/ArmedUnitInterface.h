// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ArmedUnitInterface.generated.h"


UINTERFACE(MinimalAPI)
class UArmedUnitInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IArmedUnitInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ArmedUnitInterface")
	AActor* GetWeapon();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ArmedUnitInterface")
	void OnWeaponUpdated(AActor* Weapon);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ArmedUnitInterface")
	bool IsReloading();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ArmedUnitInterface")
	void GetUnitCombatDataStruct(FUnitCombatDataStruct& OutCombatData);
};
