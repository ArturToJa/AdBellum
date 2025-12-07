// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Unit/BaseUnit.h"
#include "UnitSpawner.generated.h"

UCLASS()
class ADBELLUM_API AUnitSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUnitSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Unit")
	TArray<FMeshCreatorPrefabStruct> UnitPrefabs;
	ABaseUnit* SpawnedUnit = nullptr;

	void SpawnUnit();

	float SpawnTime = 5.f;
	FTimerHandle SpawnTimerHandle;
};
