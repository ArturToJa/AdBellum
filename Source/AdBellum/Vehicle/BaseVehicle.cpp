// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseVehicle.h"
#include "GunnerSeatComponent.h"
#include "BaseVehicleAnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/IPlayer.h"
#include "OrderSystem/OrdersManager.h"
#include "Weapon/WeaponChildComponent.h"
#include "TimerManager.h"
#include "AnimationRuntime.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "_Core/AVS_Wheel.h"
#include "Materials/MaterialInstanceDynamic.h"

ABaseVehicle::ABaseVehicle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));

	VehicleSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Vehicle Skeletal Mesh"));
	VehicleSkeletalMesh->SetupAttachment(VehicleMesh);

	DriverSeat = CreateDefaultSubobject<UVehicleSeatComponent>(TEXT("DriverSeat"));
	DriverSeat->SetupAttachment(VehicleSkeletalMesh);
	DriverSeat->SeatRole = EALSStationaryRole::Driver;

	OrdersManagerComponent = CreateDefaultSubobject<UOrdersManager>(TEXT("Order Manager Component"));

	// Third person (debug) camera orbiting the vehicle. Rotation comes from the driver look, set in CalcCamera.
	ThirdPersonCameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonCameraArm"));
	ThirdPersonCameraArm->SetupAttachment(VehicleMesh);
	ThirdPersonCameraArm->TargetArmLength = 900.0f;
	ThirdPersonCameraArm->TargetOffset = FVector(0.0f, 0.0f, 250.0f);
	ThirdPersonCameraArm->bUsePawnControlRotation = false;
	ThirdPersonCameraArm->bInheritRoll = false;
	ThirdPersonCameraArm->bDoCollisionTest = true;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonCameraArm, USpringArmComponent::SocketName);

	// Wheel, turret and gun bones are animated in C++, no anim blueprint needed
	VehicleSkeletalMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	VehicleSkeletalMesh->AnimClass = UBaseVehicleAnimInstance::StaticClass();

	// Forward / backward input drives and reverses without the driver touching the shifter
	AutomaticShifterPositon = true;

	// Created wheels have no mesh of their own to measure, the radius is set here
	DefaultWheelConfig.WheelMode = EWheelMode::Raycast;
	DefaultWheelConfig.AutoWheelRadius = false;
}

//WHEELS
void ABaseVehicle::OnConstruction(const FTransform& Transform)
{
	// AVS collects and constructs its wheel components in its construction, so ours have to exist first
	BuildWheelsFromSetup();
	Super::OnConstruction(Transform);
	ApplyHiddenBones();
}

//MESH
void ABaseVehicle::ApplyHiddenBones()
{
	if (!VehicleSkeletalMesh || !VehicleSkeletalMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	for (const FName& BoneName : HiddenBones)
	{
		if (VehicleSkeletalMesh->GetBoneIndex(BoneName) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("ABaseVehicle::ApplyHiddenBones - %s: skeletal mesh has no bone '%s'"),
				*GetName(), *BoneName.ToString());
			continue;
		}
		VehicleSkeletalMesh->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
	}
}

TArray<FVehicleWheelSetup> ABaseVehicle::GetEffectiveWheelSetups() const
{
	// Every property read in this function is EditDefaultsOnly - vehicle-type tuning, not meant to vary per
	// placed instance - so it's read from the CDO rather than `this`. A stale per-instance override (the
	// editor's own per-instance tools can't clear an EditDefaultsOnly property, so one can outlive the edit
	// that made it wrong) would otherwise silently keep using the old value forever.
	const ABaseVehicle* Default = GetClass()->GetDefaultObject<ABaseVehicle>();

	if (Default->WheelSetups.Num() > 0)
	{
		return Default->WheelSetups;
	}

	TArray<FVehicleWheelSetup> Setups;
	const USkeletalMesh* MeshAsset = VehicleSkeletalMesh ? VehicleSkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	if (!MeshAsset)
	{
		return Setups;
	}

	// lf_wheel_N_jnt / rt_wheel_N_jnt (standard - spins and, where present, also carries suspension height).
	// lf_wheel_N_fake_jnt / rt_wheel_N_fake_jnt is only used as a fallback when no plain _jnt sibling exists:
	// on some rigs (e.g. Vigilante's BMP2) that bone is a dead locator with nothing skinned to it.
	// When a sibling lf_wheel_N_root_jnt / rt_wheel_N_root_jnt also exists (Vigilante convention: that bone
	// alone carries the suspension/track skin weight, the wheel's own _jnt only spins), it is recorded as
	// SuspensionBoneName so the anim instance moves it separately from the spin bone.
	TArray<int32> Axles;
	TSet<FString> JointCovered; // "L3" / "R7"
	const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();

	auto TryAddWheelBone = [&](const FName& BoneName, bool bFakeJoint)
	{
		FString Prefix = BoneName.ToString();
		if (!(bFakeJoint ? Prefix.RemoveFromEnd(TEXT("_fake_jnt")) : Prefix.RemoveFromEnd(TEXT("_jnt"))))
		{
			return;
		}
		FString AxleString = Prefix;
		FString Side;
		if (AxleString.RemoveFromStart(TEXT("lf_wheel_"))) Side = TEXT("L");
		else if (AxleString.RemoveFromStart(TEXT("rt_wheel_"))) Side = TEXT("R");
		else return;
		if (!AxleString.IsNumeric())
		{
			return;
		}

		const FString Key = Side + AxleString;
		if (!bFakeJoint)
		{
			JointCovered.Add(Key);
		}
		else if (JointCovered.Contains(Key))
		{
			return; // a plain _jnt sibling for this wheel already got added
		}

		FVehicleWheelSetup& NewSetup = Setups.AddDefaulted_GetRef();
		NewSetup.BoneName = BoneName;
		if (!bFakeJoint)
		{
			const FName SuspensionBone(*(Prefix + TEXT("_root_jnt")));
			if (RefSkeleton.FindBoneIndex(SuspensionBone) != INDEX_NONE)
			{
				NewSetup.SuspensionBoneName = SuspensionBone;

				const FName PivotBone(*(Prefix + TEXT("_hidr_chasing_jnt")));
				if (RefSkeleton.FindBoneIndex(PivotBone) != INDEX_NONE)
				{
					NewSetup.SuspensionPivotBoneName = PivotBone;
				}
			}
		}
		Axles.Add(FCString::Atoi(*AxleString));
	};

	// plain _jnt bones first, so the fake_jnt pass below only fills in axles with no real sibling
	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		if (!BoneName.ToString().EndsWith(TEXT("_fake_jnt")))
		{
			TryAddWheelBone(BoneName, false);
		}
	}
	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		if (BoneName.ToString().EndsWith(TEXT("_fake_jnt")))
		{
			TryAddWheelBone(BoneName, true);
		}
	}

	const int32 LastAxle = Axles.Num() > 0 ? FMath::Max(Axles) : 0;
	for (int32 Index = 0; Index < Setups.Num(); ++Index)
	{
		const int32 Axle = Axles[Index];
		// Tracked vehicles always brake-steer (see UpdateTrackSteeringLock) - no wheel should also try to
		// yaw-steer, regardless of what SteerableAxles says (a per-instance leftover is an easy mistake).
		// A half-track with some genuinely steerable wheeled axles is not supported by this flag yet: bTracked
		// is still all-or-nothing, SteerableAxles would need to become per-axle alongside it.
		Setups[Index].bSteerable = !Default->bTracked && Default->SteerableAxles.Contains(Axle);
		Setups[Index].bDriving = Default->DrivenAxles.Num() == 0 || Default->DrivenAxles.Contains(Axle);
		Setups[Index].bHandbrake = Default->HandbrakeAxles.Num() == 0 ? Axle == LastAxle : Default->HandbrakeAxles.Contains(Axle);
		// The first/last axle of a tracked vehicle is the sprocket/idler: fixed in place, no suspension travel
		Setups[Index].bFixedSuspension = Default->bTracked && (Axle == 1 || Axle == LastAxle);
	}
	return Setups;
}

