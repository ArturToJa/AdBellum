// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGMMissile.h"
#include "ATGMWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AATGMMissile::AATGMMissile()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetNotifyRigidBodyCollision(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = Mesh;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	TrailEffect = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);

	// The actor itself still replicates (needed for the explosion multicast's channel to exist on clients at
	// all), but NOT its movement/transform - see the class comment for why Tick runs identically everywhere
	// instead.
	bReplicates = true;
	SetReplicateMovement(false);
}

void AATGMMissile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// COND_SkipOwner: the gunner who fired this missile (its Owner) computes these from their own instant,
	// locally-predicted aim (see AATGMWeapon::UpdateHomingPoint/GunnerSeat->IsLocallyAimed) rather than the
	// server's, which only ever sees that gunner's aim after it has round-tripped over the network - replicating
	// the server's value to the owner too would otherwise fight with/overwrite their own fresher one every tick.
	DOREPLIFETIME_CONDITION(AATGMMissile, HomingPoint, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AATGMMissile, SightlineOrigin, COND_SkipOwner);
	DOREPLIFETIME(AATGMMissile, OwnerGun);
}

void AATGMMissile::BeginPlay()
{
	Super::BeginPlay();
	Mesh->OnComponentHit.AddDynamic(this, &AATGMMissile::OnHit);
}

void AATGMMissile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bFlightStopped)
	{
		return;
	}

	// Guidance/acceleration run on every machine identically now (see the class comment) - only
	// damage/explosion (OnHit/Explode) stay server-authoritative.
	ElapsedTime += DeltaTime;

	CurrentRotationSpeed = FMath::FInterpTo(CurrentRotationSpeed, MaxTurnRateDegreesPerSecond, DeltaTime, RotationSpeedRampRate);
	if (ElapsedTime >= DelayTargeting)
	{
		const FVector LineDir = (HomingPoint - SightlineOrigin).GetSafeNormal();
		if (!LineDir.IsNearlyZero())
		{
			// Aim at a point further down the gunner's sightline, not straight at HomingPoint itself: project
			// the missile's own position onto the line, then look LineLookAheadDistance ahead of that - this
			// eases the missile back onto the line instead of cutting a sharp angle at a single far point
			// whenever it has drifted off to the side.
			const float ProjectedDist = FVector::DotProduct(GetActorLocation() - SightlineOrigin, LineDir);
			const FVector ClosestPointOnLine = SightlineOrigin + LineDir * FMath::Max(ProjectedDist, 0.0f);
			const FVector AimPoint = ClosestPointOnLine + LineDir * LineLookAheadDistance;

			const FRotator TargetRotation = (AimPoint - GetActorLocation()).Rotation();
			// Hard turn-rate cap (degrees/second), not just an exponential approach - RInterpTo alone can still
			// produce a large per-frame delta when the target direction suddenly changes (e.g. the gunner's aim
			// itself jumping), which read as the missile snapping/teleporting rather than turning.
			SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), TargetRotation, DeltaTime, CurrentRotationSpeed));
		}
	}

	if (ElapsedTime >= DelayLaunchAcceleration)
	{
		CurrentSpeed = FMath::FInterpTo(CurrentSpeed, MaxRocketSpeed, DeltaTime, Acceleration);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = GetActorForwardVector() * CurrentSpeed;
	}
}

void AATGMMissile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// Every machine now actually simulates movement/collision locally (see Tick/class comment), so OnHit fires
	// on clients too - only the server's own hit is authoritative for damage/explosion. A client's hit still
	// stops the missile locally (below) so it visibly rests at the impact point instead of flying on through
	// whatever it touched until the server's destroy eventually replicates down.
	if (HasAuthority())
	{
		Explode(Hit.Location);
	}
	else if (!bFlightStopped)
	{
		bFlightStopped = true;
		SetActorEnableCollision(false);
		if (ProjectileMovement)
		{
			ProjectileMovement->StopMovementImmediately();
		}
	}
}

void AATGMMissile::LifeSpanExpired()
{
	if (HasAuthority())
	{
		Explode(GetActorLocation());
	}
	else
	{
		// Local safety net only - the server's own Explode() (destroying itself via a short SetLifeSpan)
		// replicates the real destroy shortly after anyway.
		Super::LifeSpanExpired();
	}
}

void AATGMMissile::Explode(const FVector& Location)
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;

	Multicast_PlayExplosionEffects(Location);

	UGameplayStatics::ApplyRadialDamageWithFalloff(this, BaseDamage, MinimumDamage, Location,
		DamageInnerRadius, DamageOuterRadius, DamageFalloff, nullptr, TArray<AActor*>(), this);

	bFlightStopped = true;
	SetActorEnableCollision(false);
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}
	// Not an immediate Destroy() (would risk the multicast RPC above getting dropped if the actor channel
	// starts closing before it's flushed) and NOT SetLifeSpan (its expiry re-enters this same function through
	// our own LifeSpanExpired override, which bExploded above would then just swallow - the actor would never
	// actually be destroyed, only lose collision/movement, which is exactly what left the mesh/trail visible).
	// A plain one-shot timer sidesteps both.
	GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AATGMMissile::DestroyDeferred, 0.1f, false);
}

void AATGMMissile::DestroyDeferred()
{
	Destroy();
}

void AATGMMissile::Multicast_PlayExplosionEffects_Implementation(FVector Location)
{
	if (ExplosionVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionVFX, Location);
	}
	if (ExplosionSX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSX, Location);
	}

	// Reaches every machine (unlike a direct call from the server-only Explode()), including the gunner's own
	// client, which runs its own separate homing timer (see AATGMWeapon::UpdateHomingPoint) and needs this
	// explicit signal to stop it reliably instead of polling the replicated bMissileActive.
	if (OwnerGun)
	{
		OwnerGun->DeactivateMissile();
	}
}
