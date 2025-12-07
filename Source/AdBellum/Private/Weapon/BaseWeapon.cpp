// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BaseWeapon.h"
#include "EBBarrel.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/ALSInputInterface.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Perception/AISense_Hearing.h"
#include "Net/UnrealNetwork.h"
#include "OrderSystem/Orderable.h"
#include "ITargetable.h"

// Sets default values
ABaseWeapon::ABaseWeapon()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));

	EBarrel = CreateDefaultSubobject<UEBBarrel>(TEXT("EBarrel"));
	EBarrel->ShotFired.AddDynamic(this, &ABaseWeapon::HandleBarrelShotFired);

	ADS = CreateDefaultSubobject<USceneComponent>(TEXT("ADS"));

	WeaponMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMeshComponent"));
	WeaponMeshComponent->SetupAttachment(RootComponent);

	EBarrel->AttachToComponent(WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	ADS->AttachToComponent(WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);

	Magazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Magazine Mesh"));
	Magazine->SetupAttachment(WeaponMeshComponent, "Mag");

	IRS = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("IRS"));
	IRS->SetupAttachment(WeaponMeshComponent, "IRS");

	EmitterScaleVector = UKismetMathLibrary::MakeVector(EmitterScale, EmitterScale, EmitterScale);
	SightComponent = CreateDefaultSubobject<USightChildActorComponent>(TEXT("SightComponent"));
	bReplicates = true;
	bAlwaysRelevant = true;
	ReplicatedComponents.Add(EBarrel);
}

void ABaseWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWeapon, EBarrel);
	DOREPLIFETIME(ABaseWeapon, SightComponent);
	DOREPLIFETIME(ABaseWeapon, TotalAmmo);
}

void ABaseWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}
// Called when the game starts or when spawned
void ABaseWeapon::BeginPlay()
{
	Super::BeginPlay();
}

//CUSTOMIZATION

void ABaseWeapon::PrepareWeapon(const FWeaponCustomizationDataStruct& WeaponPrefab)
{
	PrepareMaterials(WeaponPrefab);
	PrepareSight(WeaponPrefab.SightClass);
	CalibrateSight();
}

void ABaseWeapon::PrepareMaterials(const FWeaponCustomizationDataStruct& WeaponPrefab)
{
	switch (WeaponPrefab.PaintMode)
	{
	case EWeaponPaintMode::EMPTY: 
		{
		if (BaseMaterial) 
			{
				WeaponMeshComponent->SetMaterial(0, BaseMaterial);
			}
		break;
		}
	case EWeaponPaintMode::COLOR: 
		{
		if (ColorMaterial)
			{
				UMaterialInstanceDynamic* NewColorMaterial = WeaponMeshComponent->CreateDynamicMaterialInstance(0, ColorMaterial);
				WeaponMeshComponent->SetMaterial(0, NewColorMaterial);
				NewColorMaterial->SetVectorParameterValue("SkinColor", WeaponPrefab.ColorValue);
			}
		break;
		}

	case EWeaponPaintMode::TEXTURE: 
		{
		if (SkinMaterial && !WeaponPrefab.SkinTexture.IsNone())
			{
				UMaterialInstanceDynamic* NewTextureMaterial = WeaponMeshComponent->CreateDynamicMaterialInstance(0, SkinMaterial);
				WeaponMeshComponent->SetMaterial(0, NewTextureMaterial);
				TexturePrefabData.RowName = WeaponPrefab.SkinTexture;
				if (FTexturePrefabData* TextureData = TexturePrefabData.GetRow<FTexturePrefabData>("Weapon Customization Texture"))
				{
					UTexture* SkinTexture = TextureData->SkinTexture.LoadSynchronous();
					NewTextureMaterial->SetTextureParameterValue("SkinTexture", SkinTexture);
				}
				NewTextureMaterial->SetScalarParameterValue("Scale", WeaponPrefab.SkinScale);
				NewTextureMaterial->SetScalarParameterValue("Rotate", WeaponPrefab.SkinRotation);
			}
		break;
		}
	}
}

void ABaseWeapon::PrepareSight(FName SightClassName)
{
	if (!SightClassName.IsNone())
	{
		SightPrefabData.RowName = SightClassName;
		if (FSightPrefabData* SightData = SightPrefabData.GetRow<FSightPrefabData>("Weapon Customization Sight"))
		{
			TSubclassOf<ABaseSight> SightClass = SightData->SightClass.LoadSynchronous();
			SightComponent->SetChildActorClass(SightClass);
			if (SightComponent->GetSightActor())
			{
				SightComponent->AttachToComponent(WeaponMeshComponent,
					FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, false),
					SightComponent->GetSightActor()->SocketName);
				IRS->SetVisibility(false);
			}
		}
	}
	else if (IRS->GetStaticMesh() != nullptr) 
	{
		IRS->SetVisibility(true);
	}
}

