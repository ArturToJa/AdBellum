// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "InteractablePassengerStruct.generated.h"


USTRUCT(BlueprintType)
struct ADBELLUM_API FSeat
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTransform PlaceTransform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		AActor* PlacedActor;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FSeatList
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FSeat> SeatList;
};


USTRUCT(BlueprintType)
struct ADBELLUM_API FSeatMap
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TMap<FName, FSeatList> SeatMap;
};


USTRUCT(BlueprintType)
struct ADBELLUM_API FSeatAccessPoint
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FName> Roles;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UActorComponent* DoorComponent;
};

