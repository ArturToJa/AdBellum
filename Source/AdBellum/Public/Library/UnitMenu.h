// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UnitMenu.generated.h"

UENUM(BlueprintType)
enum class BodyPartEnumSecond : uint8
{
	EMPTY UMETA(DisplayName = "EMPTY"),
	Head UMETA(DisplayName = "Head"),
	Body UMETA(DisplayName = "Body"),
	Legs UMETA(DisplayName = "Legs"),
	Boots UMETA(DisplayName = "Boots"),
	Hands UMETA(DisplayName = "Hands"),
	Helmet UMETA(DisplayName = "Helmet"),
	Vest UMETA(DisplayName = "Vest")
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FBodyPartIndexStructSecond
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		BodyPartEnumSecond bodyPart;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 mainIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 maxIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 materialIndex;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 maxMaterialIndex;

	FBodyPartIndexStructSecond();
	FBodyPartIndexStructSecond(BodyPartEnumSecond _bodyPart, int32 _mainIndex, int32 _maxIndex, int32 _materialIndex, int32 _maxMaterialIndex);
};

