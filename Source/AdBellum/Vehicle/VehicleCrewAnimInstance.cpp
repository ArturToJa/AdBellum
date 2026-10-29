// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleCrewAnimInstance.h"
#include "BaseVehicle.h"
#include "GunnerSeatComponent.h"
#include "Unit/BaseUnit.h"

void UVehicleCrewAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	Unit = Cast<AALSBaseCharacter>(TryGetPawnOwner());
}

void UVehicleCrewAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AALSBaseCharacter* Character = Unit.Get();
	if (!Character)
	{
		HandIKAlpha = 0.0f;
		return;
	}

	CrewRole = Character->GetStationaryRole();

	const ABaseUnit* BaseUnit = Cast<ABaseUnit>(Character);
	const UVehicleSeatComponent* Seat = BaseUnit ? BaseUnit->OccupiedSeat.Get() : nullptr;
	const UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(Seat);
	ABaseVehicle* Vehicle = Cast<ABaseVehicle>(Character->ControlledInputInterceptor);

	SteeringBlend = Vehicle ? FMath::Clamp(Vehicle->GetCurrentSteering(), -1.0f, 1.0f) * SteeringBlendRange : 0.0f;
	bSittingPose = GunnerSeat && GunnerSeat->bSittingPose;

	FTransform Left, Right;
	if (Seat && Seat->GetHandTargets(Character, Left, Right))
	{
		LeftHandTarget = Left;
		RightHandTarget = Right;
		HandIKAlpha = 1.0f;
	}
	else
	{
		HandIKAlpha = 0.0f;
	}

	// The driver looks where the vehicle camera looks (replicated driver look), everybody else where the unit aims.
	// Both are valid on every machine, control rotation would only be on the owning one.
	const FVector Origin = Character->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f);
	const FVector Direction = (CrewRole == EALSStationaryRole::Driver && Vehicle)
		? Vehicle->GetDriverLookWorldRotation().Vector() * LookAtDistance
		: Character->GetAimingRotation().Vector() * LookAtDistance;
	LookAtTarget = FTransform(Origin + Direction);
}
