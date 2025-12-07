// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EBBarrel.h"
#include <NiagaraSystem.h>
#include "IWeapon.h"
#include "SimpleWeapon.generated.h"


UCLASS()
class ADBELLUM_API ASimpleWeapon : public AActor, public IIWeapon
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASimpleWeapon();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	UEBBarrel* EBarrel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* FireSX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* FireVX;

	//UFUNCTION(NetMulticast, Reliable)
	//void Multicast_PlayEffects();

	//UFUNCTION()
	//void HandleBarrelShotFired();

	virtual FVector GetCalibrationVector_Implementation(float Distance) override;

	UFUNCTION(Server, Reliable)
	void Fire(bool Trigger);

	virtual void Trigger_Implementation(bool trigger) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	UTexture2D* WeaponReticleTexture;

	virtual UTexture2D* GetWeaponHUDReticle_Implementation() override;
};