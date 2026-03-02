// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeshCreatorOutput.generated.h"

USTRUCT(BlueprintType)
struct ADBELLUM_API FMeshCreatorOutputStruct
{
	GENERATED_USTRUCT_BODY();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
		float ArmourParamValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
		USkeletalMesh* OutputMesh;
};