void ABaseVehicle::BuildWheelsFromSetup()
{
	if (!VehicleSkeletalMesh || !VehicleSkeletalMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	// Everything is placed from the reference pose and relative transforms: during construction the
	// components may not be registered, so their world transforms and bone poses are not usable yet
	FTransform MeshToVehicle = FTransform::Identity;
	for (const USceneComponent* Component = VehicleSkeletalMesh; Component && Component != VehicleMesh; Component = Component->GetAttachParent())
	{
		MeshToVehicle = MeshToVehicle * Component->GetRelativeTransform();
	}
	const FReferenceSkeleton& RefSkeleton = VehicleSkeletalMesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	// EditDefaultsOnly, see the comment in GetEffectiveWheelSetups
	const FAVS1_Wheel_Config& DefaultConfig = GetClass()->GetDefaultObject<ABaseVehicle>()->DefaultWheelConfig;

	// Wheels are thrown away and rebuilt every time construction reruns, rebuild this lookup alongside them
	WheelSuspensionArcs.Reset();
	FixedSuspensionWheelBones.Reset();

	for (const FVehicleWheelSetup& Setup : GetEffectiveWheelSetups())
	{
		const int32 BoneIndex = RefSkeleton.FindBoneIndex(Setup.BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("ABaseVehicle::BuildWheelsFromSetup - %s: skeletal mesh has no bone '%s', wheel skipped"),
				*GetName(), *Setup.BoneName.ToString());
			continue;
		}

		// Same creation method the construction script uses: the wheels are thrown away and rebuilt
		// every time construction reruns, nothing of them is saved with the actor
		UAVS_Wheel* Wheel = NewObject<UAVS_Wheel>(this);
		Wheel->CreationMethod = EComponentCreationMethod::UserConstructionScript;

		Wheel->WheelConfig = DefaultConfig;
		Wheel->WheelConfig.IsDrivingWheel = Setup.bDriving;
		Wheel->WheelConfig.InvertTorque = Setup.bInvertTorque;
		Wheel->WheelConfig.IsSteerableWheel = Setup.bSteerable;
		Wheel->WheelConfig.InvertSteering = Setup.bInvertSteering;
		Wheel->WheelConfig.IsHandbrakeWheel = Setup.bHandbrake;
		if (Setup.bFixedSuspension)
		{
			// Rigid (never visibly compresses) rather than literally Spring Length 0: for a Line trace, trace
			// length is SpringLength + WheelRadius*2 (see AAVS_Vehicle::AVS_PhysicsTick), so zeroing it instead
			// of stiffening it also shortens the wheel's ground-detection reach - and this one's fixed attach
			// height doesn't drop to compensate the way a compliant wheel's spring compression does, so it can
			// come up short of the ground at the same body height a compliant wheel comfortably still reaches
			// (losing contact, then spin getting stuck at zero - confirmed on this project's own BMP2).
			Wheel->WheelConfig.SpringStrength *= 20.0f;
			Wheel->WheelConfig.SpringDamping *= 20.0f;
			FixedSuspensionWheelBones.Add(Setup.BoneName);
		}

		// The named bone is moved with the wheel by UBaseVehicleAnimInstance. The wheel itself hangs on the
		// vehicle body at the bone's reference location (not on the bone, it would ride on what it animates),
		// X forward and Z up of the vehicle, whatever way the skeletal mesh or the bone is turned.
		Wheel->BoneName = Setup.BoneName;
		Wheel->SetupAttachment(VehicleMesh);
		Wheel->SetRelativeLocation(MeshToVehicle.TransformPosition(FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, BoneIndex).GetLocation()));
		Wheel->RegisterComponent();

		if (!Setup.SuspensionBoneName.IsNone())
		{
			FWheelSuspensionArc Arc;
			Arc.SuspensionBoneName = Setup.SuspensionBoneName;
			if (!Setup.SuspensionPivotBoneName.IsNone())
			{
				const int32 PivotIndex = RefSkeleton.FindBoneIndex(Setup.SuspensionPivotBoneName);
				const int32 SuspensionIndex = RefSkeleton.FindBoneIndex(Setup.SuspensionBoneName);
				if (PivotIndex != INDEX_NONE && SuspensionIndex != INDEX_NONE)
				{
					const FVector PivotRefPos = FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, PivotIndex).GetLocation();
					const FVector SuspensionRefPos = FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, SuspensionIndex).GetLocation();
					Arc.PivotToSuspensionRefPose = SuspensionRefPos - PivotRefPos;
					Arc.bHasPivot = true;
					Arc.MaxArcSwing = Setup.MaxSuspensionArcSwing;
					Arc.MaxTravel = Wheel->WheelConfig.SpringLength;
					Arc.PivotBoneName = Setup.SuspensionPivotBoneName;
				}
			}
			WheelSuspensionArcs.Add(Setup.BoneName, Arc);
		}
	}
}

