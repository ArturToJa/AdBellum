// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "BaseVehicleAnimInstance.generated.h"

class ABaseVehicle;
class UGunnerSeatComponent;

// Change applied to one bone on top of the reference pose, in skeletal mesh component space
struct FVehicleBoneOffset
{
	FName BoneName = NAME_None;
	FVector Translation = FVector::ZeroVector;
	// Turns the bone around its own origin
	FQuat Rotation = FQuat::Identity;
	// Extra turn around one of the bone's own axes (steering wheel), applied after Rotation
	bool bTurnAroundBoneAxis = false;
	EAxis::Type BoneAxis = EAxis::X;
	float BoneAxisAngle = 0.0f; // radians
};

// Builds the pose on the animation thread: reference pose plus BoneOffsets, no anim graph involved
struct FBaseVehicleAnimInstanceProxy : public FAnimInstanceProxy
{
	FBaseVehicleAnimInstanceProxy() = default;
	FBaseVehicleAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}

	virtual bool Evaluate(FPoseContext& Output) override;

	// Written by UBaseVehicleAnimInstance on the game thread
	TArray<FVehicleBoneOffset> BoneOffsets;
};

// Animation of a vehicle skeletal mesh, done fully in C++ (assign the class itself as the mesh's Anim Class):
// - wheel bones follow suspension, spin and steering of the raycast AVS wheels that name them (UAVS_Wheel::BoneName)
// - turret / gun bones follow the aim of every gunner seat (UGunnerSeatComponent::TurretBoneName / GunBoneName)
// Everything is applied in component space, so it does not depend on how the model's bones are oriented.
// The pose is produced natively: the graph of an anim blueprint derived from this class is not evaluated.
UCLASS()
class ADBELLUM_API UBaseVehicleAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// How fast turrets of other players and AI follow their replicated aim, 0 - no smoothing.
	// The gunner's own machine and the server always use the exact aim.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim", meta = (ClampMin = "0.0"))
	float AimInterpSpeed = 15.0f;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	void AddWheelOffsets(const ABaseVehicle* Vehicle, const FTransform& MeshTransform, TArray<FVehicleBoneOffset>& OutOffsets) const;

	void AddAimOffsets(const ABaseVehicle* Vehicle, const FTransform& MeshTransform, float DeltaSeconds, TArray<FVehicleBoneOffset>& OutOffsets);

	void AddSteeringWheelOffsets(ABaseVehicle* Vehicle, TArray<FVehicleBoneOffset>& OutOffsets) const;

	TWeakObjectPtr<ABaseVehicle> OwningVehicle;

	// Aim (X - yaw, Y - pitch) currently shown for every gunner seat
	TMap<TWeakObjectPtr<const UGunnerSeatComponent>, FVector2D> ShownAim;
};
