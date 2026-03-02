// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Library/ALSCharacterEnumLibrary.h"
#include "VehicleSeatComponent.generated.h"

class AALSBaseCharacter;

DECLARE_DELEGATE_OneParam(OnExitSeatDelegate, AALSBaseCharacter*)

/**
 * 
 */
UCLASS(DefaultToInstanced)
class ADBELLUM_API UVehicleSeatComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()
public:

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
		
private:

	AALSBaseCharacter* BookingUnit = nullptr;
	static float LocationOffset;;
};
