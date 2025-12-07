// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/VehicleSeatComponent.h"
#include "Character/ALSBaseCharacter.h"
#include "System/AdBellumGameState.h"
#include <Unit/BaseUnit.h>
#include <Net/UnrealNetwork.h>

float UVehicleSeatComponent::LocationOffset = 90.0f;

void UVehicleSeatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVehicleSeatComponent, OccupyingUnit);
}

bool UVehicleSeatComponent::SetOccupyingUnit(AALSBaseCharacter* InUnit)
{
	if (BookingUnit)
	{
		if (InUnit != BookingUnit)
		{
			return false;
		}
		ClearBookedSeat();
	}
	OccupyingUnit = InUnit;
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	if (GameState)
	{
		GameState->SetSelectionCircle(false, Cast<ABaseUnit>(InUnit));
	}
	OccupyingUnit->ExitDelegate.BindUObject(this, &UVehicleSeatComponent::ExitSeat);

	//OccupyingUnit->GunnerWeaponChanged.BindUObject(this, &UVehicleSeatComponent::NewFunction) //NewFunction AActor* (int)
	return true;
}

AALSBaseCharacter* UVehicleSeatComponent::GetOccupyingUnit()
{
	return OccupyingUnit;
}

void UVehicleSeatComponent::BookSeat(AALSBaseCharacter* InUnit)
{
	BookingUnit = InUnit;
}

void UVehicleSeatComponent::ClearBookedSeat()
{
	BookingUnit = nullptr;
}

void UVehicleSeatComponent::ClearOccupyingUnit()
{
	OccupyingUnit = nullptr;
}

bool UVehicleSeatComponent::IsSeatAvailable()
{
	return OccupyingUnit == nullptr && BookingUnit == nullptr;
}

bool UVehicleSeatComponent::IsSeatBookedFor(AALSBaseCharacter* InUnit)
{
	return BookingUnit == InUnit;
}

EALSStationaryRole UVehicleSeatComponent::GetRole()
{
	return SeatRole;
}

void UVehicleSeatComponent::ExitSeat()
{
	if (OccupyingUnit)
	{
		ExitDelegate.ExecuteIfBound(OccupyingUnit);
		ClearOccupyingUnit();
	}
}

bool UVehicleSeatComponent::GetUseADS() 
{
	return bUseADS;
}

bool UVehicleSeatComponent::GetUseMainCameraTransform()
{
	return bUseMainCameraTransform;
}
