// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleAccessPoint.h"
#include "Player/IPlayer.h"
#include "VehicleSeatComponent.h"
#include "GunnerSeatComponent.h"


void UVehicleAccessPoint::BeginPlay()
{
	Super::BeginPlay();
	OwnerActor = GetOwner();
}

void UVehicleAccessPoint::Interact_Implementation(AALSBaseCharacter* InUnit)
{
	if (InUnit->StationaryRole == EALSStationaryRole::None) {
		UVehicleSeatComponent* Seat = GetAvailableSeat(InUnit);
		if (Seat)
		{
			if (Seat->SetOccupyingUnit(InUnit))
			{
				InUnit->ControlledInputInterceptor = OwnerActor;
				InUnit->Server_SitCharacterDown(Seat);
				InUnit->StationaryRole = Seat->GetRole();

				if (InUnit->GetController()->GetClass()->ImplementsInterface(UIPlayer::StaticClass()))
				{
					IIPlayer::Execute_NotifyHUDRole(InUnit->GetController(), Seat->GetRole(), InUnit);
				}

				if (Seat->GetRole() == EALSStationaryRole::Driver) 
				{
					InUnit->GetController()->Possess(Cast<APawn>(OwnerActor));
				}
				InUnit->Client_SetUseStationaryADS(Seat->GetUseADS());
				InUnit->Client_SetUseMainCameraTransform(Seat->GetUseMainCameraTransform());
				//InUnit->Client_SetRecenterCamera(true);
				//go to controller -> Selectable AActor* for APlayerController
				Seat->ExitDelegate.BindUObject(this, &UVehicleAccessPoint::Exit);
			}
		}
	}
}


void UVehicleAccessPoint::Exit(AALSBaseCharacter* ExitingChar)
{
	ExitingChar->Client_SetUseStationaryADS(false);
	ExitingChar->Client_SetUseMainCameraTransform(false);

	if (ExitingChar->GetController()->GetClass()->ImplementsInterface(UIPlayer::StaticClass()))
	{
		IIPlayer::Execute_NotifyHUDRole(ExitingChar->GetController(), EALSStationaryRole::None, ExitingChar);
	}

	if (ExitingChar->GunnerWeaponChangedDelegate.IsBound())
	{
		ExitingChar->GunnerWeaponChangedDelegate.Unbind();
		Cast<UGunnerSeatComponent>(GetSeatWithRole(EALSStationaryRole::Gunner))->SetWeaponArrayOwner(nullptr);
	}
	ExitingChar->Server_StandCharacterUp(GetComponentLocation());
}

float UVehicleAccessPoint::InteractRange_Implementation()
{
	return 50.0f;
}

FVector UVehicleAccessPoint::InteractLocation_Implementation()
{
	return GetComponentLocation();
}

UVehicleSeatComponent* UVehicleAccessPoint::GetAvailableSeat(AALSBaseCharacter* InUnit)
{
	UVehicleSeatComponent* AnyAvailableSeat = nullptr;
	for (FComponentReference SeatRef : SeatArray)
	{
		UVehicleSeatComponent* Seat = Cast<UVehicleSeatComponent>(SeatRef.GetComponent(GetOwner()));
		if (Seat->IsSeatBookedFor(InUnit))
		{
			return Seat;
		}
		else if (Seat->IsSeatAvailable())
		{
			if (Seat->GetRole() == InUnit->StationaryRole || AnyAvailableSeat == nullptr)
			{
				AnyAvailableSeat = Seat;
			}
		}
	}
	return AnyAvailableSeat;
}

UVehicleSeatComponent* UVehicleAccessPoint::GetSeatWithRole(EALSStationaryRole StationaryRole) 
{
	UVehicleSeatComponent* AnyAvailableSeat = nullptr;
	for (FComponentReference SeatRef : SeatArray)
	{
		UVehicleSeatComponent* Seat = Cast<UVehicleSeatComponent>(SeatRef.GetComponent(GetOwner()));
		if (Seat->GetRole() == EALSStationaryRole::Gunner)
		{
			return Seat;
		}
	}
	return AnyAvailableSeat;
}

bool UVehicleAccessPoint::BookSeat(AALSBaseCharacter* InUnit)
{
	UVehicleSeatComponent* Seat = GetAvailableSeat(InUnit);
	if (Seat)
	{
		Seat->BookSeat(InUnit);
		return true;
	}
	else
	{
		return false;
	}
}

#if WITH_EDITOR
void UVehicleAccessPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Update the visual representation of the component in the editor
	MarkRenderStateDirty();
}
#endif