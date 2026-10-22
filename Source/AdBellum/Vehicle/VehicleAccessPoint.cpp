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
				if (ABaseUnit* Unit = Cast<ABaseUnit>(InUnit))
				{
					Unit->OccupiedSeat = Seat;
				}
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
				// One access point can serve several seats, the seat remembers which point its unit leaves through
				Seat->ExitDelegate.BindUObject(this, &UVehicleAccessPoint::Exit);
			}
		}
	}
}


void UVehicleAccessPoint::Exit(AALSBaseCharacter* ExitingChar)
{
	ExitingChar->Client_SetUseStationaryADS(false);
	ExitingChar->Client_SetUseMainCameraTransform(false);

	if (ExitingChar->GetController() && ExitingChar->GetController()->GetClass()->ImplementsInterface(UIPlayer::StaticClass()))
	{
		IIPlayer::Execute_NotifyHUDRole(ExitingChar->GetController(), EALSStationaryRole::None, ExitingChar);
	}

	// The seat this unit sits in, not just any gunner seat: a vehicle can have several gunners
	ABaseUnit* ExitingUnit = Cast<ABaseUnit>(ExitingChar);
	UGunnerSeatComponent* GunnerSeat = ExitingUnit ? Cast<UGunnerSeatComponent>(ExitingUnit->OccupiedSeat) : nullptr;
	if (ExitingChar->GunnerWeaponChangedDelegate.IsBound())
	{
		ExitingChar->GunnerWeaponChangedDelegate.Unbind();
	}
	if (GunnerSeat)
	{
		GunnerSeat->SetWeaponArrayOwner(nullptr);
	}
	if (ExitingUnit)
	{
		ExitingUnit->OccupiedSeat = nullptr;
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
		if (!Seat)
		{
			continue;
		}
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
	for (FComponentReference SeatRef : SeatArray)
	{
		UVehicleSeatComponent* Seat = Cast<UVehicleSeatComponent>(SeatRef.GetComponent(GetOwner()));
		if (Seat && Seat->GetRole() == StationaryRole)
		{
			return Seat;
		}
	}
	return nullptr;
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