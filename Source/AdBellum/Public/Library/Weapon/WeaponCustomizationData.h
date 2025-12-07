// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PaintModeEnum.h"
#include "Math/Color.h"
#include "Engine/DataTable.h"
#include "Weapon/Attachments/BaseSight.h"
#include "WeaponCustomizationData.generated.h"

USTRUCT(BlueprintType)
struct ADBELLUM_API FWeaponCustomizationDataStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint")
	EWeaponPaintMode PaintMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Texture")
	FName SkinTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Texture")
	float SkinScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Texture")
	float SkinRotation = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Paint|Color")
	FLinearColor ColorValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sight")
	FName SightClass;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FTexturePrefabData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture> SkinTexture;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FSightPrefabData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<ABaseSight> SightClass;
};