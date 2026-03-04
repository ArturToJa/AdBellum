// Fill out your copyright notice in the Description page of Project Settings.


#include "RandomSpawnArea.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "IPlayer.h"
#include "Unit/BaseUnit.h"
#include "Interfaces/Selectable.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ARandomSpawnArea::ARandomSpawnArea()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
}

// Called when the game starts or when spawned
void ARandomSpawnArea::BeginPlay()
{
	Super::BeginPlay();
}

void ARandomSpawnArea::OnConstruction(const FTransform& Transform)
{
	InitializePlayerSpawnPoint(Transform);
}

FTransform ARandomSpawnArea::GetNextTransform()
{
	int RandomIndex = FMath::Rand() % SpawnTransforms.Num();
	FTransform NextTransform = SpawnTransforms[RandomIndex];
	NextTransform.SetLocation(NextTransform.GetLocation() + GetActorLocation());
	return NextTransform;
}

int ARandomSpawnArea::GetPriority(int TeamIndex)
{
	TArray<AActor*> OverlappingActors;
	BoxComponent->GetOverlappingActors(OverlappingActors, ABaseUnit::StaticClass());
	if (OverlappingActors.IsEmpty())
	{
		return 1;
	}
	else
	{
		int Priority = 0;

		for (AActor* Actor : OverlappingActors)
		{
			if (Actor->GetClass()->ImplementsInterface(UOwnershipInterface::StaticClass()))
			{
				int TeamId = IOwnershipInterface::Execute_GetTeamIndex(Actor);
				if (TeamId == TeamIndex)
				{
					Priority++;
				}
				else
				{
					Priority--;
				}
			}
		}
		return Priority;
	}
}

void ARandomSpawnArea::InitializePlayerSpawnPoint(const FTransform& Transform)
{
	FVector Scale = BoxComponent->GetScaledBoxExtent();
	FVector NewLocation = Transform.GetLocation() - UKismetMathLibrary::GetForwardVector(Transform.Rotator()) * Scale.Y;
	NewLocation.Z += 2000.0f;
	RTSCameraSpawnTransform.SetLocation(NewLocation);
	FRotator Rotation = Transform.GetRotation().Rotator();
	Rotation.Pitch = -45.0f;;
	RTSCameraSpawnTransform.SetRotation(Rotation.Quaternion());
}

void ARandomSpawnArea::GenerateTransforms(int NumberOfUnits)
{
}

FTransform ARandomSpawnArea::GetPlayerSpawnPoint()
{
	return RTSCameraSpawnTransform;
}