void ABaseWeapon::CalibrateSight()
{
	if (SightComponent->GetChildActor())
	{
		FVector HitLocation = EBarrel->GetRelativeTransform().GetUnitAxis(EAxis::X) * 10000.0f + EBarrel->GetRelativeLocation();
		SightComponent->CalibrateSightActor(HitLocation);
	}
}

float ABaseWeapon::CalculateFlightTime(TSubclassOf<AEBBullet> BulletClass)
{
	if (BulletClass)
	{
		AEBBullet* DefaultBullet = BulletClass.GetDefaultObject();

		// Access the MuzzleVelocityMin property value
		//UKismetSystemLibrary::PrintString(GetWorld(), FString::SanitizeFloat((SightTargetDistance * 100) / DefaultBullet->MuzzleVelocityMin).Append("<--- Calculated flight time "), true, true);
		return (SightTargetDistance * 100)/ DefaultBullet->MuzzleVelocityMin;
	}
	else return 0.0f;
}
//END CUSTOMIZATION

//SHOOTING LOGIC
void ABaseWeapon::HandleBarrelShotFired() 
{
	if (FireSound) 
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, EBarrel->GetComponentLocation());
	}
	if (MuzzleFlashParticleSystem)
	{	
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlashParticleSystem, EBarrel,
			FName(),
			EBarrel->GetComponentLocation(), 
			EBarrel->GetComponentRotation(), 
			EmitterScaleVector, 
			EAttachLocation::KeepWorldPosition, true, EPSCPoolMethod::None, true);
	}

	UAISense_Hearing::ReportNoiseEvent(GetWorld(), EBarrel->GetComponentLocation(), 1.f, GetOwner());

	if (GetOwner())
	{
		Client_AddRecoil();

		if (!(EBarrel->GetAmmoCount(true) > 0))
		{
			Client_StopRecoil();
		}
		OnWeaponUpdate();
	}
}

UEBBarrel* ABaseWeapon::GetEBarrel_Implementation()
{
	return EBarrel;
}

void ABaseWeapon::Client_AddRecoil_Implementation()
{
	IALSInputInterface::Execute_AddRecoil(GetOwner());
}

void ABaseWeapon::Client_StopRecoil_Implementation()
{
	IALSInputInterface::Execute_StopRecoil(GetOwner());
}

void ABaseWeapon::OnWeaponUpdate()
{
	IOrderable::Execute_OnWeaponUpdated(GetOwner(), this);
}

FVector ABaseWeapon::getADSTarget_Implementation()
{
	if (SightComponent->GetSightActor())
	{
		return SightComponent->GetSightActor()->ADS->GetComponentLocation();
	}
	else 
	{ 
		return ADS->GetComponentLocation(); 
	}
}

//END SHOOTING LOGIC
//IWeapon
void ABaseWeapon::Trigger_Implementation(bool trigger)
{
	Fire(trigger);
}

void ABaseWeapon::Fire_Implementation(bool trigger)
{
	EBarrel->Shoot(trigger);
	//if (EBarrel->GetAmmoCount(true) == 0) 
	//{
	//	Server_PlayDryShotSound();
	//}
}

void ABaseWeapon::Server_PlayDryShotSound_Implementation() 
{
	Multicast_PlayDryShotSound();
}

void ABaseWeapon::Multicast_PlayDryShotSound_Implementation() 
{
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), DryShotSound, GetActorLocation());
}

void ABaseWeapon::BlockShooting_Implementation(bool Block)
{
	EBarrel->Shoot(!Block);
}

AdBellumWeaponTypeEnum ABaseWeapon::GetWeaponType_Implementation()
{
	return WeaponType;
}

bool ABaseWeapon::IsReloading_Implementation() 
{
	return false;
}
bool ABaseWeapon::HasAmmoToReload_Implementation() 
{
	return TotalAmmo > 0;
}
float ABaseWeapon::GetEffectiveRange_Implementation() 
{
	return Range * 6.5f;
}
float ABaseWeapon::GetMaxRange_Implementation() 
{
	return Range * 10.0f;
}
void ABaseWeapon::Reload_Implementation()
{
	bIsReloading = true;
	if (TotalAmmo <= MagazineSize)
	{
		EBarrel->SetAmmo(TotalAmmo, false, false, false, EBarrel->GetAmmo(true));
		TotalAmmo = 0;
	}
	else
	{
		EBarrel->SetAmmo(MagazineSize, false, false, false, EBarrel->GetAmmo(true));
		TotalAmmo -= MagazineSize;
	}
	OnWeaponUpdate();
}
bool ABaseWeapon::HasAmmo_Implementation()
{
	return EBarrel->GetAmmoCount(true) > 0;
}

