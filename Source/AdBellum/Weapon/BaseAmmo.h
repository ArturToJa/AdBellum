// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EBBullet.h"
#include "Library/AdBellumEnumLibrary.h"
#include "Library/ImpactEffectDataStruct.h"
#include "BaseAmmo.generated.h"

/**
 * 
 */
UCLASS()
class ADBELLUM_API ABaseAmmo : public AEBBullet
{
	GENERATED_BODY()

public:
	ABaseAmmo(const FObjectInitializer& ObjectInitializer);

	//UFUNCTION(BlueprintAuthorityOnly, BlueprintNativeEvent, Category = "EBBullet|Impact")
	virtual void OnImpact_Implementation(bool Ricochet, bool PassedThrough, FVector Location, FVector IncomingVelocity, 
		FVector Normal, FVector ExitLocation, FVector ExitVelocity, FVector Impulse, float PenetrationDepth, 
		AActor* Actor, USceneComponent* Component, FName BoneName, UPhysicalMaterial* PhysMaterial, FHitResult HitResult) override;

	void DealDamage(AActor* DamagedActor, FHitResult HitResult, FVector IncomingVelocity);

	EBodyPart FindBodyPart(FName BoneName);

	void DealHumanDamage(FVector IncomingVelocity, EBodyPart BodyPart, AActor* DamagedActor);

	void PlayEffects(EPhysicalSurface HitSurfaceEnum, FVector Location, FVector IncomingVelocity);

	UFUNCTION(Server, Reliable)
	void Server_PlayEffects(EPhysicalSurface HitSurfaceEnum, FVector Location, FVector IncomingVelocity);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEffects(EPhysicalSurface HitSurfaceEnum, FVector Location, FVector IncomingVelocity);

	float CalculateEnergy(FVector CurrentVelocity);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FImpactEffectLibrary EffectsLibrary;

	FVector EmitterBaseScale = FVector(1.0f, 1.0f, 1.0f);

	void PrintDebugInfo(EBodyPart BodyPart, float BodyPartMultiplier, float ArmourMultiplier, FVector IncomingVelocity);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool SpawnDecal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterial* DecalMaterial;
};
