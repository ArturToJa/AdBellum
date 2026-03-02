// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "BaseSpawnArea.generated.h"

UCLASS()
class ADBELLUM_API ABaseSpawnArea : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseSpawnArea();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UBoxComponent> BoxComponent;

	virtual FTransform GetNextTransform();
	virtual int GetPriority(int TeamIndex);
	virtual void GenerateTransforms(int NumberOfUnits);
	virtual FTransform GetPlayerSpawnPoint();

private:

	TArray<FTransform> SpawnTransforms;
};
