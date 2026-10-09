// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseSight.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
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

USoundBase* ABaseSight::GetAdjustSound(ESightAdjustSound Sound) const
{
	switch (Sound)
	{
	case ESightAdjustSound::ZoomIn: return ZoomInSound;
	case ESightAdjustSound::ZoomOut: return ZoomOutSound;
	case ESightAdjustSound::ZeroChange: return ZeroChangeSound;
	}
	return nullptr;
}

float ABaseSight::GetBaseSensitivity()
{
	return (HasZoomLevels() && bScaleSensitivityWithZoom) ? BaseMouseSensitivity * GetZoomFOVScale() : BaseMouseSensitivity;
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
	return HasZoomLevels() ? MagnificationToFOV(GetDisplayedMagnification()) : SightFOV;
}

float ABaseSight::GetDisplayedMagnification() const
{
	const float Target = ZoomLevels[GetZoomLevelIndex()];
	if (ZoomTransitionAlpha >= 1.0f || ZoomTransitionFrom <= 0.0f || Target <= 0.0f)
	{
		return Target;
	}
	// Interpolate the logarithm, so 2x -> 4x and 8x -> 16x look like the same amount of zooming
	const float Alpha = FMath::SmoothStep(0.0f, 1.0f, ZoomTransitionAlpha);
	return FMath::Exp(FMath::Lerp(FMath::Loge(ZoomTransitionFrom), FMath::Loge(Target), Alpha));
}

float ABaseSight::GetZoomFOVScale() const
{
	return FMath::Tan(FMath::DegreesToRadians(GetSightFOV() * 0.5f)) / FMath::Tan(FMath::DegreesToRadians(SightFOV * 0.5f));
}

bool ABaseSight::TickZoomTransition(float DeltaTime)
{
	if (ZoomTransitionAlpha >= 1.0f)
	{
		return false;
	}
	ZoomTransitionAlpha = ZoomTransitionTime > 0.0f ? FMath::Min(ZoomTransitionAlpha + DeltaTime / ZoomTransitionTime, 1.0f) : 1.0f;
	return true;
}

bool ABaseSight::HasZoomLevels() const
{
	return bIsScoped && ZoomLevels.Num() > 0;
}

int32 ABaseSight::GetZoomLevelIndex() const
{
	const int32 Index = CurrentZoomLevelIndex == INDEX_NONE ? DefaultZoomLevelIndex : CurrentZoomLevelIndex;
	return FMath::Clamp(Index, 0, FMath::Max(ZoomLevels.Num() - 1, 0));
}

float ABaseSight::MagnificationToFOV(float Magnification) const
{
	// Magnification M shows 1/M of the unmagnified view: tan(FOV / 2) = tan(UnmagnifiedFOV / 2) / M
	const float HalfUnmagnified = FMath::DegreesToRadians(UnmagnifiedFOV * 0.5f);
	return FMath::RadiansToDegrees(2.0f * FMath::Atan(FMath::Tan(HalfUnmagnified) / FMath::Max(Magnification, 0.01f)));
}

float ABaseSight::GetCurrentMagnification() const
{
	if (HasZoomLevels())
	{
		return ZoomLevels[GetZoomLevelIndex()];
	}
	return FMath::Tan(FMath::DegreesToRadians(UnmagnifiedFOV * 0.5f)) / FMath::Tan(FMath::DegreesToRadians(SightFOV * 0.5f));
}

bool ABaseSight::CycleZoomLevel()
{
	if (!HasZoomLevels() || ZoomLevels.Num() < 2)
	{
		return false;
	}
	// Start from what is shown right now, also when the previous transition has not finished
	ZoomTransitionFrom = GetDisplayedMagnification();
	CurrentZoomLevelIndex = (GetZoomLevelIndex() + 1) % ZoomLevels.Num();
	ZoomTransitionAlpha = ZoomTransitionTime > 0.0f ? 0.0f : 1.0f;
	return true;
}