EWeaponSocketEnum ABaseWeapon::GetWeaponSocket_Implementation()
{
	return EWeaponSocketEnum::PRIMARY;// WeaponPrefabStruct->WeaponSocketType;
}

void ABaseWeapon::ConfigureWeapon_Implementation(const FWeaponCustomizationDataStruct& WeaponPrefab)
{
	PrepareWeapon(WeaponPrefab);
}

EFireMode ABaseWeapon::GetFireMode_Implementation()
{
	return EBarrel->FireMode;
}

void ABaseWeapon::NotifyAim_Implementation(bool AimValue) 
{
	if (SightComponent->GetSightActor())
	{
		SightComponent->GetSightActor()->NotifyAim(AimValue);
	}
}

float ABaseWeapon::GetCameraSensitivity_Implementation()
{
	if (SightComponent->GetSightActor())
	{
		return SightComponent->GetSightActor()->GetBaseSensitivity();
	} else return WeaponAimSensivity;
}

FTransform ABaseWeapon::GetHandIKTransform_Implementation(ERelativeTransformSpace TransformSpace)
{
	return WeaponMeshComponent->GetSocketTransform(HandIKSocketName, TransformSpace);
}

int ABaseWeapon::GetCurrentAmmo_Implementation()
{
	return EBarrel->GetAmmoCount(true);
}

int ABaseWeapon::GetRemainingAmmo_Implementation()
{
	return TotalAmmo;
}

bool ABaseWeapon::GetApplyHandIK_Implementation()
{
	return ApplyHandIK;
}

void ABaseWeapon::RefillAmmo()
{
	TotalAmmo = MaxMagazineCount * MagazineSize;
	EBarrel->SetAmmo(MagazineSize, true, false, false, EBarrel->GetAmmo(true));
}

bool ABaseWeapon::GainMagazine()
{
	int MaxAmmo = MaxMagazineCount * MagazineSize;
	bool bCanGainMagazine = false;
	if (TotalAmmo < MaxAmmo)
	{
		bCanGainMagazine = true;
		TotalAmmo += MagazineSize;
		if(TotalAmmo > MaxAmmo)
		{
			TotalAmmo = MaxAmmo;
		}
	}
	OnWeaponUpdate();
	return bCanGainMagazine;
}

void ABaseWeapon::SetupAim_Implementation(UObject* TargetObject)
{
	if (TargetObject == nullptr) {
		EBarrel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	}
	else {
		FVector TargetLocation = IITargetable::Execute_GetChestLocation(TargetObject);
		FVector TargetVelocity = Cast<AActor>(TargetObject)->GetVelocity();
		TSubclassOf<class AEBBullet> BulletClass = EBarrel->Ammo[0];
		FVector StartLocation = EBarrel->GetComponentLocation();

		FVector TargetAimDirection;
		FVector PredictedTargetLocation;
		float PredictedFlightTime;
		FVector PredictedIntersectionLocation;
		float Error;
		float Step = 0.1f;
		float MaxTime = 10.0f;
		int NumIterations = 4;

		EBarrel->CalculateAimDirectionFromLocation(BulletClass, StartLocation, TargetLocation, TargetVelocity, TargetAimDirection, PredictedTargetLocation, PredictedIntersectionLocation, PredictedFlightTime, Error, MaxTime, Step, NumIterations);
		EBarrel->SetWorldRotation(TargetAimDirection.Rotation());
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), StartLocation, StartLocation + TargetAimDirection * 500000.0f, 10.0f, FColor::Blue, 5.0f, 2.0f);

		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, "Calculated Rotator: ");
		//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TargetAimDirection.Rotation().ToString());

		//Angle calculation - set pitch on new angle





		/*FRotator UnrealRotator = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, "Unreal Rotator: ");
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Blue, UnrealRotator.ToString());
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), StartLocation, StartLocation + UnrealRotator.Vector() * 5000.0f, 10.0f, FColor::Blue, 5.0f, 2.0f);*/


	}
}

void ABaseWeapon::SetFireMode_Implementation(EFireMode NewFireMode)
{
	EBarrel->FireMode = NewFireMode;
}

void ABaseWeapon::SetWeaponSpread_Implementation(float Spread)
{
	EBarrel->Spread = Spread;
}

void ABaseWeapon::ReloadComplete_Implementation()
{
	bIsReloading = false;
}