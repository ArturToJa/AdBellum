// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <Library/ALSCharacterEnumLibrary.h>
#include "GameFramework/HUD.h"
#include "CharacterHUD.generated.h"

UCLASS()
class ADBELLUM_API ACharacterHUD : public AHUD
{
	GENERATED_BODY()
	
public:

	void HUDOpen(AActor* ControlledActor);
	
	void HUDClose();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHUDOpen(AActor* ControlledActor);

	UFUNCTION(BlueprintImplementableEvent)
	void OnHUDClose();

	UFUNCTION(BlueprintImplementableEvent)
	void NotifyRole(EALSStationaryRole StationaryRole);

	UFUNCTION(BlueprintImplementableEvent)
	void NotifyWeapon();

	UFUNCTION(BlueprintImplementableEvent)
	void NotifyWeaponShot();
};
