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


	UPROPERTY(ReplicatedUsing = OnRep_OccupyingUnit, EditAnywhere, BlueprintReadOnly)
	AALSBaseCharacter* OccupyingUnit = nullptr;

	UFUNCTION()
	void OnRep_OccupyingUnit(AALSBaseCharacter* PreviousOccupyingUnit);

	virtual bool SetOccupyingUnit(AALSBaseCharacter* InUnit);
	AALSBaseCharacter* GetOccupyingUnit();

	// Hides the whole mesh (SetVisibility, recursive - also hides held weapons/gear) of whichever unit sits
	// here, for every machine, regardless of who or what controls that unit - not just the local driver's own
	// view. For an enclosed seat (e.g. Driver) nobody can see the occupant from outside the hull anyway, and a
	// standing human doesn't actually fit a real IFV driver's position cleanly enough for any seated pose to
	// look right. Leave off for an exposed seat (Gunner_Standing) that should stay visible.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bHideOccupantMesh = false;

private:
	// Shared by OnRep_OccupyingUnit (clients) and SetOccupyingUnit/ClearOccupyingUnit (the server's own local
	// view, which never receives its own OnRep) so both apply the exact same rule.
	void ApplyOccupantMeshVisibility(AALSBaseCharacter* Unit, bool bHide) const;

public:

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

	// Delays ApplyOccupantMeshVisibility(Unit, true) by the same 1 second
	// AALSBaseCharacter::Server_SitCharacterDown_Implementation itself waits before actually attaching the
	// character (CharacterAttachmentTimer) - without this the mesh vanished a full second early, mid-climb-in,
	// instead of exactly when the unit is actually seated.
	void ScheduleOccupantMeshHide(AALSBaseCharacter* Unit);
	FTimerHandle OccupantHideTimer;
};
