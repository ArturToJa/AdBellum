// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/BaseSpawnArea.h"
#include "Components/BoxComponent.h"
#include "SpawnArea.generated.h"

UCLASS()
class ADBELLUM_API ASpawnArea : public ABaseSpawnArea
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnArea();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

public:

	UPROPERTY(EditAnywhere, Category = "Default")
	int TeamId;

	virtual FTransform GetNextTransform() override;
	virtual int GetPriority(int TeamIndex) override;
	virtual void GenerateTransforms(int NumberOfUnits) override;
	virtual FTransform GetPlayerSpawnPoint() override;

	UPROPERTY(EditAnywhere, Category = "Default")
	FVector TopLeftCorner;
	UPROPERTY(EditAnywhere, Category = "Default")
	FVector TopRightCorner;
	UPROPERTY(EditAnywhere, Category = "Default")
	FVector BottomLeftCorner;
	UPROPERTY(EditAnywhere, Category = "Default")
	FTransform RTSCameraSpawnTransform;

private:
	void InitializePlayerSpawnPoint(const FTransform& Transform);
	void GenerateCorners(const FTransform& Transform);

	TArray<FTransform> SpawnTransforms;
	int UserTransformsCounter;
};
