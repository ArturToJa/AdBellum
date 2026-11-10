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

	UGunnerSeatComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	OnGunnerWeaponChanged GunnerWeaponChanged;

	UPROPERTY(Replicated, BlueprintReadWrite, EditDefaultsOnly, Meta = (UseComponentPicker))
	TArray<FComponentReference> WeaponArray;

	// Coaxial machine gun fired independently of WeaponArray's main weapon(s), via JumpAction (space) while
	// seated as gunner - see ABaseUnit::JumpAction_Implementation. Uses the gunner's existing crosshair/zero,
	// no reticle of its own. None - space does nothing extra for this seat.
	UPROPERTY(Replicated, BlueprintReadWrite, EditDefaultsOnly, Meta = (UseComponentPicker))
	FComponentReference CoaxWeaponComponent;

	// Resolves CoaxWeaponComponent to its actor, null when not configured
	UFUNCTION(BlueprintPure, Category = "Weapons")
	AActor* GetCoaxWeapon() const;

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

	// Gunner_Standing logic (camera, aiming, input), but the unit sits instead of standing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Seat")
	bool bSittingPose = false;

	// ZOOM

	// Magnifications this seat's own optic (periscope/sight glass) can step through, e.g. [1.0, 4.0, 8.0].
	// Only used while Use Main Camera Transform is on (an enclosed gunner looking through the vehicle's own
	// sight, not its weapon's) - a Gunner_Standing instead zooms through its weapon's own sight/FOV, since
	// GetUseADS() is what gates that role. Empty - no zoom, always BaseFOV.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Zoom")
	TArray<float> ZoomLevels;

	// FOV at 1x (no magnification)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Zoom", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float BaseFOV = 90.0f;

	// Steps to the next (Direction > 0) or previous (Direction < 0) zoom level. bWrap wraps from the last
	// level back to the first (and back) - used for the aim button's click-to-cycle; scroll instead clamps,
	// stopping at the first/last level. Returns false (no-op) when ZoomLevels is empty, or clamped and
	// already at that end.
	UFUNCTION(BlueprintCallable, Category = "Zoom")
	bool CycleZoom(int32 Direction, bool bWrap = true);

	// True when this seat has its own optic zoom table (ZoomLevels), rather than relying on the weapon's ADSFOV
	UFUNCTION(BlueprintPure, Category = "Zoom")
	bool HasZoomLevels() const { return ZoomLevels.Num() > 0; }

	// FOV for the current zoom level (BaseFOV divided by its magnification), BaseFOV when there is no zoom
	UFUNCTION(BlueprintPure, Category = "Zoom")
	float GetCurrentZoomFOV() const;

	// Pixel offset from screen center for the gunner crosshair (UGunnerCrosshairWidget). A weapon's reticle
	// texture is shared across vehicles, but this seat's own MCT socket placement is not - a reticle correctly
	// centered on one vehicle's camera can need a different nudge on another, so this lives per seat, not on
	// the weapon or the HUD widget's own default.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Crosshair")
	FVector2D CrosshairOffset = FVector2D::ZeroVector;

	UFUNCTION(BlueprintPure, Category = "Zoom")
	float GetCurrentZoomMagnification() const;

	// Grips of the weapon this unit mans here (ASimpleWeapon hand sockets)
	virtual bool GetHandTargets(const AALSBaseCharacter* Unit, FTransform& OutLeft, FTransform& OutRight) const override;

private:
	// Index in ZoomLevels this seat's optic is currently set to
	int32 CurrentZoomIndex = 0;

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
