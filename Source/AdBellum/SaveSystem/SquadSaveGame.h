// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Library/MeshCreatorParamsStruct.h"
#include "SquadSaveGame.generated.h"



UCLASS()
class ADBELLUM_API UUnitSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Units")
	TMap<FString, FMeshCreatorPrefabStruct> UnitPrefabs;
};

USTRUCT(BlueprintType)
struct FUnitSaveData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Units")
	TArray<FString> UnitPrefabNames;
};

UCLASS()
class ADBELLUM_API USquadSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Squads")
	TMap<FString, FUnitSaveData> SquadPrefabs;
};
