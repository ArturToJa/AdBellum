// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FormationInterface.generated.h"

class ABaseSpawnArea;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFormationInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IFormationInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	TMap<AActor*, int> GetEnemiesInSight();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	TMap<AActor*, FVector> GetLastKnownPosition();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	TArray<APawn*> GetUnitsInFormation();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	void AddUnitToFormation(APawn* UnitToAdd);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	void OnEnemyInSightChanged(AActor* EnemyUnit, bool IsVisible);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	void OnNoiseHeard(AActor* NoiseSource);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	int GetFormationCost();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	void RespawnFormation(ABaseSpawnArea* SpawnArea);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FormationInterface")
	AActor* GetFirstActorOfInterest();
};
