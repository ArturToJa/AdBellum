// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseWeapon.h"
#include "EBBarrel.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Character/ALSInputInterface.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Perception/AISense_Hearing.h"
#include "Net/UnrealNetwork.h"
#include "Unit/ArmedUnitInterface.h"
#include "Interfaces/ITargetable.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

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

	// Nothing here needs to run every frame. A slow timer catches late arriving state (owner, sight actor,
	// replicated bullet class), the random first delay spreads many weapons over different frames.
	constexpr float OwnerStateInterval = 0.25f;
	GetWorldTimerManager().SetTimer(OwnerStateTimer, this, &ABaseWeapon::RefreshOwnerDependentState,
		OwnerStateInterval, true, FMath::FRandRange(0.0f, OwnerStateInterval));
}

void ABaseWeapon::RefreshOwnerDependentState()
{
	UpdateClientSideAim();
	EnsureSightCalibrated();
	UpdateDebugHooks();
}

void ABaseWeapon::UpdateDebugHooks()
{
#if !UE_BUILD_SHIPPING
	// Debug drawing needs every frame, but only while adb.Sight.DebugCalibration is on
	if (ABaseSight::GetCalibrationDebugMode() > 0 && !bDebugDrawStepScheduled && GetNetMode() != NM_DedicatedServer)
	{
		bDebugDrawStepScheduled = true;
		GetWorldTimerManager().SetTimerForNextTick(this, &ABaseWeapon::StepDebugDraw);
	}

	// Sight diagnostics are sampled after the camera update, registered only while adb.Sight.DiagnosticsFile is on
	const bool bDiagnostics = IsSightDiagnosticsEnabled() && GetNetMode() != NM_DedicatedServer;
	if (bDiagnostics && !SightDiagnosticsHandle.IsValid())
	{
		SightDiagnosticsHandle = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &ABaseWeapon::WriteSightDiagnostics);
	}
	else if (!bDiagnostics && SightDiagnosticsHandle.IsValid())
	{
		FWorldDelegates::OnWorldPostActorTick.Remove(SightDiagnosticsHandle);
		SightDiagnosticsHandle.Reset();
	}
#endif
}

void ABaseWeapon::StepDebugDraw()
{
	bDebugDrawStepScheduled = false;
	if (ABaseSight::GetCalibrationDebugMode() > 0)
	{
		DrawSightCalibrationDebug();
		bDebugDrawStepScheduled = true;
		GetWorldTimerManager().SetTimerForNextTick(this, &ABaseWeapon::StepDebugDraw);
	}
}

//CUSTOMIZATION

void ABaseWeapon::PrepareWeapon(const FWeaponCustomizationDataStruct& WeaponPrefab)
{
	PrepareMaterials(WeaponPrefab);
	PrepareSight(WeaponPrefab.SightClass);
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

TSubclassOf<AEBBullet> ABaseWeapon::GetCalibrationBulletClass() const
{
	if (!EBarrel)
	{
		return nullptr;
	}
	if (EBarrel->ChamberedBullet)
	{
		return EBarrel->ChamberedBullet;
	}
	if (EBarrel->Ammo.Num() > 0)
	{
		return EBarrel->Ammo[0];
	}
	return nullptr;
}

void ABaseWeapon::StepSightZoomTransition()
{
	bZoomTransitionStepScheduled = false;

	ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	const UWorld* World = GetWorld();
	// True only for the frames between pressing ZoomAction and reaching the new zoom level
	if (!Sight || !World || !Sight->TickZoomTransition(World->GetDeltaSeconds()))
	{
		return;
	}

	if (GetOwner() && GetOwner()->GetClass()->ImplementsInterface(UArmedUnitInterface::StaticClass()))
	{
		IArmedUnitInterface::Execute_RefreshSightZoom(GetOwner(), this);
	}
	bZoomTransitionStepScheduled = true;
	GetWorldTimerManager().SetTimerForNextTick(this, &ABaseWeapon::StepSightZoomTransition);
}

void ABaseWeapon::EnsureSightCalibrated()
{
	if (SightCalibrationMode != ESightCalibrationMode::Ballistic)
	{
		return;
	}

	// ZeroIndexSight is the sight of the last successful calibration
	ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	if (!Sight || Sight == ZeroIndexSight || Sight == CalibrationAttemptedSight)
	{
		return;
	}

	// Calibration is only visible to the player looking through the sight
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled() || !Cast<APlayerController>(OwnerPawn->GetController()))
	{
		return;
	}

	// Bullet class is replicated, wait until it is known on this machine
	if (!GetCalibrationBulletClass())
	{
		return;
	}

	// One attempt per sight actor, so a failing calibration does not repeat every frame
	CalibrationAttemptedSight = Sight;
	CalibrateSight(GetDefaultSightZeroDistance());
}

