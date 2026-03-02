// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponSpawner.h"
#include "System/AdBellumGameMode.h"

// Sets default values
AWeaponSpawner::AWeaponSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWeaponSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (HasAuthority())
	{
		CurrentSpawnTime -= DeltaTime;
		if (CurrentSpawnTime <= 0.f)
		{
			CurrentSpawnTime = SpawnTime;
			if (SpawnedWeapon)
			{
				SpawnedWeapon->Destroy();
			}
			if (WeaponPrefabs.Num() > 0)
			{
				int32 RandomIndex = FMath::RandRange(0, WeaponPrefabs.Num() - 1);

				TArray<FWeaponPrefabDataStruct> WeaponPrefabToSpawn;
				WeaponPrefabToSpawn.Add(WeaponPrefabs[RandomIndex]);

				AAdBellumGameMode* GameMode = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
				check(GameMode);
				GameMode->SpawnWeaponsForUnit(this, WeaponPrefabToSpawn);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No weapon classes available to spawn."));
			}
		}
	}
}

void AWeaponSpawner::ConfigureWeapon_Implementation(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum)
{
	if (HasAuthority())
	{
		SpawnedWeapon = Weapon;
		Weapon->SetActorTransform(GetActorTransform());
	}
}