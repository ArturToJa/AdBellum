// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "VehicleCrewAnimInstance.generated.h"

class AALSBaseCharacter;

// Parent class of the anim blueprint used while a unit sits in a vehicle (ABP_CharacterVehicle).
// Gathers everything the anim graph needs once per frame on the game thread; the graph (seat pose,
// steering blendspace, DragonIK look and arm solvers) only reads these values.
// Hand targets come from the seat the unit sits in, so every gunner holds its own weapon.
UCLASS()
class ADBELLUM_API UVehicleCrewAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Seat role of the unit, read every frame (it can replicate after this anim instance starts)
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Crew")
	EALSStationaryRole CrewRole = EALSStationaryRole::None;

	// Vehicle steering mapped to the steering blendspace input (-SteeringBlendRange .. SteeringBlendRange)
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Crew")
	float SteeringBlend = 0.0f;

	// Gunner_Standing seat that keeps the unit sitting (UGunnerSeatComponent::bSittingPose)
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Crew")
	bool bSittingPose = false;

	// 1 when the seat gives hand targets (weapon grips, steering wheel), 0 otherwise
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Crew")
	float HandIKAlpha = 0.0f;

	// World space hand targets for the DragonIK Twist Arm solvers
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Crew")
	FTransform LeftHandTarget = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Crew")
	FTransform RightHandTarget = FTransform::Identity;

	// World space look target for the DragonIK aim solver
	UPROPERTY(BlueprintReadOnly, Category = "Vehicle Crew")
	FTransform LookAtTarget = FTransform::Identity;

	// Range of the steering blendspace input (CIA_Steering: -75 .. 75)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle Crew")
	float SteeringBlendRange = 75.0f;

	// How far ahead of the unit the head looks, driver excluded (it looks far down the road)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle Crew")
	float LookAtDistance = 500.0f;

private:
	TWeakObjectPtr<AALSBaseCharacter> Unit;
};