void ABaseWeapon::UpdateClientSideAim()
{
	if (!EBarrel)
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	const bool bPlayerControlled = Cast<APlayerController>(OwnerController) != nullptr;

	// ClientSideAim is not replicated, every machine decides for itself:
	// - server: accept aim from the player's machine
	// - controlling player's machine: send aim to the server
	// - everybody else (other clients, AI controlled units): off
	const bool bEnable = bUseClientSideAimForPlayers && bPlayerControlled && (HasAuthority() || OwnerPawn->IsLocallyControlled());
	AController* AimController = bEnable ? OwnerController : nullptr;

	if (ClientSideAimController != AimController || EBarrel->ClientSideAim != bEnable)
	{
		// Also resets aim received from the previous player
		ClientSideAimController = AimController;
		EBarrel->SetClientSideAim(bEnable);
	}
}

void ABaseWeapon::DrawSightCalibrationDebug() const
{
	const int32 DebugMode = ABaseSight::GetCalibrationDebugMode();
	if (DebugMode <= 0 || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (DebugMode == 1)
	{
		const APawn* OwnerPawn = Cast<APawn>(GetOwner());
		if (!OwnerPawn || !OwnerPawn->IsLocallyControlled() || !OwnerPawn->IsPlayerControlled())
		{
			return;
		}
	}
	const ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	if (!Sight)
	{
		return;
	}

	const float DrawDistance = FMath::Max(SightTargetDistance, CalibratedDistance * 1.5f) * 100.0f;
	Sight->DrawSightLineDebug(DrawDistance);

#if ENABLE_DRAW_DEBUG
	if (CalibrationTrajectory.Num() > 1)
	{
		UWorld* World = GetWorld();
		const FTransform BarrelTransform = GetBaseBarrelWorldTransform();
		for (int32 Index = 1; Index < CalibrationTrajectory.Num(); ++Index)
		{
			DrawDebugLine(World,
				BarrelTransform.TransformPosition(CalibrationTrajectory[Index - 1]),
				BarrelTransform.TransformPosition(CalibrationTrajectory[Index]),
				FColor::Orange, false, -1.0f, 0, 0.0f);
		}
		DrawDebugSphere(World, BarrelTransform.TransformPosition(CalibrationZeroPoint), 5.0f, 8, FColor::White, false, -1.0f);
	}
#endif
}

FTransform ABaseWeapon::GetBaseBarrelWorldTransform() const
{
	if (!EBarrel)
	{
		return GetActorTransform();
	}
	const USceneComponent* Parent = EBarrel->GetAttachParent();
	const FTransform ParentTransform = Parent ? Parent->GetSocketTransform(EBarrel->GetAttachSocketName()) : FTransform::Identity;
	FTransform Result = FTransform(BaseBarrelRelativeRotation, EBarrel->GetRelativeLocation()) * ParentTransform;
	Result.SetScale3D(FVector::OneVector);
	return Result;
}

bool ABaseWeapon::SimulateTrajectoryPoint(TSubclassOf<AEBBullet> BulletClass, float DistanceMeters, FVector& OutRangePoint,
	float& OutFlightTime, TArray<FVector>* OutTrajectory) const
{
	UWorld* World = GetWorld();
	const AEBBullet* Bullet = BulletClass ? BulletClass->GetDefaultObject<AEBBullet>() : nullptr;
	if (!World || !Bullet || !EBarrel || DistanceMeters <= 0.0f)
	{
		return false;
	}

	constexpr float TimeStep = 0.001f;
	constexpr float MaxTime = 10.0f;
	constexpr int32 TrajectorySampleInterval = 10;

	const float TargetX = DistanceMeters * 100.0f;
	// When collecting trajectory for debug, continue past the zero point to show the drop behind it
	const float EndX = OutTrajectory ? TargetX * 1.5f : TargetX;

	// Same muzzle velocity as EBBarrel prediction functions use
	const float MuzzleSpeed = FMath::Lerp(EBarrel->MuzzleVelocityMultiplierMin, EBarrel->MuzzleVelocityMultiplierMax, 0.5f)
		* FMath::Lerp(Bullet->MuzzleVelocityMin, Bullet->MuzzleVelocityMax, 0.5f);
	if (MuzzleSpeed <= 0.0f)
	{
		return false;
	}

	// Virtual level frame: real barrel location (air density depends on altitude), shooting along world +X,
	// so gravity (world -Z) acts as if the weapon was held level. Range frame X = downrange, Z = up.
	const FVector Start = GetBaseBarrelWorldTransform().GetLocation();
	FVector Location = Start;
	FVector Velocity(MuzzleSpeed, 0.0f, 0.0f);
	float Time = 0.0f;
	int32 StepIndex = 0;
	bool bFound = false;

	if (OutTrajectory)
	{
		OutTrajectory->Reset();
		OutTrajectory->Add(FVector::ZeroVector);
	}

	while (Time < MaxTime)
	{
		const FVector PreviousVelocity = Velocity;
		Velocity = Bullet->UpdateVelocity(World, Location, Velocity, TimeStep);
		const FVector NewLocation = Location + (PreviousVelocity + Velocity) * 0.5f * TimeStep;

		if (!bFound && NewLocation.X - Start.X >= TargetX)
		{
			const float Alpha = (TargetX - (Location.X - Start.X)) / (NewLocation.X - Location.X);
			OutRangePoint = FMath::Lerp(Location, NewLocation, Alpha) - Start;
			OutFlightTime = Time + Alpha * TimeStep;
			bFound = true;
		}

		Location = NewLocation;
		Time += TimeStep;

		const bool bEnd = Location.X - Start.X >= EndX || Velocity.X <= KINDA_SMALL_NUMBER;
		if (OutTrajectory && (++StepIndex % TrajectorySampleInterval == 0 || bEnd))
		{
			OutTrajectory->Add(Location - Start);
		}
		if (bEnd)
		{
			break;
		}
	}
	return bFound;
}

// Signed angle (radians) rotating From to To around Axis, both projected onto the plane perpendicular to Axis
static float SignedAngleAroundAxis(const FVector& From, const FVector& To, const FVector& Axis)
{
	const FVector FromProjected = FVector::VectorPlaneProject(From, Axis).GetSafeNormal();
	const FVector ToProjected = FVector::VectorPlaneProject(To, Axis).GetSafeNormal();
	if (FromProjected.IsNearlyZero() || ToProjected.IsNearlyZero())
	{
		return 0.0f;
	}
	return FMath::Atan2(FVector::DotProduct(Axis, FVector::CrossProduct(FromProjected, ToProjected)),
		FVector::DotProduct(FromProjected, ToProjected));
}

bool ABaseWeapon::CalibrateSight(float Distance)
{
	if (SightCalibrationMode != ESightCalibrationMode::Ballistic)
	{
		return false;
	}

	ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	if (!Sight || !EBarrel)
	{
		return false;
	}

	const TSubclassOf<AEBBullet> BulletClass = GetCalibrationBulletClass();
	if (!BulletClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseWeapon::CalibrateSight - %s: no bullet class in EBarrel"), *GetName());
		return false;
	}

	FVector ZeroPointRange;
	float FlightTime = 0.0f;
	TArray<FVector> Trajectory;
	if (!SimulateTrajectoryPoint(BulletClass, Distance, ZeroPointRange, FlightTime, &Trajectory))
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseWeapon::CalibrateSight - %s: bullet %s does not reach %.1f m"),
			*GetName(), *BulletClass->GetName(), Distance);
		return false;
	}

	Sight->ValidateSightSetup();

	// Calibration rotates the sight actor's root, not SightComponent: SightComponent is replicated,
	// the sight actor is not, so the rotation stays local to this machine.
	USceneComponent* SightRoot = Sight->GetRootComponent();
	if (!SightRoot)
	{
		return false;
	}

	// Always start from the uncalibrated rotation, so calibration result does not depend on previous calls
	if (BaseRotationSight != Sight)
	{
		BaseSightRelativeRotation = SightRoot->GetRelativeRotation();
		BaseRotationSight = Sight;
	}
	SightRoot->SetRelativeRotation(BaseSightRelativeRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FTransform BarrelTransform = GetBaseBarrelWorldTransform();
	const FVector ZeroPoint = BarrelTransform.TransformPosition(ZeroPointRange);
	const FVector PitchAxis = BarrelTransform.GetUnitAxis(EAxis::Y);
	const FVector YawAxis = BarrelTransform.GetUnitAxis(EAxis::Z);

	// Rotating around the sight root also moves ADS, so repeat until the line of sight hits the zero point
	constexpr int32 MaxIterations = 4;
	for (int32 Iteration = 0; Iteration < MaxIterations; ++Iteration)
	{
		const FVector CurrentDirection = Sight->GetSightLineDirection();
		const FVector DesiredDirection = (ZeroPoint - Sight->GetSightLineOrigin()).GetSafeNormal();

		FQuat Delta(PitchAxis, SignedAngleAroundAxis(CurrentDirection, DesiredDirection, PitchAxis));
		if (bCalibrateSightYaw)
		{
			const FVector PitchedDirection = Delta.RotateVector(CurrentDirection);
			Delta = FQuat(YawAxis, SignedAngleAroundAxis(PitchedDirection, DesiredDirection, YawAxis)) * Delta;
		}
		if (Delta.GetAngle() < 1.0e-7f)
		{
			break;
		}
		SightRoot->SetWorldRotation(Delta * SightRoot->GetComponentQuat(), false, nullptr, ETeleportType::TeleportPhysics);
	}

	const FVector FinalDirection = Sight->GetSightLineDirection();
	const FVector FinalDesired = (ZeroPoint - Sight->GetSightLineOrigin()).GetSafeNormal();
	const float RemainingErrorCm = FVector::CrossProduct(FinalDirection, FinalDesired).Size() * Distance * 100.0f;
	const float CorrectionMrad = (BaseSightRelativeRotation.Quaternion().Inverse() * SightRoot->GetRelativeRotation().Quaternion()).GetAngle() * 1000.0f;

	CalibratedDistance = Distance;
	CalibrationZeroPoint = ZeroPointRange;
	CalibrationTrajectory = MoveTemp(Trajectory);

	// Keep zero stepping in sync when calibrated to one of the sight's configured distances
	ZeroIndexSight = Sight;
	CurrentZeroIndex = Sight->ZeroDistances.IndexOfByPredicate([Distance](float ZeroDistance)
	{
		return FMath::IsNearlyEqual(ZeroDistance, Distance, 0.01f);
	});

	UE_LOG(LogTemp, Log, TEXT("ABaseWeapon::CalibrateSight - %s: %s zeroed at %.1f m, drop %.1f cm, flight %.3f s, correction %.2f mrad, remaining error %.2f cm"),
		*GetName(), *BulletClass->GetName(), Distance, -ZeroPointRange.Z, FlightTime, CorrectionMrad, RemainingErrorCm);
	return true;
}