FName ABaseVehicle::GetWheelSuspensionTranslation(FName WheelBoneName, const FVector& RawTranslation, FVector& OutTranslation, FName& OutPivotBoneName, FQuat& OutPivotRotation) const
{
	OutPivotBoneName = NAME_None;
	OutPivotRotation = FQuat::Identity;

	const FWheelSuspensionArc* Arc = WheelSuspensionArcs.Find(WheelBoneName);
	if (!Arc)
	{
		OutTranslation = FVector::ZeroVector;
		return NAME_None;
	}
	if (!Arc->bHasPivot)
	{
		OutTranslation = RawTranslation;
		return Arc->SuspensionBoneName;
	}

	// Swing the reference-pose arm (pivot -> suspension bone) around the pivot so its Z reproduces
	// RawTranslation.Z, instead of moving it straight up/down - closer to a real hinged arm, and the
	// difference grows with travel, which is exactly where the straight line looked worst.
	const FVector& V0 = Arc->PivotToSuspensionRefPose;
	const float Radius = FMath::Sqrt(V0.X * V0.X + V0.Z * V0.Z);
	if (Radius < KINDA_SMALL_NUMBER)
	{
		// Degenerate arm (pivot directly above/below the suspension bone): nothing meaningful to swing
		OutTranslation = RawTranslation;
		return Arc->SuspensionBoneName;
	}

	const float TargetZ = FMath::Clamp(V0.Z + RawTranslation.Z, -Radius, Radius);
	const float TargetXMag = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - TargetZ * TargetZ));
	const float TargetX = V0.X >= 0.0f ? TargetXMag : -TargetXMag;

	// A real trailing arm does swing fore/aft as it travels (that part of the arc is correct), growing with
	// how far the wheel has travelled rather than a straight up/down line. But the raw arc's X is only used
	// for its direction here, not its magnitude: how much X changes per cm of Z (the arc's slope) depends
	// entirely on where the pivot happens to sit, and can be steep enough that a clamp on that raw value
	// snaps to the limit within a sliver of travel and then just sits there - fast, unnatural-looking motion
	// at low speed instead of a swing that grows smoothly across the wheel's whole travel range. Scaling by
	// how much of the wheel's actual suspension travel this frame's Z represents keeps the direction the
	// geometry implies but always ramps smoothly from 0 to Max Suspension Arc Swing over the full range.
	const float ArcDirection = FMath::Sign(TargetX - V0.X);
	const float TravelFraction = Arc->MaxTravel > KINDA_SMALL_NUMBER ? FMath::Clamp(FMath::Abs(RawTranslation.Z) / Arc->MaxTravel, 0.0f, 1.0f) : 0.0f;
	const float SwingX = ArcDirection * Arc->MaxArcSwing * TravelFraction;

	OutTranslation = FVector(SwingX, RawTranslation.Y, TargetZ - V0.Z);

	// Cosmetic: the rotation the pivot bone itself would have swung through for that same (direction- and
	// travel-fraction-shaped) swing - not the raw/unclamped geometry, so a visible strut/rod rigidly childed
	// to it (e.g. BMP2's _hidr_rod_jnt) matches what the wheel bone above actually did, not what the raw arc
	// math alone would imply. FindBetweenVectors only uses direction, not magnitude, so V1 doesn't need to
	// land exactly back on the circle of radius Radius.
	if (!Arc->PivotBoneName.IsNone())
	{
		const FVector V1(V0.X + SwingX, V0.Y, TargetZ);
		OutPivotBoneName = Arc->PivotBoneName;
		OutPivotRotation = FQuat::FindBetweenVectors(V0, V1);
	}

	return Arc->SuspensionBoneName;
}

void ABaseVehicle::SyncWheelEffectsFromBaseVehicle()
{
	// GlobalWheelEffects/EffectsForDefaultSurface/SurfaceEffects (inherited from AAVS_Vehicle) are meant to be
	// configured once on BP_BaseVehicle and shared by every vehicle type - but each Blueprint subclass (BP_BMP2,
	// BP_Coyote_LSV, ...) gets its own independent copy the moment either one is touched in the editor, so a
	// child's copy silently drifts out of sync with BP_BaseVehicle's from then on unless hand-copied again every
	// time (the same class of stale-divergence bug this project hit repeatedly with EditDefaultsOnly data this
	// session, just at the Blueprint-inheritance level instead of the per-instance level). Walking up to the
	// first non-native class above the native ABaseVehicle - i.e. BP_BaseVehicle itself, whatever the actual
	// subclass is - and always using its data removes the need to keep every subclass in sync by hand.
	UClass* EffectsSourceClass = GetClass();
	while (EffectsSourceClass && EffectsSourceClass->GetSuperClass() && !EffectsSourceClass->GetSuperClass()->HasAnyClassFlags(CLASS_Native))
	{
		EffectsSourceClass = EffectsSourceClass->GetSuperClass();
	}
	if (const ABaseVehicle* EffectsSource = EffectsSourceClass ? EffectsSourceClass->GetDefaultObject<ABaseVehicle>() : nullptr)
	{
		GlobalWheelEffects = EffectsSource->GlobalWheelEffects;
		EffectsForDefaultSurface = EffectsSource->EffectsForDefaultSurface;
		SurfaceEffects = EffectsSource->SurfaceEffects;
	}
}

void ABaseVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Crew, gunners and the RTS view all need vehicle health, not only the driver who owns the vehicle
	DOREPLIFETIME(ABaseVehicle, HP);
	DOREPLIFETIME(ABaseVehicle, TeamIndex);
	DOREPLIFETIME(ABaseVehicle, PlayerPtr);
	DOREPLIFETIME(ABaseVehicle, OwningFormation);
	// The driver's machine has its own, newer value
	DOREPLIFETIME_CONDITION(ABaseVehicle, ReplicatedDriverLook, COND_SkipOwner);
}

