// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Library/MeshCreatorParamsStruct.h"
#include "Library/MeshCreatorOutput.h"
#include "Customizable.generated.h"

UINTERFACE(MinimalAPI)
class UCustomizable : public UInterface
{
	GENERATED_BODY()
};

class ADBELLUM_API ICustomizable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Customizable")
	void GetSkeletalMeshFromDefinition(const EBodyPart BodyPart, UPARAM(ref) const FMeshCreatorDataInputStruct& MeshDefinition, FMeshCreatorOutputStruct& Output) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Customizable")
	void ConfigureUnit(const FMeshCreatorPrefabStruct& UnitPrefab);
	UFUNCTION(BlueprintNativeEvent, Category = "Customizable")
	void ConfigureWeapon(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum);
};