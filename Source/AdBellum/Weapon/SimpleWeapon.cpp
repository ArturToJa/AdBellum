// Fill out your copyright notice in the Description page of Project Settings.


#include "SimpleWeapon.h"
#include "WeaponBallistics.h"
#include "Net/UnrealNetwork.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include <Kismet/GameplayStatics.h>
#include <NiagaraFunctionLibrary.h>


// Sets default values
ASimpleWeapon::ASimpleWeapon()
{
	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	RootComponent = WeaponRoot;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(WeaponRoot);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(WeaponRoot);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EBarrel = CreateDefaultSubobject<UEBBarrel>(TEXT("EBarrel"));
	EBarrel->SetupAttachment(WeaponRoot);

	ADS = CreateDefaultSubobject<USceneComponent>(TEXT("ADS"));
	ADS->SetupAttachment(WeaponRoot);

	bReplicates = true;
	bAlwaysRelevant = true;
	ReplicatedComponents.Add(EBarrel);
}

void ASimpleWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASimpleWeapon, EBarrel);
}

void ASimpleWeapon::BeginPlay()
{
	Super::BeginPlay();

	// A mounted weapon is its own actor: the vehicle's camera arm (and any third person camera trace) would
	// treat it as an obstacle and pull the camera in. Only the Camera channel is changed.
	TInlineComponentArray<UPrimitiveComponent*> Primitives(this);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		Primitive->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	// Nothing here needs to run every frame, the random first delay spreads many weapons over different frames
	constexpr float OwnerStateInterval = 0.25f;
	GetWorldTimerManager().SetTimer(OwnerStateTimer, this, &ASimpleWeapon::RefreshOwnerDependentState,
		OwnerStateInterval, true, FMath::FRandRange(0.0f, OwnerStateInterval));
}

void ASimpleWeapon::RefreshOwnerDependentState()
{
	UpdateClientSideAim();
	EnsureADSCalibrated();
}

void ASimpleWeapon::UpdateClientSideAim()
{
	if (!EBarrel)
	{
		return;
	}

	// Owner is the gunner's unit while the weapon is manned, the vehicle otherwise
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	const bool bPlayerControlled = Cast<APlayerController>(OwnerController) != nullptr;

	// ClientSideAim is not replicated, every machine decides for itself:
	// - server: accept aim from the player's machine
	// - controlling player's machine: send aim to the server
	// - everybody else (other clients, AI gunners): off
	const bool bEnable = bUseClientSideAimForPlayers && bPlayerControlled && (HasAuthority() || OwnerPawn->IsLocallyControlled());
	AController* AimController = bEnable ? OwnerController : nullptr;

	if (ClientSideAimController != AimController || EBarrel->ClientSideAim != bEnable)
	{
		// Also resets aim received from the previous player
		ClientSideAimController = AimController;
		EBarrel->SetClientSideAim(bEnable);
	}
}

void ASimpleWeapon::EnsureADSCalibrated()
{
	if (bCalibrationAttempted || ZeroDistances.Num() == 0)
	{
		return;
	}

	// Calibration is only visible to the player looking through ADS
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled() || !Cast<APlayerController>(OwnerPawn->GetController()))
	{
		return;
	}

	// Bullet class is replicated, wait until it is known on this machine
	if (!WeaponBallistics::GetCalibrationBulletClass(EBarrel))
	{
		return;
	}

	// One attempt, so a failing calibration does not repeat on every timer tick
	bCalibrationAttempted = true;
	CalibrateADS(ZeroDistances[FMath::Clamp(DefaultZeroDistanceIndex, 0, ZeroDistances.Num() - 1)]);
}

