// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SimpleWeapon.h"
#include "ATGMWeapon.generated.h"

class AATGMMissile;
class AALSBaseCharacter;
class UAudioComponent;
class UGunnerSeatComponent;

// ATGM (wire-guided, SACLOS) launcher: fires one AATGMMissile at a time and continuously steers it toward
// wherever the gunner is currently aiming (not a locked target) until it hits something.
UCLASS()
class ADBELLUM_API AATGMWeapon : public ASimpleWeapon
{
	GENERATED_BODY()

public:
	AATGMWeapon();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM")
	TSubclassOf<AATGMMissile> MissileClass;

	// Seconds the barrel stays blocked (reload sound loops) after a shot, while ammo remains to cycle
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM")
	float ReloadTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM")
	TObjectPtr<USoundBase> ReloadSound;

	// True from the moment a missile is launched until it's deactivated (hit/expired) - replicated so clients
	// also know a shot can't be fired again yet, rather than relying only on EBarrel's own (also replicated,
	// but separately-timed) ShootingBlocked state.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ATGM")
	bool bMissileActive = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual EWeaponSocketEnum GetWeaponSocket_Implementation() override;

	// Unblocks the barrel for the next shot, called by the active missile on impact
	void DeactivateMissile();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleShotFired();

	// Timer callback (100Hz while a missile is in flight): line traces along the gunner's current aim and
	// feeds the result to ActiveMissile as its new HomingPoint - this is what makes the missile wire-guided
	// rather than target-locked.
	void UpdateHomingPoint();

	void StopReloadSound();

	// Replicated (not just a server-local pointer) so the gunner's own client can also find the missile it
	// just fired and run its own local homing update against it - see UpdateHomingPoint/IsLocallyAimed.
	UPROPERTY(Replicated)
	TObjectPtr<AATGMMissile> ActiveMissile;

	TWeakObjectPtr<AALSBaseCharacter> GunnerChar;
	// Aim direction comes from here (GetAim()), not GunnerChar->GetControlRotation() (not reliably kept in sync
	// for a seated gunner). GetAim() itself resolves differently per machine: the server always sees
	// ReplicatedAim (the gunner's aim as last received over the network - UpdateHomingPoint still runs there
	// for authoritative/other-viewer purposes), while the gunner's own client sees LocalAim (instant, no network
	// lag) when GunnerSeat->IsLocallyAimed() - which is why that machine also runs its own UpdateHomingPoint,
	// rather than just trusting the server's (always one round-trip stale, relative to that gunner's own screen).
	TWeakObjectPtr<UGunnerSeatComponent> GunnerSeat;
	TWeakObjectPtr<UAudioComponent> ReloadAudioComponent;
	FTimerHandle HomingPointTimerHandle;
	FTimerHandle ReloadSoundTimerHandle;
};
