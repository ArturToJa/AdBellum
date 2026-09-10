// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DefaultSquadsDataAsset.h"
#include "SquadSaveGame.h"
#include "SaveSubsystem.generated.h"


UCLASS(Blueprintable)
class ADBELLUM_API USaveSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UDefaultSquadsDataAsset* DefaultSquadsDataAsset;
private:
	UPROPERTY()
	UUnitSaveGame* UnitSaveGame;
	UPROPERTY()
	USquadSaveGame* SquadSaveGame;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void SaveSquadDataAsset(const FString& SquadName, const FUnitSaveData& PrefabDataArray, bool bOverwrite = false);

	UFUNCTION(BlueprintCallable)
	void SaveUnitDataAsset(const FString& UnitName, const FMeshCreatorPrefabStruct& PrefabData, bool bOverwrite = false);

	// Squad related functions
	UFUNCTION(BlueprintCallable)
	void SaveSquads();

	UFUNCTION(BlueprintCallable)
	void SaveSquadPrefab(const FString& SquadName, const FUnitSaveData& PrefabDataArray, bool bOverwrite = false);

	UFUNCTION(BlueprintCallable)
	void RemoveSquadPrefab(const FString& SquadName);

	UFUNCTION(BlueprintCallable)
	bool DoesSquadPrefabExist(const FString& SquadName) const;

	UFUNCTION(BlueprintCallable)
	TMap<FString, FUnitSaveData>& GetSaveSquadPrefabs();

	UFUNCTION(BlueprintCallable)
	TMap<FString, FUnitSaveData>& GetDefaultSquadPrefabs();

	UFUNCTION(BlueprintCallable)
	FUnitSaveData& GetSquadPrefab(const FString& SquadName);

	// Unit related functions
	UFUNCTION(BlueprintCallable)
	void SaveUnits();

	UFUNCTION(BlueprintCallable)
	void SaveUnitPrefab(const FString& UnitName, const FMeshCreatorPrefabStruct& PrefabData, bool bOverwrite = false);

	UFUNCTION(BlueprintCallable)
	void RemoveUnitPrefab(const FString& UnitName);

	UFUNCTION(BlueprintCallable)
	bool DoesUnitPrefabExist(const FString& UnitName) const;

	UFUNCTION(BlueprintCallable)
	TMap<FString, FMeshCreatorPrefabStruct>& GetSaveUnitPrefabs();

	UFUNCTION(BlueprintCallable)
	TMap<FString, FMeshCreatorPrefabStruct>& GetDefaultUnitPrefabs();

	UFUNCTION(BlueprintCallable)
	FMeshCreatorPrefabStruct& GetUnitPrefab(const FString& UnitName);
private:
	void LoadOrCreateSquadSave();
	void LoadOrCreateUnitSave();
	void ValidateSquadPrefabData() const;
};
