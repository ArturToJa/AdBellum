// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Library/Weapon/WeaponSocketEnum.h"

#include "Library/Weapon/WeaponCustomizationData.h"
#include "IWeapon.generated.h"

UENUM(BlueprintType)
enum class AdBellumWeaponTypeEnum : uint8
{
	Pistol UMETA(DisplayName = "Pistol"),
	Assault UMETA(DisplayName = "Assault"),
	SMG UMETA(DisplayName = "SMG"),
	LMG UMETA(DisplayName = "LMG"),
	Shotgun UMETA(DisplayName = "Shotgun"),
	Sniper UMETA(DisplayName = "Sniper"),
	GrenadeLauncher UMETA(DisplayName = "GrenadeLauncher"),
	Knife UMETA(DisplayName = "Knife")
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIWeapon : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IIWeapon
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		UEBBarrel* GetEBarrel();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		AdBellumWeaponTypeEnum GetWeaponType();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		float GetEffectiveRange();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		float GetMaxRange();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		bool HasAmmo();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		bool HasAmmoToReload();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		bool IsReloading();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void Reload();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void ChangeWeapon();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void Trigger(bool trigger);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void BlockShooting(bool Block);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		float GetCameraSensitivity();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void NotifyAim(bool AimActive);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		EWeaponSocketEnum GetWeaponSocket();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void ConfigureWeapon(const FWeaponCustomizationDataStruct& WeaponPrefab);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		EFireMode GetFireMode();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon|IK")
		FTransform GetHandIKTransform(ERelativeTransformSpace TransformSpace);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon|IK")
		bool GetApplyHandIK();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		UTexture2D* GetWeaponHUDReticle();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		FVector GetCalibrationVector(float Distance);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		int GetCurrentAmmo();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		int GetRemainingAmmo();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void SetupAim(UObject* TargetObject);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void ResetAim();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void SetFireMode(EFireMode NewFireMode);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void SetWeaponSpread(float Spread);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void ReloadComplete();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		bool IsTriggerActive();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "IWeapon")
		void GetWeaponCombatData(FWeaponCombatDataStruct& OutWeaponCombatData);
};