void ABaseSight::CacheAuthoredSightTransforms()
{
	if (!bAuthoredSightTransformsCached)
	{
		bAuthoredSightTransformsCached = true;
		AuthoredSightMeshLocation = SightMesh->GetRelativeLocation();
		AuthoredSightMeshScale = SightMesh->GetRelativeScale3D();
		AuthoredSightViewScale = SightView->GetRelativeScale3D();
	}
}

void ABaseSight::SetAimingMeshScale(bool bIsAiming)
{
	CacheAuthoredSightTransforms();

	if (!bIsScoped || !bIsAiming)
	{
		SightMesh->SetRelativeLocation(AuthoredSightMeshLocation);
		SightMesh->SetRelativeScale3D(AuthoredSightMeshScale);
		SightView->SetRelativeScale3D(AuthoredSightViewScale);
		ApplyZoomToReticleMaterial(1.0f);
		return;
	}

	// The camera zooms by narrowing its FOV, which also magnifies the scope itself. To keep the scope the
	// same size on screen at every zoom, scale it sideways (around the line of sight) by
	// tan(FOV / 2) / tan(SightFOV / 2). Depth along the line of sight is left alone, so the result is exact.
	const float ZoomScale = GetZoomFOVScale();

	// Line of sight in the sight's own space: ADS -> SightView
	const FVector ReticleLocation = SightView->GetRelativeLocation();
	const FVector SightAxis = (ReticleLocation - ADS->GetRelativeLocation()).GetSafeNormal();

	// Move the mesh origin away from / towards the line of sight by the same factor
	const FVector FromReticle = AuthoredSightMeshLocation - ReticleLocation;
	const FVector AlongAxis = SightAxis * FVector::DotProduct(FromReticle, SightAxis);
	SightMesh->SetRelativeLocation(ReticleLocation + AlongAxis + (FromReticle - AlongAxis) * ZoomScale);

	// X is the scope's length (flattened while aiming, as before), Y and Z are across the line of sight
	SightMesh->SetRelativeScale3D(FVector(AuthoredSightMeshScale.X * 0.1f, AuthoredSightMeshScale.Y * ZoomScale, AuthoredSightMeshScale.Z * ZoomScale));

	// The reticle plane sits on the line of sight, scaling it in place keeps it centered
	SightView->SetRelativeScale3D(AuthoredSightViewScale * ZoomScale);
	ApplyZoomToReticleMaterial(ZoomScale);
}

void ABaseSight::ApplyZoomToReticleMaterial(float MeshZoomScale)
{
	if (!HasZoomLevels())
	{
		return;
	}

	if (!bReticleMaterialCached)
	{
		bReticleMaterialCached = true;
		// Returns the existing dynamic instance when slot 0 already has one
		ReticleMaterial = SightView->CreateDynamicMaterialInstance(0);
		if (ReticleMaterial)
		{
			bHasReticleParallaxParameter = !ReticleParallaxParameter.IsNone()
				&& ReticleMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(ReticleParallaxParameter), AuthoredReticleParallax);
			bHasReticleScaleParameter = !ReticleScaleParameter.IsNone()
				&& ReticleMaterial->GetScalarParameterValue(FHashedMaterialParameterInfo(ReticleScaleParameter), AuthoredReticleScale);
		}
	}
	if (!ReticleMaterial)
	{
		return;
	}

	if (bHasReticleParallaxParameter)
	{
		ReticleMaterial->SetScalarParameterValue(ReticleParallaxParameter, AuthoredReticleParallax * FMath::Pow(MeshZoomScale, ReticleParallaxZoomExponent));
	}
	if (bHasReticleScaleParameter)
	{
		ReticleMaterial->SetScalarParameterValue(ReticleScaleParameter, AuthoredReticleScale * FMath::Pow(MeshZoomScale, ReticleScaleZoomExponent));
	}
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
