// Fill out your copyright notice in the Description page of Project Settings.


#include "UnitSpawner.h"
#include "System/AdBellumGameMode.h"


// Sets default values
AUnitSpawner::AUnitSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AUnitSpawner::BeginPlay()
{
	Super::BeginPlay();
	if(HasAuthority())
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AUnitSpawner::SpawnUnit, SpawnTime, false, SpawnTime);
	}
	
}

// Called every frame
void AUnitSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AUnitSpawner::SpawnUnit()
{
	if (UnitPrefabs.Num() == 0)
	{
		return;
	}

	int32 RandomIndex = FMath::RandRange(0, UnitPrefabs.Num() - 1);
	TArray<FMeshCreatorPrefabStruct> SelectedPrefabs;
	SelectedPrefabs.Add(UnitPrefabs[RandomIndex]);
	// Spawn the unit
	AAdBellumGameMode* GameMode = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	check(GameMode);
	GameMode->SpawnFormationForPlayer(GameMode->PlayersData[0][0].playerReference, 0, 0, SelectedPrefabs);
}
