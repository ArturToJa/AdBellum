// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Library/AdBellumEnumLibrary.h"
#include "Library/Weapon/WeaponPrefabStruct.h"
#include "Engine/DataTable.h"
#include "UnitCombatData.h"
#include "MeshCreatorParamsStruct.generated.h"

USTRUCT(BlueprintType)
struct FMeshCreatorDataInputStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	EBodyPart BodyPart;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
		int32 id;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
		int32 materialId;
};


USTRUCT(BlueprintType)
struct FMeshCreatorPrefabStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TSoftClassPtr<APawn> UnitClass;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TArray<FMeshCreatorDataInputStruct> PrefabDefinition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TArray<FMeshCreatorDataInputStruct> PrefabHeadDefinition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TArray<TSoftObjectPtr<USkeletalMesh>> AdditionalMeshes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TArray<TSoftObjectPtr<USkeletalMesh>> AdditionalHeadMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	TArray<FWeaponPrefabDataStruct> WeaponPrefabData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	FUnitCombatDataStruct UnitCombatData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	int TicketCost = 1;
};

USTRUCT(BlueprintType)
struct FMeshMaterialCombinationStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TArray<TSoftObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	float ArmourValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	int MeshMaterialApplyIndex = -1;
};

USTRUCT(BlueprintType)
struct FMeshMaterialCombinationArrayStruct : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MeshCreator")
	TArray<FMeshMaterialCombinationStruct> MeshMaterialArray;
};
