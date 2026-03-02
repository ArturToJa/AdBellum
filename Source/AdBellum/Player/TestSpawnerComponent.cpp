// Fill out your copyright notice in the Description page of Project Settings.


#include "TestSpawnerComponent.h"
#include "IPlayer.h"
#include "Character/ALSBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "System/AdBellumGameMode.h"
#include "NavigationSystem.h"
#include "Unit/BaseUnit.h"

UTestSpawnerComponent::UTestSpawnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

TArray<AActor*> UTestSpawnerComponent::SpawnTestUnits()
{
	TArray<AActor*> SpawnedUnits;

	if (GetOwner()->GetClass()->ImplementsInterface(UIPlayer::StaticClass()))
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		// check that we have a nav system
		if (NavSys)
		{
			AAdBellumGameMode* AdGameMode = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

			for (const FTestSpawn& TestSpawn : TestSpawnData)
			{
				for (int i = 0; i < TestSpawn.NumberOfActors; ++i)
				{
					FVector startPosi = TestSpawn.TargetTransform.GetLocation();
					FTransform SpawnTransform;
					SpawnTransform.SetLocation(startPosi + i * 300);
					SpawnTransform.SetRotation(TestSpawn.TargetTransform.GetRotation());
					APawn* SpawnedUnit = GetWorld()->SpawnActorDeferred<APawn>(TestSpawn.UnitClass, SpawnTransform);
					if (SpawnedUnit)
					{
						if (ABaseUnit* SpawnedBaseUnit = Cast<ABaseUnit>(SpawnedUnit))
						{
							//SpawnedBaseUnit->PrefabName = TestSpawn.PrefabName;
						}
						SpawnedUnit->SetOwner(GetOwner());
						SpawnedUnit->FinishSpawning(SpawnTransform);
						//AdGameMode->AddUnitForPlayer(SpawnedUnit, GetOwner());
						SpawnedUnits.Add(SpawnedUnit);
					}
				}
			}
		}
	}
	return SpawnedUnits;
}