bool ASimpleWeapon::CalibrateADS(float Distance)
{
	if (!EBarrel || !ADS)
	{
		return false;
	}

	const TSubclassOf<AEBBullet> BulletClass = WeaponBallistics::GetCalibrationBulletClass(EBarrel);
	if (!BulletClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASimpleWeapon::CalibrateADS - %s: no bullet class in EBarrel"), *GetName());
		return false;
	}

	FTransform BarrelTransform = EBarrel->GetComponentTransform();
	BarrelTransform.SetScale3D(FVector::OneVector);

	FVector ZeroPointRange;
	float FlightTime = 0.0f;
	if (!WeaponBallistics::SimulateTrajectoryPoint(GetWorld(), EBarrel, BulletClass, BarrelTransform.GetLocation(), Distance, ZeroPointRange, FlightTime))
	{
		UE_LOG(LogTemp, Warning, TEXT("ASimpleWeapon::CalibrateADS - %s: bullet %s does not reach %.1f m"),
			*GetName(), *BulletClass->GetName(), Distance);
		return false;
	}

	// Always start from the uncalibrated rotation, so calibration result does not depend on previous calls
	if (!bHasBaseADSRotation)
	{
		BaseADSRelativeRotation = ADS->GetRelativeRotation();
		bHasBaseADSRotation = true;
	}
	ADS->SetRelativeRotation(BaseADSRelativeRotation);

	// ADS turns around its own origin, so one step is exact
	const FVector ZeroPoint = BarrelTransform.TransformPosition(ZeroPointRange);
	const FVector PitchAxis = BarrelTransform.GetUnitAxis(EAxis::Y);
	const FVector YawAxis = BarrelTransform.GetUnitAxis(EAxis::Z);
	const FVector CurrentDirection = ADS->GetForwardVector();
	const FVector DesiredDirection = (ZeroPoint - ADS->GetComponentLocation()).GetSafeNormal();

	FQuat Delta(PitchAxis, WeaponBallistics::SignedAngleAroundAxis(CurrentDirection, DesiredDirection, PitchAxis));
	if (bCalibrateADSYaw)
	{
		const FVector PitchedDirection = Delta.RotateVector(CurrentDirection);
		Delta = FQuat(YawAxis, WeaponBallistics::SignedAngleAroundAxis(PitchedDirection, DesiredDirection, YawAxis)) * Delta;
	}
	ADS->SetWorldRotation(Delta * ADS->GetComponentQuat());

	CalibratedDistance = Distance;
	CurrentZeroIndex = ZeroDistances.IndexOfByPredicate([Distance](float ZeroDistance)
	{
		return FMath::IsNearlyEqual(ZeroDistance, Distance, 0.01f);
	});

	UE_LOG(LogTemp, Log, TEXT("ASimpleWeapon::CalibrateADS - %s: zeroed at %.1f m, drop %.1f cm, flight %.3f s, correction %.2f mrad"),
		*GetName(), Distance, -ZeroPointRange.Z, FlightTime, Delta.GetAngle() * 1000.0f);
	return true;
}

void ASimpleWeapon::ChangeSightZero_Implementation(int32 Direction)
{
	if (ZeroDistances.Num() == 0 || Direction == 0)
	{
		return;
	}

	const int32 LastIndex = ZeroDistances.Num() - 1;
	const bool bHasIndex = CurrentZeroIndex != INDEX_NONE;
	const int32 Index = bHasIndex ? CurrentZeroIndex : FMath::Clamp(DefaultZeroDistanceIndex, 0, LastIndex);
	const int32 NewIndex = FMath::Clamp(Index + FMath::Sign(Direction), 0, LastIndex);
	if (bHasIndex && NewIndex == Index)
	{
		return;
	}

	const float NewDistance = ZeroDistances[NewIndex];
	if (CalibrateADS(NewDistance))
	{
		bCalibrationAttempted = true;
		OnADSZeroChanged(NewDistance);
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage((uint64)GetUniqueID(), 2.0f, FColor::White, FString::Printf(TEXT("Sight zero: %.0f m"), NewDistance));
		}
#endif
	}
}

float ASimpleWeapon::GetSightZeroDistance_Implementation()
{
	if (CalibratedDistance > 0.0f)
	{
		return CalibratedDistance;
	}
	return ZeroDistances.Num() > 0 ? ZeroDistances[FMath::Clamp(DefaultZeroDistanceIndex, 0, ZeroDistances.Num() - 1)] : 0.0f;
}

void ASimpleWeapon::Trigger_Implementation(bool trigger)
{
	Fire(trigger);
}

void ASimpleWeapon::Fire_Implementation(bool trigger)
{
	EBarrel->Shoot(trigger);
}

UEBBarrel* ASimpleWeapon::GetEBarrel_Implementation()
{
	return EBarrel;
}

UTexture2D* ASimpleWeapon::GetWeaponHUDReticle_Implementation()
{
	return WeaponReticleTexture;
}

FVector ASimpleWeapon::GetCalibrationVector_Implementation(float Distance)
{
	return EBarrel->GetForwardVector() * Distance * 100.0f + EBarrel->GetComponentLocation();
	//return EBarrel->GetComponentTransform().GetUnitAxis(EAxis::X)* Distance * 100.0f;
}

FVector ASimpleWeapon::getADSTarget_Implementation()
{
	return ADS->GetComponentLocation();
}

FRotator ASimpleWeapon::getADSRotation_Implementation()
{
	return ADS->GetComponentRotation();
}

float ASimpleWeapon::GetWeaponFOV_Implementation()
{
	return ADSFOV;
}

bool ASimpleWeapon::GetHandIKTargets(FTransform& OutLeft, FTransform& OutRight) const
{
	const UMeshComponent* Meshes[] = { SkeletalMesh, StaticMesh };
	for (const UMeshComponent* Mesh : Meshes)
	{
		if (Mesh && Mesh->DoesSocketExist(LeftHandSocket) && Mesh->DoesSocketExist(RightHandSocket))
		{
			OutLeft = Mesh->GetSocketTransform(LeftHandSocket);
			OutRight = Mesh->GetSocketTransform(RightHandSocket);
			return true;
		}
	}
	return false;
}
