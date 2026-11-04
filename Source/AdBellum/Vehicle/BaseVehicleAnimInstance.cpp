// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseVehicleAnimInstance.h"
#include "BaseVehicle.h"
#include "GunnerSeatComponent.h"
#include "_Core/AVS_Wheel.h"
#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"

bool FBaseVehicleAnimInstanceProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();
	if (BoneOffsets.Num() == 0)
	{
		return true;
	}

	// Bones missing from the skeleton or removed by the current LOD are skipped
	const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();
	TArray<TPair<FCompactPoseBoneIndex, const FVehicleBoneOffset*>, TInlineAllocator<32>> Bones;
	for (const FVehicleBoneOffset& Offset : BoneOffsets)
	{
		const int32 MeshBoneIndex = BoneContainer.GetPoseBoneIndexForBoneName(Offset.BoneName);
		if (MeshBoneIndex == INDEX_NONE)
		{
			// Names each bone an offset targeted but that the current pose doesn't have, so a silently-dropped
			// offset (missing bone, or stripped by the active LOD) shows up in the log instead of just quietly
			// not animating. Logged once per distinct bone name per run.
			static TSet<FName> LoggedMissingBones;
			if (!LoggedMissingBones.Contains(Offset.BoneName))
			{
				LoggedMissingBones.Add(Offset.BoneName);
				UE_LOG(LogTemp, Warning, TEXT("FBaseVehicleAnimInstanceProxy::Evaluate - bone '%s' not found in the current pose (missing from skeleton, or stripped by the active LOD) - its offset is being skipped"), *Offset.BoneName.ToString());
			}
			continue;
		}
		const FCompactPoseBoneIndex BoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshBoneIndex));
		if (BoneIndex.IsValid())
		{
			Bones.Emplace(BoneIndex, &Offset);
		}
	}
	if (Bones.Num() == 0)
	{
		return true;
	}

	// Parents first (a parent always has the lower index), so a gun bone picks up the turn of its turret bone
	Bones.Sort([](const TPair<FCompactPoseBoneIndex, const FVehicleBoneOffset*>& A, const TPair<FCompactPoseBoneIndex, const FVehicleBoneOffset*>& B)
	{
		return A.Key.GetInt() < B.Key.GetInt();
	});

	FCSPose<FCompactPose> ComponentPose;
	ComponentPose.InitPose(Output.Pose);
	for (const TPair<FCompactPoseBoneIndex, const FVehicleBoneOffset*>& Bone : Bones)
	{
		FTransform BoneTransform = ComponentPose.GetComponentSpaceTransform(Bone.Key);
		BoneTransform.SetRotation((Bone.Value->Rotation * BoneTransform.GetRotation()).GetNormalized());
		if (Bone.Value->bTurnAroundBoneAxis)
		{
			const FVector Axis = BoneTransform.GetUnitAxis(Bone.Value->BoneAxis);
			BoneTransform.SetRotation((FQuat(Axis, Bone.Value->BoneAxisAngle) * BoneTransform.GetRotation()).GetNormalized());
		}
		BoneTransform.AddToTranslation(Bone.Value->Translation);
		ComponentPose.SetComponentSpaceTransform(Bone.Key, BoneTransform);
	}
	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPosesSafe(ComponentPose, Output.Pose);
	return true;
}

FAnimInstanceProxy* UBaseVehicleAnimInstance::CreateAnimInstanceProxy()
{
	return new FBaseVehicleAnimInstanceProxy(this);
}

void UBaseVehicleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningVehicle = Cast<ABaseVehicle>(GetOwningActor());
	ShownAim.Reset();
}

void UBaseVehicleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	TArray<FVehicleBoneOffset>& BoneOffsets = GetProxyOnGameThread<FBaseVehicleAnimInstanceProxy>().BoneOffsets;
	BoneOffsets.Reset();

	// Reference pose until the vehicle is initialized (also in the editor): wheels are placed on their bones
	// during construction, animating before that would move the bones they are being placed on
	ABaseVehicle* Vehicle = OwningVehicle.Get();
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Vehicle || !Mesh || !Vehicle->GetInitializationState())
	{
		return;
	}

	const FTransform MeshTransform = Mesh->GetComponentTransform();
	AddWheelOffsets(Vehicle, MeshTransform, BoneOffsets);
	AddAimOffsets(Vehicle, MeshTransform, DeltaSeconds, BoneOffsets);
	AddSteeringWheelOffsets(Vehicle, BoneOffsets);
}

