// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
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
	// Wheel bone of the vehicle skeletal mesh, standard names are lf_wheel_N_jnt / rt_wheel_N_jnt
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel")
	FName BoneName = NAME_None;

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
};
