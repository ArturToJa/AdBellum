// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Chaos/ChaosEngineInterface.h"
#include "VehicleStructs.generated.h"

class ABaseVehicle;

USTRUCT(BlueprintType)
struct ADBELLUM_API FVehicleData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<ABaseVehicle> VehicleClass;
};

// One wheel of a vehicle: the bone it sits on and what it does. Suspension, tire and brake tuning
// is shared by all wheels of the vehicle (ABaseVehicle::DefaultWheelConfig).
USTRUCT(BlueprintType)
struct ADBELLUM_API FVehicleWheelSetup
{
	GENERATED_BODY()

public:
	// Wheel bone of the vehicle skeletal mesh, standard names are lf_wheel_N_jnt / rt_wheel_N_jnt. This is
	// the bone that spins and steers; on rigs that animate it, it is also the one AVS_Wheel raycasts from.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	FName BoneName = NAME_None;

	// Optional: a separate bone (normally BoneName's parent) that receives the suspension height instead of
	// BoneName, which then only spins/steers. Needed on rigs where the wheel's own bone has no vertical
	// travel skinned to it and a sibling bone (e.g. Vigilante's lf_wheel_N_root_jnt) carries the track/hull
	// deformation instead. Leave empty for rigs where BoneName alone handles both (suspension and spin).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	FName SuspensionBoneName = NAME_None;

	// Optional, only used together with SuspensionBoneName: the hull-mounted bone a real suspension arm
	// would hinge from (e.g. Vigilante's lf_wheel_N_hidr_chasing_jnt). When set, SuspensionBoneName is swung
	// around this bone's reference position instead of moved in a straight line, closer to how a real arm
	// moves and more accurate the more the wheel travels. Leave empty for a straight-line approximation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	FName SuspensionPivotBoneName = NAME_None;

	// Only used together with SuspensionPivotBoneName: how far the arm swing is allowed to move the
	// suspension bone fore/aft (cm), clamped on top of the arc. Real trailing arms do swing fore/aft as they
	// travel (that part is correct), but their travel is itself limited (bump stops, shock absorbers, the
	// torsion bar reaching its angle limit); this stands in for that limit when the pivot bone's distance
	// from the wheel doesn't already keep the swing believable on its own.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	float MaxSuspensionArcSwing = 8.0f;

	// Wheel receives torque from the drivetrain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	bool bDriving = true;

	// Reverse direction of torque
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (EditCondition = "bDriving"))
	bool bInvertTorque = false;

	// Wheel turns with the steering input
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	bool bSteerable = false;

	// Steers against the front wheels (rear axle steering)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (EditCondition = "bSteerable"))
	bool bInvertSteering = false;

	// Wheel stops with the handbrake
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	bool bHandbrake = false;

	// No suspension travel (forces the created AVS_Wheel's Spring Length to 0): a tracked vehicle's first/last
	// wheel (sprocket/idler) always sits in the same place, it does not bounce like the road wheels between them.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	bool bFixedSuspension = false;
};

// How a vehicle drives on one kind of ground, relative to its Default Wheel Config (ABaseVehicle::UpdateSurfaceDrivingModifiers,
// read every tick per-wheel from UAVS_Wheel::GetContactSurfaceType - the same ground reading the wheel VFX surface
// effects already use, so this lines up with whatever surface those already react to).
USTRUCT(BlueprintType)
struct ADBELLUM_API FVehicleSurfaceDrivingModifier
{
	GENERATED_BODY()

public:
	// Multiplies Default Wheel Config's Rolling Resistance on this surface. AVS applies rolling resistance as a
	// standing brake every tick, so this is what makes soft ground actually slow the vehicle down (lower
	// achievable top speed and acceleration) rather than just changing how it steers - e.g. mud bogging a
	// tracked vehicle down even though its tracks still grip it fine.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float RollingResistanceMultiplier = 1.0f;

	// Multiplies Default Wheel Config's Tire Friction on this surface, on top of the physical material's own
	// Friction (which AVS already factors in per wheel). Use this for a difference AVS's own surface friction
	// doesn't already capture - e.g. tracks keeping meaningfully more grip on soft ground than wheels would.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	float TireFrictionMultiplier = 1.0f;
};
