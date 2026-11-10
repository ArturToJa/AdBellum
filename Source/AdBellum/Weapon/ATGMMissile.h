// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATGMMissile.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class UParticleSystemComponent;
class AATGMWeapon;

// Wire-guided (SACLOS) ATGM missile: accelerates up to MaxRocketSpeed, then steers toward whatever HomingPoint
// its OwnerGun feeds it (the gunner's own aim line - see AATGMWeapon::UpdateHomingPoint). Movement is simulated
// identically on every machine (Tick runs everywhere, fed by the replicated HomingPoint/SightlineOrigin)
// rather than relying on the engine's generic replicated-movement smoothing, which produced a visibly
// different, erratic early trajectory on clients - the actor does not replicate its transform at all. Damage
// and the explosion are still server-authoritative (see OnHit/Explode), multicast to every machine.
UCLASS()
class ADBELLUM_API AATGMMissile : public AActor
{
	GENERATED_BODY()

public:
	AATGMMissile();

	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> TrailEffect;

	// Launcher that fired this missile, told to DeactivateMissile() on impact (from Multicast_PlayExplosionEffects,
	// so every machine - including the gunner's own client, which runs its own separate homing timer, see
	// AATGMWeapon::UpdateHomingPoint - reliably stops, not just the server). Replicated so clients have it too.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ATGM")
	TObjectPtr<AATGMWeapon> OwnerGun;

	void SetOwnerGun(AATGMWeapon* InOwnerGun) { OwnerGun = InOwnerGun; }

	// Far point of the gunner's current sightline (origin -> HomingPoint) the missile steers toward once
	// DelayTargeting has elapsed. Set every tick of AATGMWeapon's homing timer (server only) while this missile
	// is in flight, alongside SightlineOrigin (see SetGuidanceLine) - replicated so every machine's own Tick
	// (see the class comment) steers toward the same point, not just the server's.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ATGM")
	FVector HomingPoint = FVector::ZeroVector;

	// Near point (the gunner's eye) of the same sightline - together with HomingPoint this defines the line
	// the missile tries to fly along (not just point at), so it corrects back onto the sightline instead of
	// cutting a sharp angle straight at a single far point. See SetGuidanceLine.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "ATGM")
	FVector SightlineOrigin = FVector::ZeroVector;

	void SetGuidanceLine(FVector InOrigin, FVector InHomingPoint) { SightlineOrigin = InOrigin; HomingPoint = InHomingPoint; }

	// Seconds after launch before guidance starts correcting toward HomingPoint (arming delay)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Flight")
	float DelayTargeting = 0.5f;

	// Seconds after launch before the missile starts accelerating toward MaxRocketSpeed
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Flight")
	float DelayLaunchAcceleration = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Flight")
	float MaxRocketSpeed = 5000.0f;

	// Interp speed (not a real acceleration value) CurrentSpeed moves toward MaxRocketSpeed at
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Flight")
	float Acceleration = 10.0f;

	// Hard cap (degrees/second) on how fast the missile can turn, ramps up from 0 to this rather than applying
	// full turn rate the instant guidance arms - a missile that snaps instead of easing into its first
	// correction. Also doubles as the rate (degrees/second^2, roughly) of that ramp.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Flight")
	float MaxTurnRateDegreesPerSecond = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Flight")
	float RotationSpeedRampRate = 60.0f;

	// How far ahead (cm), along the sightline, of the missile's own projected position the missile aims - the
	// actual SACLOS trick: steer toward a moving point on the line, not at the line's fixed far end, so the
	// missile eases back onto the sightline instead of cutting a sharp angle at it from wherever it has drifted.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Flight")
	float LineLookAheadDistance = 2000.0f;

	// Radial damage on impact, see OnHit
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Damage")
	float BaseDamage = -5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Damage")
	float MinimumDamage = -1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Damage")
	float DamageInnerRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Damage")
	float DamageOuterRadius = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Damage")
	float DamageFalloff = 0.33f;

	// Explosion VFX spawned at the hit location
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Damage")
	TObjectPtr<class UNiagaraSystem> ExplosionVFX;

	// Explosion sound played at the hit location
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ATGM|Damage")
	TObjectPtr<class USoundBase> ExplosionSX;

protected:
	virtual void BeginPlay() override;

	// Default AActor behavior just silently Destroy()s - explode instead so a missile that times out (its
	// InitialLifeSpan, e.g. 15s) still deals damage, plays effects, and unblocks the launcher via
	// DeactivateMissile the same as a direct hit does.
	virtual void LifeSpanExpired() override;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);

	// Shared by OnHit and LifeSpanExpired: damage, effects, unblocking the launcher, then destroying self.
	// Server only.
	void Explode(const FVector& Location);

	// Each machine's own locally-simulated collision can land at a slightly different point/moment (see Tick/
	// class comment) and isn't authoritative anyway - the server's hit multicasts the VFX so every machine
	// shows the explosion at the same, correct place.
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayExplosionEffects(FVector Location);

	// Guards Explode() against running more than once - the missile can keep resting against/re-touching
	// whatever it hit during the brief destroy-delay window, which without this re-fired OnHit, re-sending the
	// multicast (looping VFX).
	bool bExploded = false;

	// Set by Explode() (server) or directly by OnHit on a non-authority machine: stops Tick from continuing to
	// drive the missile forward once it has hit something, so a client's own locally-simulated missile (see the
	// class comment) visibly stops/rests at the impact point instead of ghosting on through whatever it hit -
	// collision is detected identically on every machine, but only the server used to actually act on it.
	bool bFlightStopped = false;

	// Destroys self a moment after Explode() - not immediately (see Explode's own comment) and deliberately not
	// via SetLifeSpan, since that would re-enter this class's own LifeSpanExpired override.
	void DestroyDeferred();
	FTimerHandle DestroyTimerHandle;

	// Time (seconds) since launch, compared against DelayTargeting / DelayLaunchAcceleration
	float ElapsedTime = 0.0f;
	float CurrentSpeed = 0.0f;
	float CurrentRotationSpeed = 0.0f;
};
