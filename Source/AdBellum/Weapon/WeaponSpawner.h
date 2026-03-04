// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseWeapon.h"
#include "Interfaces/Customizable.h"
#include "WeaponSpawner.generated.h"

UCLASS()
class ADBELLUM_API AWeaponSpawner : public AActor, public ICustomizable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void ConfigureWeapon_Implementation(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum) override;

	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TArray<FWeaponPrefabDataStruct> WeaponPrefabs;

	ABaseWeapon* SpawnedWeapon = nullptr;

	float SpawnTime = 5.f;
	float CurrentSpawnTime = 5.f;
};
