// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Unit/SelectionData/InstancedStaticMeshManager.h"
#include "Interfaces/Customizable.h"
#include "AdBellumGameState.generated.h"

USTRUCT(BlueprintType)
struct ADBELLUM_API FWeaponDataStruct
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponSocketEnum WeaponSocketType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWeaponCustomizationDataStruct WeaponCustomizationData;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FUnitWeaponDataStruct
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* OwningUnit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWeaponDataStruct Weapon;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FUnitPrefabDataStruct
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMeshCreatorPrefabStruct UnitPrefab;
};

USTRUCT(BlueprintType)
struct ADBELLUM_API FUnitDataStruct
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FMeshCreatorPrefabStruct UnitPrefab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* ownedUnit;

	FUnitDataStruct()
	{
		ownedUnit = nullptr;
	}

	FUnitDataStruct(FMeshCreatorPrefabStruct Prefab, AActor* Unit)
	{
		UnitPrefab = Prefab;
		ownedUnit = Unit;
	}

	bool operator==(FUnitDataStruct Other)
	{
		return this->ownedUnit == Other.ownedUnit;
	}
};

/**
 * 
 */
UCLASS()
class ADBELLUM_API AAdBellumGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, NoClear, BlueprintReadOnly, Category = Default)
	TSubclassOf<AInstancedStaticMeshManager> InstancedStaticMeshManagerClass;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	AInstancedStaticMeshManager* GetInstancedStaticMeshManager();

	void SetSelectionCircle(bool Visible, AActor* Unit);

	//Unit creation
	void SetUnitPrefab(TArray<AActor*> Units, TArray<FMeshCreatorPrefabStruct> Prefabs);

	void OnWeaponCreated(TArray<AActor*> Weapons, TArray<FUnitWeaponDataStruct> WeaponPrefabs);
private:
	AInstancedStaticMeshManager* InstancedStaticMeshManager;
	
};
