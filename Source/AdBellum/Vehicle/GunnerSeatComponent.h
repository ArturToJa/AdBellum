// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VehicleSeatComponent.h"
#include "Character/ALSBaseCharacter.h"
#include "GunnerSeatComponent.generated.h"

class AALSBaseCharacter;

DECLARE_DELEGATE_RetVal_OneParam(AActor*, OnGunnerWeaponChanged, int)

// Seat of a gunner. A vehicle can have any number of them, every seat owns its weapons and its own aim
// (turret yaw / gun pitch), so two gunners never fight over the same turret.
UCLASS(ClassGroup = (VehicleComponent), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API UGunnerSeatComponent : public UVehicleSeatComponent
{
public:
	GENERATED_BODY()

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	OnGunnerWeaponChanged GunnerWeaponChanged;

	UPROPERTY(Replicated, BlueprintReadWrite, EditDefaultsOnly, Meta = (UseComponentPicker))
	TArray<FComponentReference> WeaponArray;

	void SetWeaponArrayOwner(AALSBaseCharacter* InUnit);

	virtual bool SetOccupyingUnit(AALSBaseCharacter* InUnit) override;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BindDelegateToUnit(AALSBaseCharacter* InUnit);

	void GetControlledWeapon(int Id);
	AActor* GetWeapon(int Id);

	// AIM

	// Bone of the vehicle skeletal mesh turned by this seat's yaw (turret), None when the seat has no turret.
	// A second gunner seat needs its own bones (or None), two seats naming the same bone fight over it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim")
	FName TurretBoneName = TEXT("turret_jnt");

	// Bone of the vehicle skeletal mesh turned by this seat's pitch (gun), None when the seat has no gun bone
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Aim")
	FName GunBoneName = TEXT("gun_jnt");

	// Degrees of yaw per unit of look input
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim")
	float YawRate = 0.66f;

	// Degrees of pitch per unit of look input
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim")
	float PitchRate = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim")
	float MinPitch = -10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim")
	float MaxPitch = 45.0f;

	// Off - turret turns all the way around
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim")
	bool bLimitYaw = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim", meta = (EditCondition = "bLimitYaw"))
	float MinYaw = -90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim", meta = (EditCondition = "bLimitYaw"))
	float MaxYaw = 90.0f;

	// Fastest turn (degrees per second) the server accepts from a player, 0 - not limited
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aim", meta = (ClampMin = "0.0"))
	float MaxAimSpeed = 180.0f;

	// Look input of the gunner sitting here. On the gunner's own machine the aim changes immediately,
	// the caller is responsible for sending GetAim() to the server (ABaseUnit does, through its own RPC,
	// because a gunner does not own the vehicle).
	void AddAimInput(float YawInput, float PitchInput);

	// Server: aim reported by the gunner's machine, limited to the seat's range and MaxAimSpeed
	void SetAimFromClient(FVector2D NewAim);

	// Server: aim set by game code (AI gunner), limited to the seat's range only
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void SetAim(float Yaw, float Pitch);

	// X - yaw, Y - pitch, degrees relative to the vehicle
	FVector2D GetAim() const;

	UFUNCTION(BlueprintPure, Category = "Aim")
	float GetAimYaw() const { return GetAim().X; }

	UFUNCTION(BlueprintPure, Category = "Aim")
	float GetAimPitch() const { return GetAim().Y; }

	// True on the machine of the player sitting here (when that machine is not the server): aim is not delayed there
	bool IsLocallyAimed() const { return UsesLocalAim(); }

private:
	FVector2D ClampAim(FVector2D Aim) const;

	// True on the machine of the player sitting here, when that machine is not the server
	bool UsesLocalAim() const;

	UPROPERTY(Replicated)
	FVector2D ReplicatedAim = FVector2D::ZeroVector;

	// Aim on the gunner's own machine, ahead of ReplicatedAim by the network delay
	FVector2D LocalAim = FVector2D::ZeroVector;

	// Unit LocalAim belongs to, a new gunner starts from the aim the previous one left
	TWeakObjectPtr<AALSBaseCharacter> LocalAimUnit;

	// Server: world time of the last aim accepted from a client, used with MaxAimSpeed
	float LastClientAimTime = 0.0f;
};
