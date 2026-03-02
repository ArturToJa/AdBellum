// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseSpawnArea.h"

// Sets default values
ABaseSpawnArea::ABaseSpawnArea()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetRelativeScale3D(FVector(50.0f, 37.0f, 1.0f));
	BoxComponent->SetupAttachment(RootComponent);
}

FTransform ABaseSpawnArea::GetNextTransform()
{
	return FTransform();
}

int ABaseSpawnArea::GetPriority(int TeamIndex)
{
	return true;
}

void ABaseSpawnArea::GenerateTransforms(int NumberOfUnits)
{
}

FTransform ABaseSpawnArea::GetPlayerSpawnPoint()
{
	return FTransform();
}