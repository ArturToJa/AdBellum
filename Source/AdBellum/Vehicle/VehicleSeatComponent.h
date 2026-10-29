// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "VehicleSeatComponent.generated.h"

class AALSBaseCharacter;

DECLARE_DELEGATE_OneParam(OnExitSeatDelegate, AALSBaseCharacter*)

// Axis of a bone in its own space
UENUM(BlueprintType)
enum class EVehicleBoneAxis : uint8
{
	X,
	Y,
	Z
};

/**
 * 
 */
UCLASS(DefaultToInstanced, ClassGroup = (VehicleComponent), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API UVehicleSeatComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()
public:
	UVehicleSeatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EALSStationaryRole SeatRole;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bUseADS;
	OnExitSeatDelegate ExitDelegate;

	//OnGunnerWeaponChanged GunnerWeaponChanged;


	UPROPERTY(Replicated,EditAnywhere, BlueprintReadOnly)
	AALSBaseCharacter* OccupyingUnit = nullptr;
	virtual bool SetOccupyingUnit(AALSBaseCharacter* InUnit);
	AALSBaseCharacter* GetOccupyingUnit();

	void BookSeat(AALSBaseCharacter* InUnit);
	void ClearBookedSeat();
	void ClearOccupyingUnit();
	bool IsSeatAvailable();
	bool IsSeatBookedFor(AALSBaseCharacter* InUnit);
	EALSStationaryRole GetRole();
	bool GetUseADS();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bUseMainCameraTransform;

	bool GetUseMainCameraTransform();
	void ExitSeat();

	// STEERING WHEEL

	// The seat's steering wheel turns with the vehicle steering and the unit's hands hold it
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steering Wheel")
	bool bHasSteeringWheel = false;

	// Bone of the vehicle skeletal mesh turned by the steering
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steering Wheel", meta = (EditCondition = "bHasSteeringWheel"))
	FName SteeringWheelBoneName = TEXT("steering_wheel_jnt");

	// Axis of the bone (in its own space) the wheel turns around
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steering Wheel", meta = (EditCondition = "bHasSteeringWheel"))
	EVehicleBoneAxis SteeringWheelAxis = EVehicleBoneAxis::Z;

	// Wheel turn in degrees at full steering lock. Negative turns it the other way.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steering Wheel", meta = (EditCondition = "bHasSteeringWheel"))
	float SteeringWheelMaxAngle = 180.0f;

	// Sockets of the vehicle skeletal mesh the hands grab, placed under the steering wheel bone so they turn with it
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steering Wheel", meta = (EditCondition = "bHasSteeringWheel"))
	FName LeftHandSocket = TEXT("steering_hand_l");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steering Wheel", meta = (EditCondition = "bHasSteeringWheel"))
	FName RightHandSocket = TEXT("steering_hand_r");

	// World transforms the hands of Unit sitting here should reach (hand IK). False - no hand IK in this seat.
	// Base seat: the steering wheel sockets.
	virtual bool GetHandTargets(const AALSBaseCharacter* Unit, FTransform& OutLeft, FTransform& OutRight) const;
		
private:

	AALSBaseCharacter* BookingUnit = nullptr;
	static float LocationOffset;;
};