float ABaseWeapon::GetDefaultSightZeroDistance() const
{
	const ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	if (Sight && Sight->ZeroDistances.Num() > 0)
	{
		return Sight->ZeroDistances[FMath::Clamp(Sight->DefaultZeroDistanceIndex, 0, Sight->ZeroDistances.Num() - 1)];
	}
	return SightTargetDistance;
}

float ABaseWeapon::GetSightZeroDistance_Implementation()
{
	return CalibratedDistance > 0.0f ? CalibratedDistance : GetDefaultSightZeroDistance();
}

float ABaseWeapon::GetSightMagnification_Implementation()
{
	const ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	return Sight ? Sight->GetCurrentMagnification() : 1.0f;
}

void ABaseWeapon::PlaySightAdjustSoundLocal(ESightAdjustSound Sound)
{
	// The sight actor exists separately on every machine, each one plays its own copy of the sound
	const ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	if (USoundBase* SoundAsset = Sight ? Sight->GetAdjustSound(Sound) : nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundAsset, Sight->GetActorLocation());
	}
}

void ABaseWeapon::PlaySightAdjustSound(ESightAdjustSound Sound)
{
	PlaySightAdjustSoundLocal(Sound);
	if (GetNetMode() != NM_Standalone)
	{
		Server_PlaySightAdjustSound(Sound);
	}
}

