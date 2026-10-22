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
	const ABaseVehicle* Vehicle = OwningVehicle.Get();
	const USkeletalMeshComponent* Mesh = GetSkelMeshComponent();
	if (!Vehicle || !Mesh || !Vehicle->GetInitializationState())
	{
		return;
	}

	const FTransform MeshTransform = Mesh->GetComponentTransform();
	AddWheelOffsets(Vehicle, MeshTransform, BoneOffsets);
	AddAimOffsets(Vehicle, MeshTransform, DeltaSeconds, BoneOffsets);
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
		// spin and steering in the rotation. The bone repeats that movement, expressed in mesh component space.
		const FTransform WheelMovement = WheelMesh->GetRelativeTransform();
		const FTransform WheelTransform = Wheel->GetComponentTransform();
		const FQuat WheelToMesh = MeshTransform.GetRotation().Inverse() * WheelTransform.GetRotation();

		FVehicleBoneOffset& Offset = OutOffsets.AddDefaulted_GetRef();
		Offset.BoneName = Wheel->BoneName;
		Offset.Translation = MeshTransform.InverseTransformVector(WheelTransform.TransformVector(WheelMovement.GetLocation()));
		Offset.Rotation = WheelToMesh * WheelMovement.GetRotation() * WheelToMesh.Inverse();
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
