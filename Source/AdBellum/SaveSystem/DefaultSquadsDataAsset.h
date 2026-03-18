// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Library/MeshCreatorParamsStruct.h"
#include "SquadSaveGame.h"	
#include "DefaultSquadsDataAsset.generated.h"



UCLASS()
class ADBELLUM_API UDefaultSquadsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FString, FUnitSaveData> DefaultSquads;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FString, FMeshCreatorPrefabStruct> DefaultUnits;
};