void ABaseVehicle::BeginPlay()
{
	SyncWheelEffectsFromBaseVehicle();

	Super::BeginPlay();
	BackupAIController = Cast<AUnitAIController>(GetController());
	ApplyHiddenBones();
	if (bTracked)
	{
		BuildTrackWheelArrays();
	}
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseVehicle::OnAnyDamageReceived);

		// Weapons are attached to turret / gun bones and AI gunners shoot from the server pose,
		// so the server has to keep the bones up to date even though it renders nothing
		VehicleSkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

//SEATS
UVehicleSeatComponent* ABaseVehicle::GetSeatWithRole(EALSStationaryRole StationaryRole) const
{
	TInlineComponentArray<UVehicleSeatComponent*> Seats(this);
	for (UVehicleSeatComponent* Seat : Seats)
	{
		if (Seat->GetRole() == StationaryRole)
		{
			return Seat;
		}
	}
	return nullptr;
}

TArray<UGunnerSeatComponent*> ABaseVehicle::GetGunnerSeats() const
{
	TArray<UGunnerSeatComponent*> OutSeats;
	GetComponents<UGunnerSeatComponent>(OutSeats);
	return OutSeats;
}

//POSSESSION

void ABaseVehicle::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (NewController->IsPlayerController())
	{
		SetHandbrakeInput(false);
		SetEngineRunning(true);
	}
	// A new driver starts looking straight ahead
	ReplicatedDriverLook = FRotator::ZeroRotator;
}

void ABaseVehicle::UnPossessed()
{
	Super::UnPossessed();
	// Nobody controls the vehicle now, so the server owns its inputs and can let go of them
	ReleaseDriverInputs();
}

void ABaseVehicle::PossessByAIController()
{
	if (HasAuthority() && HP > 0.0f && BackupAIController)
	{
		BackupAIController->Possess(this);
	}
}

//DRIVER INPUT
void ABaseVehicle::ForwardMovementAction_Implementation(float Value)
{
	SetThrottleAndBrakeInput(Value);
}

void ABaseVehicle::ForwardMovementActionCompleted_Implementation()
{
	SetThrottleAndBrakeInput(0.0f);
	if (bTracked)
	{
		// Stops a tank the way it actually stops: both tracks braked, not just coasting to a halt
		SetHandbrakeInput(true);
	}
}

void ABaseVehicle::RightMovementAction_Implementation(float Value)
{
	// Still replicated/smoothed (free) even though no wheel turns for a tracked vehicle: no wheel is marked
	// steerable for one, so this has no effect on wheel yaw, only on the cosmetic track speed below.
	SetSteeringInput(Value);
	if (bTracked)
	{
		UpdateTrackSteeringLock(Value);
	}
}

void ABaseVehicle::RightMovementActionCompleted_Implementation()
{
	SetSteeringInput(0.0f);
	if (bTracked)
	{
		UpdateTrackSteeringLock(0.0f);
	}
}

void ABaseVehicle::JumpAction_Implementation(bool bValue)
{
	// JumpAction has a Pressed and a Released trigger: true when the key goes down, false when it comes up
	if (bValue != GetHandbrakeInput())
	{
		SetHandbrakeInput(bValue);
	}
}

void ABaseVehicle::ReleaseDriverInputs()
{
	SetThrottleInput(0.0f);
	SetBrakeInput(0.0f);
	SetSteeringInput(0.0f);
}

//INTERACTION
void ABaseVehicle::InteractionAction_Implementation()
{
	//find vehicle driver seat and invoke Exit delegate with possess
	Server_InteractionAction();
	//IALSInputInterface::Execute_InteractionAction(DriverSeat->GetOccupyingUnit());
}

void ABaseVehicle::Server_InteractionAction_Implementation()
{
	AALSBaseCharacter* Driver = DriverSeat ? DriverSeat->GetOccupyingUnit() : nullptr;
	AController* DriverController = GetController();
	if (!Driver || !DriverController)
	{
		return;
	}

	DriverController->Possess(Driver);

	// Parked: the driver is gone, the vehicle stays where it was left
	ReleaseDriverInputs();
	SetHandbrakeInput(true);
	SetEngineRunning(false);

	PossessByAIController();
	DriverSeat->ExitSeat();
}

//DAMAGE
void ABaseVehicle::OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// Bound on the server only
	if (HP > 0.0f)
	{
		HP -= Damage;
		if (HP <= 0.0f)
		{
			//NotifyDeath();
		}
	}
}

//vehicle interface
float ABaseVehicle::GetSteeringValue_Implementation()
{
	return GetCurrentSteering();
}

//BOOKING
UVehicleAccessPoint* ABaseVehicle::TryBookSeat_Implementation(AALSBaseCharacter* InUnit)
{
	TArray<UVehicleAccessPoint*> AccessPoints = GetAllAccessPoints_Implementation();
	for (UVehicleAccessPoint* AccessPoint : AccessPoints)
	{
		if (AccessPoint->BookSeat(InUnit))
		{
			return AccessPoint;
		}
	}
	return nullptr;
}

TArray<UVehicleAccessPoint*> ABaseVehicle::GetAllAccessPoints_Implementation()
{
	TArray<UVehicleAccessPoint*> OutComponents;
	GetComponents<UVehicleAccessPoint>(OutComponents);
	return OutComponents;
}


//ALSADS INTERFACE
FVector ABaseVehicle::getADSTarget_Implementation()
{
	return GetSeatCameraTransform(GetSeatWithRole(EALSStationaryRole::Gunner)).GetLocation();
}

FRotator ABaseVehicle::getADSRotation_Implementation()
{
	return GetSeatCameraTransform(GetSeatWithRole(EALSStationaryRole::Gunner)).Rotator();
}

FTransform ABaseVehicle::getCameraTransform_Implementation(EALSStationaryRole StationaryRole)
{
	return GetSeatCameraTransform(GetSeatWithRole(StationaryRole));
}

