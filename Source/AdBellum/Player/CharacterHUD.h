// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <Library/ALSCharacterEnumLibrary.h>
#include "GameFramework/HUD.h"
#include "CharacterHUD.generated.h"

class UGunnerCrosshairWidget;

UCLASS()
class ADBELLUM_API ACharacterHUD : public AHUD
{
	GENERATED_BODY()

public:
	ACharacterHUD();

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

	// Native gunner crosshair, independent of whatever the Blueprint-driven widget switch (NotifyRole) does.
	// Shown for Gunner and Gunner_Standing, hidden for every other role.
	void UpdateGunnerCrosshair(EALSStationaryRole StationaryRole);

	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UGunnerCrosshairWidget> GunnerCrosshairWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UGunnerCrosshairWidget> GunnerCrosshairWidget;
};
