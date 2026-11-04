// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleSeatComponent.h"
#include "Character/ALSBaseCharacter.h"
#include "System/AdBellumGameState.h"
#include <Unit/BaseUnit.h>
#include <Net/UnrealNetwork.h>
#include "TimerManager.h"
#include "BaseVehicle.h"

float UVehicleSeatComponent::LocationOffset = 90.0f;

UVehicleSeatComponent::UVehicleSeatComponent()
{
	// OccupyingUnit (and gunner aim in UGunnerSeatComponent) is replicated
	SetIsReplicatedByDefault(true);
}

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

	// The server's own local view: OnRep_OccupyingUnit never fires on the machine that made the change
	ScheduleOccupantMeshHide(OccupyingUnit);
	return true;
}

void UVehicleSeatComponent::OnRep_OccupyingUnit(AALSBaseCharacter* PreviousOccupyingUnit)
{
	ApplyOccupantMeshVisibility(PreviousOccupyingUnit, false);
	ScheduleOccupantMeshHide(OccupyingUnit);
}

void UVehicleSeatComponent::ScheduleOccupantMeshHide(AALSBaseCharacter* Unit)
{
	GetWorld()->GetTimerManager().ClearTimer(OccupantHideTimer);
	if (!bHideOccupantMesh || !IsValid(Unit))
	{
		return;
	}
	FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UVehicleSeatComponent::ApplyOccupantMeshVisibility, Unit, true);
	GetWorld()->GetTimerManager().SetTimer(OccupantHideTimer, Delegate, 1.0f, false);
}

void UVehicleSeatComponent::ApplyOccupantMeshVisibility(AALSBaseCharacter* Unit, bool bHide) const
{
	if (!bHideOccupantMesh || !IsValid(Unit))
	{
		return;
	}
	if (USkeletalMeshComponent* Mesh = Unit->GetMesh())
	{
		Mesh->SetVisibility(!bHide, true);
	}
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
	// A quick enter-then-exit (before the 1-second hide delay even fires) would otherwise leave a stale
	// scheduled hide pending, which could later hide the wrong unit if this seat gets reoccupied in the meantime
	GetWorld()->GetTimerManager().ClearTimer(OccupantHideTimer);
	// Same reasoning as SetOccupyingUnit: the server's own local view needs this applied directly too
	ApplyOccupantMeshVisibility(OccupyingUnit, false);
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

bool UVehicleSeatComponent::GetHandTargets(const AALSBaseCharacter* Unit, FTransform& OutLeft, FTransform& OutRight) const
{
	if (!bHasSteeringWheel)
	{
		return false;
	}
	const ABaseVehicle* Vehicle = Cast<ABaseVehicle>(GetOwner());
	const USkeletalMeshComponent* Mesh = Vehicle ? Vehicle->VehicleSkeletalMesh.Get() : nullptr;
	if (!Mesh || !Mesh->DoesSocketExist(LeftHandSocket) || !Mesh->DoesSocketExist(RightHandSocket))
	{
		return false;
	}
	OutLeft = Mesh->GetSocketTransform(LeftHandSocket);
	OutRight = Mesh->GetSocketTransform(RightHandSocket);
	return true;
}