FTransform ABaseVehicle::GetSeatCameraTransform(const UVehicleSeatComponent* Seat) const
{
	if (!Seat)
	{
		return GetActorTransform();
	}
	if (!Seat->CameraSocketName.IsNone() && VehicleSkeletalMesh && VehicleSkeletalMesh->DoesSocketExist(Seat->CameraSocketName))
	{
		return VehicleSkeletalMesh->GetSocketTransform(Seat->CameraSocketName);
	}
	return Seat->GetComponentTransform();
}

void ABaseVehicle::SetFormation_Implementation(ABaseFormation* Formation)
{
	OwningFormation = Formation;
}

ABaseFormation* ABaseVehicle::GetFormation_Implementation()
{
	return OwningFormation;
}

void ABaseVehicle::SetSelectionCircle_Implementation(bool Visible)
{

}

class UOrdersManager* ABaseVehicle::GetOrdersManagerComponent_Implementation()
{
	return OrdersManagerComponent;
}

bool ABaseVehicle::IsAlive_Implementation()
{
	return HP > 0.0f;
}
void ABaseVehicle::SetOwningPlayer_Implementation(AActor* Player)
{
	PlayerPtr = Player;
}

AActor* ABaseVehicle::GetOwningPlayer_Implementation()
{
	return PlayerPtr;
}

void ABaseVehicle::SetTeamIndex_Implementation(int32 Index)
{
	TeamIndex = Index;
}

int32 ABaseVehicle::GetTeamIndex_Implementation()
{
	return TeamIndex;
}

void ABaseVehicle::SetInstanceIndex_Implementation(int32 Index)
{
	SelectionCircleIndex = Index;
}

int32 ABaseVehicle::GetInstanceIndex_Implementation()
{
	return SelectionCircleIndex;
}

int ABaseVehicle::GetUnitType_Implementation()
{
	return 1;
}

//DRIVER CAMERA
FRotator ABaseVehicle::GetDriverLookRotation() const
{
	return IsLocallyControlled() ? LocalDriverLook : ReplicatedDriverLook;
}

FRotator ABaseVehicle::GetDriverLookWorldRotation() const
{
	return (VehicleMesh->GetComponentQuat() * GetDriverLookRotation().Quaternion()).Rotator();
}

FRotator ABaseVehicle::ClampDriverLook(FRotator Look) const
{
	if (bDriverFirstPerson)
	{
		Look.Yaw = FMath::Clamp(FRotator::NormalizeAxis(Look.Yaw), FirstPersonYawLimit.X, FirstPersonYawLimit.Y);
		Look.Pitch = FMath::Clamp(Look.Pitch, FirstPersonPitchLimit.X, FirstPersonPitchLimit.Y);
	}
	else
	{
		Look.Yaw = FRotator::NormalizeAxis(Look.Yaw);
		Look.Pitch = FMath::Clamp(Look.Pitch, ThirdPersonPitchLimit.X, ThirdPersonPitchLimit.Y);
	}
	Look.Roll = 0.0f;
	return Look;
}

void ABaseVehicle::AddDriverLook(float YawInput, float PitchInput)
{
	if (!IsLocallyControlled())
	{
		return;
	}
	LocalDriverLook = ClampDriverLook(LocalDriverLook + FRotator(PitchInput * DriverLookRate, YawInput * DriverLookRate, 0.0f));

	if (HasAuthority())
	{
		ReplicatedDriverLook = LocalDriverLook;
	}
	else if (!GetWorldTimerManager().IsTimerActive(DriverLookSendTimer))
	{
		// A fixed rate instead of once per input event, the timer also delivers the last value after the mouse stops
		constexpr float DriverLookSendInterval = 1.0f / 20.0f;
		GetWorldTimerManager().SetTimer(DriverLookSendTimer, this, &ABaseVehicle::SendDriverLook, DriverLookSendInterval, false);
	}
}

void ABaseVehicle::SendDriverLook()
{
	Server_SetDriverLookRotation(LocalDriverLook);
}

void ABaseVehicle::Server_SetDriverLookRotation_Implementation(FRotator NewLook)
{
	ReplicatedDriverLook = ClampDriverLook(NewLook);
}

void ABaseVehicle::CameraUpAction_Implementation(float Value)
{
	AddDriverLook(0.0f, bInvertDriverLookPitch ? -Value : Value);
}

void ABaseVehicle::CameraRightAction_Implementation(float Value)
{
	AddDriverLook(Value, 0.0f);
}

void ABaseVehicle::CameraHeldAction_Implementation()
{
	bDriverFirstPerson = !bDriverFirstPerson;
	// First person limits are tighter, bring the look back inside them
	LocalDriverLook = ClampDriverLook(LocalDriverLook);
}

void ABaseVehicle::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	AALSBaseCharacter* Driver = DriverSeat ? DriverSeat->GetOccupyingUnit() : nullptr;

	// DriverSeat->CameraSocketName on the vehicle mesh (a vision port/periscope) when configured and Use Main
	// Camera Transform is on, else the driver character's own FP_Camera head socket as before - which leaves
	// a driver unable to see much of their surroundings inside a closed hull, exactly what this is for.
	const bool bUseVehicleSocket = DriverSeat && DriverSeat->bUseMainCameraTransform && !DriverSeat->CameraSocketName.IsNone()
		&& VehicleSkeletalMesh && VehicleSkeletalMesh->DoesSocketExist(DriverSeat->CameraSocketName);
	const bool bFirstPerson = bDriverFirstPerson && Driver && (bUseVehicleSocket || Driver->GetMesh()->DoesSocketExist(FirstPersonCameraSocket));
	UpdateDriverHeadVisibility(bFirstPerson && IsLocallyControlled());

	if (bFirstPerson)
	{
		OutResult.Location = bUseVehicleSocket ? VehicleSkeletalMesh->GetSocketLocation(DriverSeat->CameraSocketName)
			: Driver->GetMesh()->GetSocketLocation(FirstPersonCameraSocket);
		OutResult.Rotation = GetDriverLookWorldRotation();
		OutResult.FOV = FirstPersonFOV;
		return;
	}

	ThirdPersonCameraArm->SetRelativeRotation(GetDriverLookRotation());
	Super::CalcCamera(DeltaTime, OutResult);
}

