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

	// HE (high explosive): detonates on impact, dealing radial damage instead of DealDamage's single-actor hit
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion")
	bool bIsHE = false;

	// Preferred explosion VFX, spawned instead of ExplosionVFX_Legacy when set
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	TObjectPtr<class UNiagaraSystem> ExplosionVFX;

	// Explosion VFX used while ExplosionVFX is not set
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	TObjectPtr<class UParticleSystem> ExplosionVFX_Legacy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	FVector ExplosionVFXScale = FVector(0.7f, 0.7f, 0.7f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	TObjectPtr<class USoundBase> ExplosionSX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	float ExplosionSXVolumeScale = 1.0f;

	// Radial damage on detonation, see UGameplayStatics::ApplyRadialDamageWithFalloff
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	float ExplosionBaseDamage = -5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	float ExplosionMinimumDamage = -1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	float ExplosionDamageInnerRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	float ExplosionDamageOuterRadius = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	float ExplosionDamageFalloff = 0.33f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "bIsHE"))
	TSubclassOf<class UDamageType> ExplosionDamageType;

	UFUNCTION(Server, Reliable)
	void Server_PlayExplosionEffects(FVector Location);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayExplosionEffects(FVector Location);

	void PlayExplosionEffects(FVector Location);

	virtual void Tick(float DeltaSeconds) override;

	// DebugEnabled as this bullet should currently use it: the console command adb.Ballistics.Debug
	// (true / false) overrides the class default, "default" hands control back to the class default
	UFUNCTION(BlueprintPure, Category = "Debug")
	bool IsBallisticsDebugEnabled() const;
};