void ABaseWeapon::Server_PlaySightAdjustSound_Implementation(ESightAdjustSound Sound)
{
	Multicast_PlaySightAdjustSound(Sound);
}

void ABaseWeapon::Multicast_PlaySightAdjustSound_Implementation(ESightAdjustSound Sound)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	// The player who adjusted the sight already heard it in PlaySightAdjustSound
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		return;
	}
	PlaySightAdjustSoundLocal(Sound);
}

void ABaseWeapon::ChangeSightZoom_Implementation()
{
	ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	const float PreviousMagnification = Sight ? Sight->GetCurrentMagnification() : 0.0f;
	if (!Sight || !Sight->CycleZoomLevel())
	{
		return;
	}

	// Covers an instant switch (ZoomTransitionTime 0); a timed one keeps refreshing every frame until it ends
	if (GetOwner() && GetOwner()->GetClass()->ImplementsInterface(UArmedUnitInterface::StaticClass()))
	{
		IArmedUnitInterface::Execute_RefreshSightZoom(GetOwner(), this);
	}
	if (!bZoomTransitionStepScheduled)
	{
		bZoomTransitionStepScheduled = true;
		GetWorldTimerManager().SetTimerForNextTick(this, &ABaseWeapon::StepSightZoomTransition);
	}

	const float NewMagnification = Sight->GetCurrentMagnification();
	// Wrapping from the highest level back to the lowest counts as zooming out
	PlaySightAdjustSound(NewMagnification > PreviousMagnification ? ESightAdjustSound::ZoomIn : ESightAdjustSound::ZoomOut);
	OnSightZoomChanged(NewMagnification);
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage((uint64)GetUniqueID() + 1, 2.0f, FColor::White, FString::Printf(TEXT("Sight zoom: %gx"), NewMagnification));
	}