void ABaseVehicle::UpdateDriverHeadVisibility(bool bHide)
{
	AALSBaseCharacter* Driver = bHide && DriverSeat ? DriverSeat->GetOccupyingUnit() : nullptr;
	AALSBaseCharacter* Hidden = HiddenHeadUnit.Get();
	if (Hidden == Driver)
	{
		return;
	}
	// Local only: everybody else keeps seeing the driver's head
	if (Hidden && Hidden->HeadComponent)
	{
		Hidden->HeadComponent->SetVisibility(true, true);
	}
	if (Driver && Driver->HeadComponent)
	{
		Driver->HeadComponent->SetVisibility(false, true);
	}
	HiddenHeadUnit = Driver;
}

void ABaseVehicle::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (IsLocallyControlled())
	{
		LocalDriverLook = FRotator::ZeroRotator;
	}
	else
	{
		// CalcCamera stops running here once this machine no longer drives, show the head again
		UpdateDriverHeadVisibility(false);
	}
}

void ABaseVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UpdateDriverHeadVisibility(false);
	Super::EndPlay(EndPlayReason);
}

//SEAT SWITCHING
TArray<UVehicleSeatComponent*> ABaseVehicle::GetSortedSeats() const
{
	TArray<UVehicleSeatComponent*> Seats;
	GetComponents<UVehicleSeatComponent>(Seats);
	Seats.Sort([](const UVehicleSeatComponent& A, const UVehicleSeatComponent& B)
	{
		return A.GetName() < B.GetName();
	});
	return Seats;
}

UVehicleAccessPoint* ABaseVehicle::FindAccessPointForSeat(const UVehicleSeatComponent* Seat) const
{
	TInlineComponentArray<UVehicleAccessPoint*> AccessPoints(this);
	for (UVehicleAccessPoint* AccessPoint : AccessPoints)
	{
		for (const FComponentReference& SeatRef : AccessPoint->SeatArray)
		{
			if (SeatRef.GetComponent(const_cast<ABaseVehicle*>(this)) == Seat)
			{
				return AccessPoint;
			}
		}
	}
	return nullptr;
}

bool ABaseVehicle::IsSeatHeldByPlayer(const UVehicleSeatComponent* Seat) const
{
	const AALSBaseCharacter* Occupant = Seat ? Seat->OccupyingUnit : nullptr;
	if (!Occupant)
	{
		return false;
	}
	// A driving player controls the vehicle, not the unit in the seat
	return Occupant->IsPlayerControlled() || (Seat->SeatRole == EALSStationaryRole::Driver && IsPlayerControlled());
}

ABaseUnit* ABaseVehicle::GetPlayerUnit(const APlayerController* Player) const
{
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (Pawn == this)
	{
		const UVehicleSeatComponent* Seat = GetSeatWithRole(EALSStationaryRole::Driver);
		return Seat ? Cast<ABaseUnit>(Seat->OccupyingUnit) : nullptr;
	}
	ABaseUnit* Unit = Cast<ABaseUnit>(Pawn);
	return Unit && Unit->OccupiedSeat && Unit->OccupiedSeat->GetOwner() == this ? Unit : nullptr;
}

bool ABaseVehicle::OccupySeat(AALSBaseCharacter* Unit, UVehicleSeatComponent* Seat, UVehicleAccessPoint* ExitPoint, bool bSwitchingSeat)
{
	if (!HasAuthority() || !Unit || !Seat || Seat->GetOwner() != this || !Seat->SetOccupyingUnit(Unit))
	{
		return false;
	}

	if (ABaseUnit* BaseUnit = Cast<ABaseUnit>(Unit))
	{
		BaseUnit->OccupiedSeat = Seat;
	}
	Unit->ControlledInputInterceptor = this;
	Unit->StationaryRole = Seat->GetRole();
	if (bSwitchingSeat)
	{
		Unit->SwitchStationarySeat(Seat);
	}
	else
	{
		Unit->Server_SitCharacterDown(Seat);
	}

	AController* UnitController = Unit->GetController();
	if (UnitController && UnitController->GetClass()->ImplementsInterface(UIPlayer::StaticClass()))
	{
		IIPlayer::Execute_NotifyHUDRole(UnitController, Seat->GetRole(), Unit);
	}

	if (Seat->GetRole() == EALSStationaryRole::Driver)
	{
		// A player drives the vehicle himself, for an AI crew the vehicle's own AI controller does
		if (APlayerController* PlayerController = Cast<APlayerController>(UnitController))
		{
			PlayerController->Possess(this);
		}
		else if (GetController() != BackupAIController)
		{
			PossessByAIController();
		}
	}

	Unit->Client_SetUseStationaryADS(Seat->GetUseADS());
	Unit->Client_SetUseMainCameraTransform(Seat->GetUseMainCameraTransform());

	// One access point can serve several seats, the seat remembers which point its unit leaves through
	if (UVehicleAccessPoint* ExitAccessPoint = ExitPoint ? ExitPoint : FindAccessPointForSeat(Seat))
	{
		Seat->ExitDelegate.BindUObject(ExitAccessPoint, &UVehicleAccessPoint::Exit);
	}
	return true;
}

void ABaseVehicle::VacateSeatForSwitch(AALSBaseCharacter* Unit)
{
	ABaseUnit* BaseUnit = Cast<ABaseUnit>(Unit);
	UVehicleSeatComponent* Seat = BaseUnit ? BaseUnit->OccupiedSeat.Get() : nullptr;
	if (!Seat)
	{
		return;
	}

	if (UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(Seat))
	{
		GunnerSeat->SetWeaponArrayOwner(nullptr);
		Unit->ActiveWeaponActor = nullptr;
	}
	if (Unit->GunnerWeaponChangedDelegate.IsBound())
	{
		Unit->GunnerWeaponChangedDelegate.Unbind();
	}
	// Lets go of held inputs of the old role (driving, standing gunner aim)
	Unit->HandleExitControlledInterceptor();
	Unit->ExitDelegate.Unbind();
	Seat->ExitDelegate.Unbind();
	Seat->ClearOccupyingUnit();
	BaseUnit->OccupiedSeat = nullptr;
}

