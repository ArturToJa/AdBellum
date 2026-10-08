// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseSight.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarSightDebugCalibration(
	TEXT("adb.Sight.DebugCalibration"),
	0,
	TEXT("Draws sight calibration debug. 0 - off, 1 - locally controlled player weapon, 2 - all weapons"),
	ECVF_Default);

// Sets default values
ABaseSight::ABaseSight()
{
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	PrimaryActorTick.bCanEverTick = false;
	SightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sight Mesh"));
	SightView = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sight View"));
	ADS = CreateDefaultSubobject<USceneComponent>(TEXT("ADS Target"));

	SightMesh->SetupAttachment(RootComponent);
	SightView->SetupAttachment(RootComponent);
	ADS->SetupAttachment(RootComponent);

	SightArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("ViewArrow"));
	SightArrow->SetupAttachment(SightView);
	bReplicates = false;
}

void ABaseSight::NotifyAim(bool bIsAiming)
{
}

float ABaseSight::GetBaseSensitivity()
{
	return BaseMouseSensitivity;
}

USightChildActorComponent::USightChildActorComponent(const FObjectInitializer& Initializer):UChildActorComponent(Initializer)
{
	SetIsReplicatedByDefault(true);
}

ABaseSight* USightChildActorComponent::GetSightActor() const
{
	return Cast<ABaseSight>(this->GetChildActor());
}

float ABaseSight::GetSightFOV() const
{
	return SightFOV;
}

void ABaseSight::SetAimingMeshScale(bool bIsAiming) 
{
	SightMesh->SetRelativeScale3D(bIsScoped && bIsAiming
		? FVector(0.1f, 1.0f, 1.0f)
		: FVector(1.0f, 1.0f, 1.0f));
}

bool ABaseSight::GetIsScoped() const
{
	return bIsScoped;
}

FVector ABaseSight::GetSightLineOrigin() const
{
	return ADS ? ADS->GetComponentLocation() : GetActorLocation();
}

FVector ABaseSight::GetSightAxisDirection() const
{
	if (SightArrow)
	{
		return SightArrow->GetForwardVector();
	}
	return SightView ? SightView->GetForwardVector() : GetActorForwardVector();
}

FVector ABaseSight::GetReticlePoint() const
{
	const FVector ReticleLocation = SightView ? SightView->GetComponentLocation() : GetActorLocation();
	return ReticleLocation + GetSightAxisDirection() * ReticleDistance;
}

FVector ABaseSight::GetSightLineDirection() const
{
	if (ADS && SightView)
	{
		const FVector Direction = (GetReticlePoint() - ADS->GetComponentLocation()).GetSafeNormal();
		if (!Direction.IsNearlyZero())
		{
			return Direction;
		}
	}
	return GetSightAxisDirection();
}

bool ABaseSight::ValidateSightSetup() const
{
	FString Problem;
	if (!ADS || !SightView)
	{
		Problem = TEXT("missing ADS or SightView component");
	}
	else if (FVector::Dist(ADS->GetComponentLocation(), SightView->GetComponentLocation()) < KINDA_SMALL_NUMBER)
	{
		Problem = TEXT("ADS and SightView are at the same location");
	}
	else
	{
		const float CosAngle = FMath::Clamp(FVector::DotProduct(GetSightLineDirection(), GetSightAxisDirection()), -1.0f, 1.0f);
		const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(CosAngle));
		if (AngleDegrees > MaxSightAxisDeviationDegrees)
		{
			const float EyeOffsetCm = FMath::PointDistToLine(ADS->GetComponentLocation(), GetSightAxisDirection(), SightView->GetComponentLocation());
			Problem = FString::Printf(TEXT("ADS -> reticle line deviates %.2f deg from SightArrow direction (max %.2f), ADS is %.2f cm off the SightArrow axis"),
				AngleDegrees, MaxSightAxisDeviationDegrees, EyeOffsetCm);
		}
	}

	if (Problem.IsEmpty())
	{
		return true;
	}
	if (!bSetupWarningLogged)
	{
		bSetupWarningLogged = true;
		UE_LOG(LogTemp, Warning, TEXT("ABaseSight::ValidateSightSetup - %s: %s"), *GetClass()->GetName(), *Problem);
	}
	return false;
}

void ABaseSight::DrawSightLineDebug(float Length) const
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World || !ADS || !SightView)
	{
		return;
	}

	const FVector Origin = GetSightLineOrigin();
	const FColor LineColor = ValidateSightSetup() ? FColor::Green : FColor::Red;

	// Line of sight: ADS -> through reticle -> downrange
	DrawDebugLine(World, Origin, Origin + GetSightLineDirection() * Length, LineColor, false, -1.0f, 0, 0.0f);
	// Authored reticle axis, short so it is distinguishable from the line of sight
	const FVector ReticleLocation = SightView->GetComponentLocation();
	DrawDebugLine(World, ReticleLocation, ReticleLocation + GetSightAxisDirection() * 50.0f, FColor::Cyan, false, -1.0f, 0, 0.0f);
	DrawDebugPoint(World, Origin, 6.0f, FColor::Yellow, false, -1.0f);
#endif
}

int32 ABaseSight::GetCalibrationDebugMode()
{
	return CVarSightDebugCalibration.GetValueOnGameThread();
}
