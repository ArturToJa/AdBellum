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
#include "BaseWeapon.generated.h"


class UEBBarrel;
//class UNetworkComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float WeaponAimSensivity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	TObjectPtr<URecoilData> RecoilData;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	//float RecoilVerticalValue = 0.075f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	//float RecoilHorizontalValue = 0.033f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	//TSubclassOf<class UCameraShakeBase> ShakeClass = nullptr;

	float CalculateFlightTime(TSubclassOf<AEBBullet> BulletClass);

	void CalibrateSight();

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

	//END SHOOTING LOGIC
	//IIWeapon
	float Range = 500.0f;
	virtual UEBBarrel* GetEBarrel_Implementation() override;
	virtual void Trigger_Implementation(bool trigger) override;
	virtual AdBellumWeaponTypeEnum GetWeaponType_Implementation() override;
	virtual FVector getADSTarget_Implementation() override;
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

	virtual void BeginPlay() override;
	bool bIsReloading = false;
	void RefillAmmo();
	UFUNCTION(BlueprintCallable)
	bool GainMagazine();
protected:
	virtual void OnConstruction(const FTransform& Transform) override;
};