#endif
}

void ABaseWeapon::ChangeSightZero_Implementation(int32 Direction)
{
	ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	if (!Sight || Sight->ZeroDistances.Num() == 0 || Direction == 0)
	{
		return;
	}

	const int32 LastIndex = Sight->ZeroDistances.Num() - 1;
	const bool bHasIndex = ZeroIndexSight == Sight && CurrentZeroIndex != INDEX_NONE;
	const int32 Index = bHasIndex ? CurrentZeroIndex : FMath::Clamp(Sight->DefaultZeroDistanceIndex, 0, LastIndex);
	const int32 NewIndex = FMath::Clamp(Index + FMath::Sign(Direction), 0, LastIndex);
	if (bHasIndex && NewIndex == Index)
	{
		return;
	}

	const float NewDistance = Sight->ZeroDistances[NewIndex];
	if (CalibrateSight(NewDistance))
	{
		PlaySightAdjustSound(ESightAdjustSound::ZeroChange);
		OnSightZeroChanged(NewDistance);
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage((uint64)GetUniqueID(), 2.0f, FColor::White, FString::Printf(TEXT("Sight zero: %.0f m"), NewDistance));
		}
#endif
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

FVector ABaseWeapon::CalculateSightRotation(FVector StartLocation, FVector TargetLocation, FVector TargetVelocity)
{
	TSubclassOf<class AEBBullet> BulletClass = EBarrel->Ammo[0];
	FVector TargetAimDirection;
	FVector PredictedTargetLocation;
	float PredictedFlightTime;
	FVector PredictedIntersectionLocation;
	float Error;
	float Step = 0.1f;
	float MaxTime = 10.0f;
	int NumIterations = 4;
	EBarrel->CalculateAimDirectionFromLocation(BulletClass, StartLocation, TargetLocation, TargetVelocity, TargetAimDirection, PredictedTargetLocation, PredictedIntersectionLocation, PredictedFlightTime, Error, MaxTime, Step, NumIterations);
	return TargetAimDirection;
}
//END CUSTOMIZATION

//SHOOTING LOGIC
void ABaseWeapon::HandleBarrelShotFired()
{
	LastShotWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastShotWorldTime;
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
	IArmedUnitInterface::Execute_OnWeaponUpdated(GetOwner(), this);
}

FRotator ABaseWeapon::getADSCalibrationRotation_Implementation()
{
	const ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	const USceneComponent* SightRoot = Sight ? Sight->GetRootComponent() : nullptr;
	// BaseSightRelativeRotation is only known for the sight that was calibrated
	if (!SightRoot || BaseRotationSight != Sight)
	{
		return FRotator::ZeroRotator;
	}

	// World rotation the sight root would have without calibration, in the weapon's current pose
	const USceneComponent* Parent = SightRoot->GetAttachParent();
	const FQuat ParentRotation = Parent ? Parent->GetSocketQuaternion(SightRoot->GetAttachSocketName()) : FQuat::Identity;
	const FQuat UncalibratedRotation = ParentRotation * BaseSightRelativeRotation.Quaternion();

	// Rotation that takes the uncalibrated sight to the calibrated one
	return (SightRoot->GetComponentQuat() * UncalibratedRotation.Inverse()).Rotator();
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
	return bIsReloading;
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

void ABaseWeapon::ResetAim_Implementation()
{
	EBarrel->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
}

void ABaseWeapon::SetupAim_Implementation(UObject* TargetObject)
{
	if(TargetObject){
		FVector TargetLocation = IITargetable::Execute_GetChestLocation(TargetObject);
		FVector TargetVelocity = Cast<AActor>(TargetObject)->GetVelocity();
		FVector StartLocation = EBarrel->GetComponentLocation();

		FVector TargetAimDirection = CalculateSightRotation(StartLocation, TargetLocation, TargetVelocity);
		EBarrel->SetWorldRotation(TargetAimDirection.Rotation());
		UKismetSystemLibrary::DrawDebugArrow(GetWorld(), StartLocation, StartLocation + TargetAimDirection * 500000.0f, 10.0f, FColor::Blue, 0.5f, 2.0f);
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

bool ABaseWeapon::IsTriggerActive_Implementation()
{
	return EBarrel->Shooting;
}

bool ABaseWeapon::IsShooting_Implementation()
{
	return EBarrel ? EBarrel->Shooting : false;
}

void ABaseWeapon::GetWeaponCombatData_Implementation(FWeaponCombatDataStruct& OutWeaponCombatData)
{
	OutWeaponCombatData = WeaponCombatData;
}

float ABaseWeapon::GetWeaponFOV_Implementation()
{
	if (SightComponent->GetSightActor())
	{
		return SightComponent->GetSightActor()->GetSightFOV();
	}
	else
	{
		return 67.5f; // Default FOV if no sight is attached
	}
}

void ABaseWeapon::SetSightMeshScale_Implementation(bool bIsAiming) 
{
	if (SightComponent->GetSightActor()) 
	{
		SightComponent->GetSightActor()->SetAimingMeshScale(bIsAiming);
	}
	if (bIsAiming)
	{
		WeaponMeshComponent->SetVisibility(false, false);
	}
	else
	{
		WeaponMeshComponent->SetVisibility(true, false);
	}
	// Hide the unit's own meshes together with the weapon mesh
	if (GetOwner() && GetOwner()->GetClass()->ImplementsInterface(UArmedUnitInterface::StaticClass()))
	{
		IArmedUnitInterface::Execute_SetBodyHiddenForScope(GetOwner(), bIsAiming);
	}
}

bool ABaseWeapon::GetIsScoped_Implementation() 
{
	if (SightComponent->GetSightActor())
	{
		return SightComponent->GetSightActor()->GetIsScoped();
	}
	else return false;
}