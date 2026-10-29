// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EBBarrel.h"
#include <NiagaraSystem.h>
#include "IWeapon.h"
#include "Library/ALSADSInterface.h"
#include "SimpleWeapon.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;

// Weapon mounted on a vehicle (or any other actor) through UWeaponChildActorComponent. Configured in blueprints:
// place the barrel, optionally fill one of the meshes, place ADS where the gunner's eye is. The gunner looks
// from ADS along its forward axis with a reticle in the middle of the screen, zeroing turns ADS so that axis
// crosses the bullet trajectory at the zero distance.
UCLASS()
class ADBELLUM_API ASimpleWeapon : public AActor, public IIWeapon, public IALSADSInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASimpleWeapon();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WeaponRoot;

	// Optional visuals, leave the mesh empty when the weapon is only a barrel on the vehicle model
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	UEBBarrel* EBarrel;

	// Gunner's eye: location and (after zeroing) rotation of the ADS camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ADS")
	TObjectPtr<USceneComponent> ADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	USoundBase* FireSX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	UNiagaraSystem* FireVX;

	//UFUNCTION(NetMulticast, Reliable)
	//void Multicast_PlayEffects();

	//UFUNCTION()
	//void HandleBarrelShotFired();

	// Zero distances (meters) the gunner can step through with ScopeZeroUp/Down. Keep them sorted ascending.
	// Empty - ADS is never turned, it looks along the direction it was placed in.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ADS|Calibration")
	TArray<float> ZeroDistances;

	// Index in ZeroDistances the weapon is zeroed at by default
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ADS|Calibration", meta = (ClampMin = "0"))
	int32 DefaultZeroDistanceIndex = 0;

	// Also correct ADS yaw (for ADS placed sideways from the barrel). Pitch is always corrected.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ADS|Calibration")
	bool bCalibrateADSYaw = true;

	// Camera FOV while the gunner holds the aim button with this weapon
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ADS", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float ADSFOV = 45.0f;

	// Grip sockets for the gunner's hand IK, looked up on SkeletalMesh first, then on StaticMesh
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HandIK")
	FName LeftHandSocket = TEXT("L_HandIK");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HandIK")
	FName RightHandSocket = TEXT("R_HandIK");

	// World transforms of the grip sockets. False when neither mesh has both sockets.
	UFUNCTION(BlueprintCallable, Category = "HandIK")
	bool GetHandIKTargets(FTransform& OutLeft, FTransform& OutRight) const;

	// Turns ADS so its forward axis crosses the bullet trajectory at Distance (meters).
	// Local only: every machine keeps its own ADS rotation, only the gunner looking through it needs it.
	UFUNCTION(BlueprintCallable, Category = "ADS|Calibration")
	bool CalibrateADS(float Distance);

	// Called after the gunner changed zero distance (HUD, sound)
	UFUNCTION(BlueprintImplementableEvent, Category = "ADS|Calibration")
	void OnADSZeroChanged(float NewDistance);

	// While a player controls the owning unit, the server shoots from the barrel transform reported by that
	// player's machine instead of its own pose. AI controlled weapons keep using the server pose.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	bool bUseClientSideAimForPlayers = true;

	virtual FVector GetCalibrationVector_Implementation(float Distance) override;

	UFUNCTION(Server, Reliable)
	void Fire(bool Trigger);

	virtual void Trigger_Implementation(bool trigger) override;

	virtual UEBBarrel* GetEBarrel_Implementation() override;

	virtual void ChangeSightZero_Implementation(int32 Direction) override;

	virtual float GetSightZeroDistance_Implementation() override;

	virtual float GetWeaponFOV_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
	UTexture2D* WeaponReticleTexture;

	virtual UTexture2D* GetWeaponHUDReticle_Implementation() override;

	//ADS Interface
	virtual FVector getADSTarget_Implementation() override;

	virtual FRotator getADSRotation_Implementation() override;

private:
	// Catches late arriving state (owner, its controller, replicated bullet class) on a slow timer
	void RefreshOwnerDependentState();

	void UpdateClientSideAim();

	// Calibrates to the default zero distance once, as soon as a local player controls the owning unit
	void EnsureADSCalibrated();

	FTimerHandle OwnerStateTimer;

	// ADS rotation as placed in the blueprint, calibration always starts from it
	FRotator BaseADSRelativeRotation = FRotator::ZeroRotator;
	bool bHasBaseADSRotation = false;

	bool bCalibrationAttempted = false;
	int32 CurrentZeroIndex = INDEX_NONE;
	float CalibratedDistance = 0.0f;

	// Player controller whose aim the barrel currently uses, null when client side aim is off
	TWeakObjectPtr<AController> ClientSideAimController;
};