void UBaseVehicleAnimInstance::AddWheelOffsets(const ABaseVehicle* Vehicle, const FTransform& MeshTransform, TArray<FVehicleBoneOffset>& OutOffsets) const
{
	for (const UAVS_Wheel* Wheel : Vehicle->VehicleWheels)
	{
		// Physics wheels move their bone themselves (Connect To Bone)
		if (!IsValid(Wheel) || Wheel->BoneName.IsNone() || Wheel->GetWheelMode() != EWheelMode::Raycast)
		{
			continue;
		}
		const UPrimitiveComponent* WheelMesh = Wheel->GetWheelMesh();
		if (!WheelMesh)
		{
			continue;
		}

		// AVS moves the wheel's own mesh relative to the wheel component: suspension travel in the location,
		// spin and steering in the rotation, expressed in mesh component space. Rigs with a separate
		// suspension bone (parent of the spin bone, e.g. Vigilante's _root_jnt) get the translation there
		// instead, so the track drapes over it correctly; the spin bone then only turns, picking up the
		// moved position from its parent the same way a gun bone picks up its turret's turn (see Evaluate).
		const FTransform WheelMovement = WheelMesh->GetRelativeTransform();
		const FTransform WheelTransform = Wheel->GetComponentTransform();
		const FQuat WheelToMesh = MeshTransform.GetRotation().Inverse() * WheelTransform.GetRotation();

		const FVector RawTranslation = MeshTransform.InverseTransformVector(WheelTransform.TransformVector(WheelMovement.GetLocation()));
		// Normally WheelMovement's own rotation; substituted for a tracked vehicle's first/last wheel - see
		// GetWheelSpinRotation for why AVS's own reading for one of those can get permanently stuck
		const FQuat SpinRotation = Vehicle->GetWheelSpinRotation(Wheel, WheelMovement.GetRotation());
		const FQuat Rotation = WheelToMesh * SpinRotation * WheelToMesh.Inverse();

		FVector SuspensionTranslation;
		FName PivotBone;
		FQuat PivotRotation;
		const FName SuspensionBone = Vehicle->GetWheelSuspensionTranslation(Wheel->BoneName, RawTranslation, SuspensionTranslation, PivotBone, PivotRotation);
		if (!SuspensionBone.IsNone())
		{
			FVehicleBoneOffset& SuspensionOffset = OutOffsets.AddDefaulted_GetRef();
			SuspensionOffset.BoneName = SuspensionBone;
			SuspensionOffset.Translation = SuspensionTranslation;

			FVehicleBoneOffset& SpinOffset = OutOffsets.AddDefaulted_GetRef();
			SpinOffset.BoneName = Wheel->BoneName;
			SpinOffset.Rotation = Rotation;

			// Cosmetic: swings a visible strut/rod rigidly childed to the pivot (e.g. BMP2's _hidr_rod_jnt)
			// with the wheel via normal FK - not itself skinned to anything, so it needs no translation, only
			// this rotation on its parent (see GetWheelSuspensionTranslation).
			if (!PivotBone.IsNone())
			{
				FVehicleBoneOffset& PivotOffset = OutOffsets.AddDefaulted_GetRef();
				PivotOffset.BoneName = PivotBone;
				PivotOffset.Rotation = PivotRotation;
			}
		}
		else
		{
			FVehicleBoneOffset& Offset = OutOffsets.AddDefaulted_GetRef();
			Offset.BoneName = Wheel->BoneName;
			// A fixed-suspension wheel (sprocket/idler) has no separate suspension bone to put translation on
			// (that's why we're in this branch at all) and no animated track bone around it either - its
			// spring isn't literally rigid any more (see BuildWheelsFromSetup), so without this a tiny
			// residual compression would visibly move the wheel relative to the track geometry around it,
			// which stays fixed in its reference pose for these wheels.
			Offset.Translation = Vehicle->IsFixedSuspensionWheel(Wheel->BoneName) ? FVector::ZeroVector : RawTranslation;
			Offset.Rotation = Rotation;
		}
	}
}

