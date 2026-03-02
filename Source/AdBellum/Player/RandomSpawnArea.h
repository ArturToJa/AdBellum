// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseSpawnArea.h"
#include "Components/BoxComponent.h"
#include "RandomSpawnArea.generated.h"

UCLASS()
class ADBELLUM_API ARandomSpawnArea : public ABaseSpawnArea
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARandomSpawnArea();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void InitializePlayerSpawnPoint(const FTransform& Transform);
public:

	UPROPERTY(EditAnywhere, meta = (MakeEditWidget))
	TArray<FTransform> SpawnTransforms;

	virtual FTransform GetNextTransform() override;
	virtual int GetPriority(int TeamIndex) override;
	virtual void GenerateTransforms(int NumberOfUnits) override;
	virtual FTransform GetPlayerSpawnPoint() override;

	UPROPERTY(EditAnywhere, Category = "Default")
	FTransform RTSCameraSpawnTransform;
};