bool ABaseVehicle::ServerSwitchSeat(ABaseUnit* Unit, UVehicleSeatComponent* TargetSeat)
{
	if (!HasAuthority() || !Unit || !TargetSeat || TargetSeat->GetOwner() != this)
	{
		return false;
	}
	UVehicleSeatComponent* CurrentSeat = Unit->OccupiedSeat;
	if (!CurrentSeat || CurrentSeat->GetOwner() != this || CurrentSeat == TargetSeat || IsSeatHeldByPlayer(TargetSeat))
	{
		return false;
	}
	AALSBaseCharacter* OtherUnit = TargetSeat->GetOccupyingUnit();
	// Free but booked: a unit is on its way in
	if (!OtherUnit && !TargetSeat->IsSeatAvailable())
	{
		return false;
	}

	// Leaving the driver seat: the player takes its unit back, the vehicle's AI takes the vehicle
	APlayerController* PlayerController = Cast<APlayerController>(Unit->GetController());
	if (!PlayerController && CurrentSeat->GetRole() == EALSStationaryRole::Driver)
	{
		PlayerController = Cast<APlayerController>(GetController());
		if (PlayerController && PlayerController->GetPawn() == this)
		{
			PlayerController->Possess(Unit);
			PossessByAIController();
		}
	}

	UVehicleAccessPoint* CurrentExit = FindAccessPointForSeat(CurrentSeat);
	UVehicleAccessPoint* TargetExit = FindAccessPointForSeat(TargetSeat);

	VacateSeatForSwitch(Unit);
	if (OtherUnit)
	{
		VacateSeatForSwitch(OtherUnit);
	}
	OccupySeat(Unit, TargetSeat, TargetExit, true);
	if (OtherUnit)
	{
		OccupySeat(OtherUnit, CurrentSeat, CurrentExit, true);
	}
	return true;
}

void ABaseVehicle::GetSelectionMenuOptions_Implementation(APlayerController* Player, TArray<FSelectionMenuOption>& Options, int32& CurrentIndex)
{
	const ABaseUnit* Unit = GetPlayerUnit(Player);
	const UVehicleSeatComponent* CurrentSeat = Unit ? Unit->OccupiedSeat.Get() : nullptr;
	CurrentIndex = INDEX_NONE;

	const TArray<UVehicleSeatComponent*> Seats = GetSortedSeats();
	for (int32 Index = 0; Index < Seats.Num(); ++Index)
	{
		UVehicleSeatComponent* Seat = Seats[Index];
		FSelectionMenuOption& Option = Options.AddDefaulted_GetRef();
		Option.Label = FText::FromString(Seat->GetName());
		// Free seats and seats of AI units, not the own seat and not seats of other players
		Option.bEnabled = Seat != CurrentSeat && !IsSeatHeldByPlayer(Seat) && (Seat->OccupyingUnit || Seat->IsSeatAvailable());
		if (Seat == CurrentSeat)
		{
			CurrentIndex = Index;
		}
	}
}

void ABaseVehicle::HandleSelectionMenuChoice_Implementation(APlayerController* Player, int32 Index)
{
	const TArray<UVehicleSeatComponent*> Seats = GetSortedSeats();
	if (Seats.IsValidIndex(Index))
	{
		ServerSwitchSeat(GetPlayerUnit(Player), Seats[Index]);
	}
}

//TRACKED VEHICLES
void ABaseVehicle::BuildTrackWheelArrays()
{
	LeftTrackWheels.Reset();
	RightTrackWheels.Reset();
	for (UAVS_Wheel* Wheel : GetWheels())
	{
		if (!IsValid(Wheel))
		{
			continue;
		}
		const FString BoneString = Wheel->BoneName.ToString();
		if (BoneString.StartsWith(TEXT("lf_")))
		{
			LeftTrackWheels.Add(Wheel);
		}
		else if (BoneString.StartsWith(TEXT("rt_")))
		{
			RightTrackWheels.Add(Wheel);
		}
	}

	// Front-to-back order regardless of component creation order, so locking "the lead wheels" below is stable
	auto ParseAxle = [](const auto& Wheel)
	{
		FString AxleString = Wheel.BoneName.ToString();
		if (!AxleString.RemoveFromEnd(TEXT("_fake_jnt")))
		{
			AxleString.RemoveFromEnd(TEXT("_jnt"));
		}
		AxleString.RemoveFromStart(TEXT("lf_wheel_"));
		AxleString.RemoveFromStart(TEXT("rt_wheel_"));
		return AxleString.IsNumeric() ? FCString::Atoi(*AxleString) : 0;
	};
	auto SortByAxle = [ParseAxle](const auto& A, const auto& B)
	{
		return ParseAxle(A) < ParseAxle(B);
	};
	LeftTrackWheels.Sort(SortByAxle);
	RightTrackWheels.Sort(SortByAxle);
}

void ABaseVehicle::SetTrackLockCount(const TArray<TObjectPtr<UAVS_Wheel>>& Side, int32 LockCount)
{
	for (int32 Index = 0; Index < Side.Num(); ++Index)
	{
		if (UAVS_Wheel* Wheel = Side[Index].Get())
		{
			Wheel->LockWheel(Index < LockCount);
		}
	}
}

void ABaseVehicle::UpdateTrackSteeringLock(float Turn)
{
	// Turn > 0 (turning right) locks the right side's lead wheels while the left side keeps driving, same
	// convention as a real tracked vehicle's brake-steer. The locked count grows with how hard the turn is,
	// so a gentle turn only brakes the lead wheels instead of snapping the whole side to a stop at once.
	const int32 LeftLock = Turn < 0.0f ? FMath::CeilToInt(-Turn * LeftTrackWheels.Num()) : 0;
	const int32 RightLock = Turn > 0.0f ? FMath::CeilToInt(Turn * RightTrackWheels.Num()) : 0;
	SetTrackLockCount(LeftTrackWheels, LeftLock);
	SetTrackLockCount(RightTrackWheels, RightLock);
}

