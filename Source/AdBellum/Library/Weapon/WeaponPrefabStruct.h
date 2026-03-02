// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaintModeEnum.h"
#include "Math/Color.h"
#include "WeaponSocketEnum.h"
#include "Engine/DataTable.h"
#include "Weapon/BaseWeapon.h"
#include "WeaponCustomizationData.h"
#include "WeaponPrefabStruct.generated.h"

USTRUCT(BlueprintType)
struct ADBELLUM_API FWeaponPrefabStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponSocketEnum WeaponSocketType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ABaseWeapon> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint")
	EWeaponPaintMode PaintMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Texture")
	UTexture* SkinTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Texture")
	float SkinScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Texture")
	float SkinRotation = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Color")
	FLinearColor ColorValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
	TSubclassOf<class ABaseSight> SightClass;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FWeaponPrefabDataStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponSocketEnum WeaponSocketType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	FWeaponCustomizationDataStruct WeaponCustomizationData;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FWeaponPrefabData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<ABaseWeapon> WeaponClass;
};