void UBaseVehicleAnimInstance::AddAimOffsets(const ABaseVehicle* Vehicle, const FTransform& MeshTransform, float DeltaSeconds, TArray<FVehicleBoneOffset>& OutOffsets)
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	const USceneComponent* VehicleRoot = Vehicle->GetRootComponent();
	if (!VehicleRoot)
	{
		return;
	}

	// Vehicle axes in mesh component space, the skeletal mesh is often turned relative to the vehicle
	const FQuat VehicleToMesh = MeshTransform.GetRotation().Inverse() * VehicleRoot->GetComponentQuat();
	const FVector UpAxis = VehicleToMesh.GetAxisZ();
	const FVector RightAxis = VehicleToMesh.GetAxisY();

	for (const UGunnerSeatComponent* Seat : Vehicle->GetGunnerSeats())
	{
		const bool bHasTurretBone = !Seat->TurretBoneName.IsNone() && Mesh->GetBoneIndex(Seat->TurretBoneName) != INDEX_NONE;
		const bool bHasGunBone = !Seat->GunBoneName.IsNone() && Mesh->GetBoneIndex(Seat->GunBoneName) != INDEX_NONE;
		if (!bHasTurretBone && !bHasGunBone)
		{
			continue;
		}

		const FVector2D TargetAim = Seat->GetAim();
		FVector2D& Aim = ShownAim.FindOrAdd(Seat, TargetAim);
		if (AimInterpSpeed <= 0.0f || Seat->IsLocallyAimed() || Vehicle->HasAuthority())
		{
			Aim = TargetAim;
		}
		else
		{
			// Yaw takes the short way around
			Aim.X = FRotator::NormalizeAxis(Aim.X + FMath::FInterpTo(0.0f, FMath::FindDeltaAngleDegrees(Aim.X, TargetAim.X), DeltaSeconds, AimInterpSpeed));
			Aim.Y = FMath::FInterpTo(Aim.Y, TargetAim.Y, DeltaSeconds, AimInterpSpeed);
		}

		// Positive pitch raises the gun: the same turn as FRotator pitch, which is a negative angle around the right axis
		const FQuat Yaw(UpAxis, FMath::DegreesToRadians(Aim.X));
		const FQuat Pitch(Yaw.RotateVector(RightAxis), -FMath::DegreesToRadians(Aim.Y));

		if (bHasTurretBone)
		{
			FVehicleBoneOffset& Offset = OutOffsets.AddDefaulted_GetRef();
			Offset.BoneName = Seat->TurretBoneName;
			Offset.Rotation = Yaw;
		}
		if (bHasGunBone)
		{
			// Below a turret bone the gun already turns with it, a gun without turret bone does both turns itself
			FVehicleBoneOffset& Offset = OutOffsets.AddDefaulted_GetRef();
			Offset.BoneName = Seat->GunBoneName;
			Offset.Rotation = bHasTurretBone ? Pitch : Pitch * Yaw;
		}
	}
}

void UBaseVehicleAnimInstance::AddSteeringWheelOffsets(ABaseVehicle* Vehicle, TArray<FVehicleBoneOffset>& OutOffsets) const
{
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	TInlineComponentArray<UVehicleSeatComponent*> Seats(Vehicle);
	for (const UVehicleSeatComponent* Seat : Seats)
	{
		if (!Seat->bHasSteeringWheel || Seat->SteeringWheelBoneName.IsNone() || Mesh->GetBoneIndex(Seat->SteeringWheelBoneName) == INDEX_NONE)
		{
			continue;
		}

		static const EAxis::Type Axes[] = { EAxis::X, EAxis::Y, EAxis::Z };
		FVehicleBoneOffset& Offset = OutOffsets.AddDefaulted_GetRef();
		Offset.BoneName = Seat->SteeringWheelBoneName;
		Offset.bTurnAroundBoneAxis = true;
		Offset.BoneAxis = Axes[static_cast<uint8>(Seat->SteeringWheelAxis)];
		// Actual steering (smoothed by AVS), the same value on every machine
		Offset.BoneAxisAngle = FMath::DegreesToRadians(Vehicle->GetCurrentSteering() * Seat->SteeringWheelMaxAngle);
	}
}