float ABaseVehicle::GetReliableWheelAngVel(bool bRight) const
{
	const TArray<TObjectPtr<UAVS_Wheel>>& Side = bRight ? RightTrackWheels : LeftTrackWheels;
	// Excludes the first/last wheel (sprocket/idler, see GetWheelSpinRotation for why its own reading can get
	// permanently stuck at exactly zero) - a tracked vehicle's wheels are all mechanically forced to the same
	// surface speed by the track regardless, so excluding two of them from the average loses nothing, and
	// averaging a stuck one in would otherwise drag the whole side's speed toward zero too.
	const int32 FirstIndex = Side.Num() > 2 ? 1 : 0;
	const int32 LastIndex = Side.Num() > 2 ? Side.Num() - 2 : Side.Num() - 1;
	if (LastIndex < FirstIndex)
	{
		return 0.0f;
	}
	float Total = 0.0f;
	int32 Count = 0;
	for (int32 Index = FirstIndex; Index <= LastIndex; ++Index)
	{
		if (UAVS_Wheel* Wheel = Side[Index].Get())
		{
			// The wheel's actual simulated angular velocity: already reflects slip, a locked wheel, and reverse
			Total += Wheel->GetWheelAngVelInRadians();
			++Count;
		}
	}
	return Count > 0 ? Total / Count : 0.0f;
}

float ABaseVehicle::GetTrackSpeed(bool bRight) const
{
	return GetReliableWheelAngVel(bRight) * TrackScrollSpeedScale;
}

FQuat ABaseVehicle::GetWheelSpinRotation(const UAVS_Wheel* Wheel, const FQuat& RawSpinRotation) const
{
	if (!bTracked || !Wheel)
	{
		return RawSpinRotation;
	}
	const bool bRight = Wheel->BoneName.ToString().StartsWith(TEXT("rt_"));
	const TArray<TObjectPtr<UAVS_Wheel>>& Side = bRight ? RightTrackWheels : LeftTrackWheels;
	if (Side.Num() < 2 || (Side[0] != Wheel && Side.Last() != Wheel))
	{
		return RawSpinRotation;
	}
	return FQuat(FRotator(bRight ? RightFixedWheelSpinAngle : LeftFixedWheelSpinAngle, 0.0f, 0.0f));
}

void ABaseVehicle::UpdateTrackVisuals()
{
	if (!bTracked || !VehicleSkeletalMesh || IsRunningDedicatedServer())
	{
		return;
	}

	if (LeftTrackMaterialSlot != NAME_None && !LeftTrackMID)
	{
		const int32 SlotIndex = VehicleSkeletalMesh->GetMaterialIndex(LeftTrackMaterialSlot);
		if (SlotIndex != INDEX_NONE)
		{
			LeftTrackMID = VehicleSkeletalMesh->CreateDynamicMaterialInstance(SlotIndex);
		}
	}
	if (RightTrackMaterialSlot != NAME_None && !RightTrackMID)
	{
		const int32 SlotIndex = VehicleSkeletalMesh->GetMaterialIndex(RightTrackMaterialSlot);
		if (SlotIndex != INDEX_NONE)
		{
			RightTrackMID = VehicleSkeletalMesh->CreateDynamicMaterialInstance(SlotIndex);
		}
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	// Wrapped well clear of any texture tiling so it never visibly jumps, just keeps float precision sane
	LeftTrackOffset = FMath::Fmod(LeftTrackOffset + GetTrackSpeed(false) * DeltaSeconds, 1000000.0f);
	RightTrackOffset = FMath::Fmod(RightTrackOffset + GetTrackSpeed(true) * DeltaSeconds, 1000000.0f);

	// GetWheelSpinRotation's substitute for the first/last wheel - same accumulation AVS itself does for a
	// wheel's own spin (UAVS_Wheel::TickComponent: degrees, negated, wrapped to stay bounded over a long drive)
	constexpr float RadsToDegreesPerSecond = 180.0f / PI;
	LeftFixedWheelSpinAngle = FMath::Fmod(LeftFixedWheelSpinAngle + GetReliableWheelAngVel(false) * RadsToDegreesPerSecond * -1.0f * DeltaSeconds, 360.0f);
	RightFixedWheelSpinAngle = FMath::Fmod(RightFixedWheelSpinAngle + GetReliableWheelAngVel(true) * RadsToDegreesPerSecond * -1.0f * DeltaSeconds, 360.0f);

	if (LeftTrackMID)
	{
		LeftTrackMID->SetScalarParameterValue(TrackScrollParameterName, LeftTrackOffset);
	}
	if (RightTrackMID)
	{
		RightTrackMID->SetScalarParameterValue(TrackScrollParameterName, RightTrackOffset);
	}
}

void ABaseVehicle::AVS_AlwaysTick()
{
	Super::AVS_AlwaysTick();
	UpdateTrackVisuals();
	UpdateSurfaceDrivingModifiers();
}

void ABaseVehicle::UpdateSurfaceDrivingModifiers()
{
	if (VehicleWheels.Num() == 0)
	{
		return;
	}

	// Read from the CDO rather than this instance: SurfaceDrivingModifiers is EditDefaultsOnly (vehicle-type
	// tuning, not meant to vary per placed instance), but a stale per-instance override left over from before
	// it was configured can still exist and isn't clearable through the editor's per-instance tools for an
	// EditDefaultsOnly property (same class of bug as a stale SteerableAxles override earlier) - going to the
	// CDO sidesteps that instead of relying on every instance staying in sync with it.
	const ABaseVehicle* DefaultVehicle = GetClass()->GetDefaultObject<ABaseVehicle>();
	for (UAVS_Wheel* Wheel : VehicleWheels)
	{
		if (!IsValid(Wheel) || Wheel->GetWheelMode() != EWheelMode::Raycast)
		{
			continue;
		}
		const TEnumAsByte<EPhysicalSurface> ContactSurface = Wheel->GetContactSurfaceType();
		const FVehicleSurfaceDrivingModifier* Modifier = DefaultVehicle->SurfaceDrivingModifiers.Find(ContactSurface);
		const FVehicleSurfaceDrivingModifier& Mod = Modifier ? *Modifier : DefaultVehicle->DefaultSurfaceDrivingModifier;

		// Re-derived from DefaultWheelConfig every time rather than compounded onto the wheel's current
		// values, so switching surfaces back and forth can't drift the numbers away from their tuned baseline
		Wheel->WheelConfig.RollingResistance = DefaultVehicle->DefaultWheelConfig.RollingResistance * Mod.RollingResistanceMultiplier;
		Wheel->WheelConfig.TireFriction = DefaultVehicle->DefaultWheelConfig.TireFriction * Mod.TireFrictionMultiplier;
	}
}
