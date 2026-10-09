// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IWeapon.h"
#include "Library/ALSADSInterface.h"
#include "Weapon/Attachments/BaseSight.h"
#include "Library/Weapon/WeaponCustomizationData.h"
#include "Library/NetworkComponent.h"
#include "EBBullet.h"
#include "RecoilAnimationComponent.h"
#include "Library/WeaponCombatData.h"
#include "BaseWeapon.generated.h"



class UEBBarrel;
//class UNetworkComponent;

UENUM(BlueprintType)
enum class ESightCalibrationMode : uint8
{
	Legacy		UMETA(DisplayName = "Legacy (Blueprint)"),
	Ballistic	UMETA(DisplayName = "Ballistic (C++)")
};

UCLASS()
class ADBELLUM_API ABaseWeapon : public AActor, public IIWeapon, public IALSADSInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseWeapon();

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNetworkComponent> NetworkComponent;

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	UEBBarrel* EBarrel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	AdBellumWeaponTypeEnum WeaponType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SX")
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SX")
	USoundBase* DryShotSound;

	UFUNCTION(Server, Reliable)
	void Server_PlayDryShotSound();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDryShotSound();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VX")
	UParticleSystem* MuzzleFlashParticleSystem;

	UPROPERTY(BlueprintReadWrite, Category = "VX")
	float EmitterScale = 0.3f;

	FVector EmitterScaleVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ADS")
	USceneComponent* ADS;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMeshComponent;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* Magazine;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* IRS;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Montages")
	UAnimMontage* ReloadStandMontage;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Montages")
	UAnimMontage* ReloadCrouchMontage;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Montages")
	UAnimMontage* ReloadManualMontage;

	//CUSTOMIZATION

	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	FDataTableRowHandle TexturePrefabData;

	UPROPERTY(EditDefaultsOnly, Category = "Customization")
	FDataTableRowHandle SightPrefabData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization|Materials")
	UMaterialInterface* BaseMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization|Materials")
	UMaterialInterface* ColorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization|Materials")
	UMaterialInterface* SkinMaterial;

	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = "Components")
	USightChildActorComponent* SightComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	FVector WeaponHeldRelativeLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization|Sight")
	float SightTargetDistance = 100.0f;

	// Legacy - blueprint calibration, Ballistic - C++ calibration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization|Sight")
	ESightCalibrationMode SightCalibrationMode = ESightCalibrationMode::Legacy;

	// EBarrel rotation relative to WeaponMeshComponent when not aimed by AI, used as reference for sight calibration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization|Sight")
	FRotator BaseBarrelRelativeRotation = FRotator(0.0f, 90.0f, 0.0f);

	// Bullet used for sight calibration: chambered bullet, otherwise first bullet in EBarrel ammo, otherwise nullptr
	UFUNCTION(BlueprintCallable, Category = "Customization|Sight")
	TSubclassOf<AEBBullet> GetCalibrationBulletClass() const;

	// Also correct sight yaw (for sights offset sideways from the barrel). Pitch is always corrected.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization|Sight")
	bool bCalibrateSightYaw = true;

	// Ballistic calibration: rotates the sight actor so the line of sight (ADS -> reticle) crosses
	// the bullet trajectory at Distance (meters). Does nothing unless SightCalibrationMode is Ballistic.
	// Local only: the rotation is applied to the non-replicated sight actor, never sent over the network.
	UFUNCTION(BlueprintCallable, Category = "Customization|Sight")
	bool CalibrateSight(float Distance);

	// Zero distance (meters) a freshly attached sight should use: sight's default from ZeroDistances,
	// or SightTargetDistance when the sight has no ZeroDistances configured
	UFUNCTION(BlueprintCallable, Category = "Customization|Sight")
	float GetDefaultSightZeroDistance() const;

	// Called after player changed sight zero distance (HUD, sound)
	UFUNCTION(BlueprintImplementableEvent, Category = "Customization|Sight")
	void OnSightZeroChanged(float NewDistance);

	// Called after player switched sight zoom level (HUD, sound)
	UFUNCTION(BlueprintImplementableEvent, Category = "Customization|Sight")
	void OnSightZoomChanged(float NewMagnification);

	// Plays one of the sight's adjust sounds for the player who made the change (immediately, no network
	// delay) and through the server for every other player near the weapon
	void PlaySightAdjustSound(ESightAdjustSound Sound);
	void PlaySightAdjustSoundLocal(ESightAdjustSound Sound);

	UFUNCTION(Server, Unreliable)
	void Server_PlaySightAdjustSound(ESightAdjustSound Sound);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySightAdjustSound(ESightAdjustSound Sound);

	// Simulates calibration bullet without collision in a level frame of the barrel (X downrange, Z up).
	// OutRangePoint is the trajectory point at DistanceMeters, relative to the barrel, in that frame.
	bool SimulateTrajectoryPoint(TSubclassOf<AEBBullet> BulletClass, float DistanceMeters, FVector& OutRangePoint,
		float& OutFlightTime, TArray<FVector>* OutTrajectory = nullptr) const;

	// EBarrel world transform with BaseBarrelRelativeRotation instead of its current (possibly AI aimed) rotation
	FTransform GetBaseBarrelWorldTransform() const;

	void DrawSightCalibrationDebug() const;

	// Re-evaluates everything that depends on who controls the owning unit and on the attached sight:
	// client side aim, first calibration of a new sight, debug/diagnostic hooks.
	// Runs on a slow timer; the owning unit also calls it right away when its controller changes.
	void RefreshOwnerDependentState();

	// Client side aim: while a player controls the owning unit, the server shoots from the barrel transform
	// reported by that player's machine (what the player sees through the sight) instead of its own pose.
	// AI controlled weapons keep using the server pose. Disable to always use the server pose.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrel")
	bool bUseClientSideAimForPlayers = true;

	void UpdateClientSideAim();

	// Calibrates a sight that has not been calibrated yet (new weapon, new sight actor) to its default
	// zero distance, as soon as a local player controls the unit. Zero keys keep working on top of it.
	void EnsureSightCalibrated();

	// Steps the sight's zoom transition and makes the owning unit apply the intermediate zoom.
	// Schedules itself for the next frame only while a transition is running.
	void StepSightZoomTransition();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float WeaponAimSensivity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	TObjectPtr<URecoilData> RecoilData;


	UFUNCTION(BlueprintCallable)
	float CalculateFlightTime(TSubclassOf<AEBBullet> BulletClass);


	UFUNCTION(BlueprintCallable)
	FVector CalculateSightRotation(FVector StartLocation, FVector TargetLocation, FVector TargetVelocity);

	UFUNCTION(BlueprintCallable)
	void PrepareWeapon(const FWeaponCustomizationDataStruct& WeaponPrefab);
	void PrepareMaterials(const FWeaponCustomizationDataStruct& WeaponPrefab);
	void PrepareSight(FName SightClassName);
	//END CUSTOMIZATION

	//SHOOTING LOGIC
	UFUNCTION(Server, Reliable)
	void Fire(bool Trigger);

	UFUNCTION()
	void HandleBarrelShotFired();

	bool bIsPlayerControlled;
	void SetbIsPlayerControlled(bool bIsPlayerControlled);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int MaxMagazineCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int MagazineSize;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int TotalAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK")
	bool ApplyHandIK = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IK")
	FName HandIKSocketName = "L_Hand_IK";

	UFUNCTION(Client, Reliable)
	void Client_AddRecoil();

	UFUNCTION(Client, Reliable)
	void Client_StopRecoil();

	void OnWeaponUpdate();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "AI")
	FWeaponCombatDataStruct WeaponCombatData;

	//END SHOOTING LOGIC
	//IIWeapon
	float Range = 500.0f;
	virtual UEBBarrel* GetEBarrel_Implementation() override;
	virtual void Trigger_Implementation(bool trigger) override;
	virtual AdBellumWeaponTypeEnum GetWeaponType_Implementation() override;
	virtual FVector getADSTarget_Implementation() override;
	virtual FRotator getADSCalibrationRotation_Implementation() override;
	virtual bool IsReloading_Implementation() override;
	virtual bool HasAmmoToReload_Implementation() override;
	virtual float GetEffectiveRange_Implementation() override;
	virtual float GetMaxRange_Implementation() override;
	virtual void Reload_Implementation() override;
	virtual bool HasAmmo_Implementation() override;
	virtual void BlockShooting_Implementation(bool Block) override;
	virtual void NotifyAim_Implementation(bool AimActive) override;
	virtual float GetCameraSensitivity_Implementation() override;
	virtual EWeaponSocketEnum GetWeaponSocket_Implementation() override;
	virtual void ConfigureWeapon_Implementation(const FWeaponCustomizationDataStruct& WeaponPrefab) override;
	virtual EFireMode GetFireMode_Implementation() override;
	virtual bool GetApplyHandIK_Implementation() override;
	virtual FTransform GetHandIKTransform_Implementation(ERelativeTransformSpace TransformSpace) override;
	virtual int GetCurrentAmmo_Implementation() override;
	virtual int GetRemainingAmmo_Implementation() override;
	virtual void ResetAim_Implementation() override;
	virtual void SetupAim_Implementation(UObject* TargetObject) override;
	virtual void SetFireMode_Implementation(EFireMode NewFireMode) override;
	virtual void SetWeaponSpread_Implementation(float Spread) override;
	virtual void ReloadComplete_Implementation() override;
	virtual bool IsTriggerActive_Implementation() override;
	virtual bool IsShooting_Implementation() override;
	virtual void GetWeaponCombatData_Implementation(FWeaponCombatDataStruct& OutWeaponCombatData) override;
	virtual float GetWeaponFOV_Implementation() override;
	virtual void SetSightMeshScale_Implementation(bool bIsAiming) override;
	virtual bool GetIsScoped_Implementation() override;
	virtual void ChangeSightZero_Implementation(int32 Direction) override;
	virtual float GetSightZeroDistance_Implementation() override;
	virtual void ChangeSightZoom_Implementation() override;
	virtual float GetSightMagnification_Implementation() override;

	virtual void BeginPlay() override;
	bool bIsReloading = false;
	void RefillAmmo();
	UFUNCTION(BlueprintCallable)
	bool GainMagazine();
protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	// Sight actor root relative rotation before any ballistic calibration, calibration is applied on top of it.
	// Captured again whenever the sight actor changes.
	FRotator BaseSightRelativeRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<ABaseSight> BaseRotationSight;

	// Last ballistic calibration result, kept for debug drawing (points in barrel range frame)
	float CalibratedDistance = 0.0f;
	FVector CalibrationZeroPoint = FVector::ZeroVector;
	TArray<FVector> CalibrationTrajectory;

	// Index of the current zero distance in sight's ZeroDistances, valid only for ZeroIndexSight
	int32 CurrentZeroIndex = INDEX_NONE;
	TWeakObjectPtr<ABaseSight> ZeroIndexSight;

	// Sight EnsureSightCalibrated already tried to calibrate
	TWeakObjectPtr<ABaseSight> CalibrationAttemptedSight;

	// Player controller whose aim the barrel currently uses, null when client side aim is off
	TWeakObjectPtr<AController> ClientSideAimController;

	// Optional sight diagnostics (ADS camera vs calibrated sight), implemented in SightDiagnostics.cpp.
	// Writes Saved/Logs/SightDiagnostics.csv while adb.Sight.DiagnosticsFile is 1 and the local player aims.
	void WriteSightDiagnostics(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	static bool IsSightDiagnosticsEnabled();
	FDelegateHandle SightDiagnosticsHandle;

	// Slow polling of RefreshOwnerDependentState
	FTimerHandle OwnerStateTimer;
	// Per frame work exists only while it is needed
	bool bZoomTransitionStepScheduled = false;
	bool bDebugDrawStepScheduled = false;
	void UpdateDebugHooks();
	void StepDebugDraw();

	float LastShotWorldTime = -1000.0f;
	float SightDiagnosticsTimer = 0.0f;
	TWeakObjectPtr<ABaseSight> SightDiagnosticsLoggedSight;
};
