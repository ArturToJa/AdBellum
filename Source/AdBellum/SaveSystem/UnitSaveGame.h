// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Library/MeshCreatorParamsStruct.h"
#include "UnitSaveGame.generated.h"



UCLASS()
class ADBELLUM_API UUnitSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Units")
	TMap<FString, FMeshCreatorPrefabStruct> UnitPrefabs;
};
