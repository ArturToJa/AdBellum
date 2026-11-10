// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGMWeapon.h"
#include "ATGMMissile.h"
#include "Character/ALSBaseCharacter.h"
#include "Components/AudioComponent.h"
#include "EBBullet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Unit/BaseUnit.h"
#include "Vehicle/GunnerSeatComponent.h"

AATGMWeapon::AATGMWeapon()
{
}

void AATGMWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AATGMWeapon, bMissileActive);
	DOREPLIFETIME(AATGMWeapon, ActiveMissile);
}

void AATGMWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (EBarrel)
	{
		EBarrel->ShotFired.AddDynamic(this, &AATGMWeapon::HandleShotFired);
	}
}

EWeaponSocketEnum AATGMWeapon::GetWeaponSocket_Implementation()
{
	return EWeaponSocketEnum::SPECIAL;
}

void AATGMWeapon::HandleShotFired()
{
	// ShotFired is replicated by EBarrel to every machine (ReplicateShotFiredEvents) - the reload sound is
	// purely cosmetic feedback and needs to play on all of them, unlike the missile spawn/guidance below
	// (which must only ever happen once, on the server) - so this cannot be behind the HasAuthority() guard.
	if (ReloadSound)
	{
		ReloadAudioComponent = UGameplayStatics::SpawnSoundAttached(ReloadSound, WeaponRoot);
	}
	GetWorldTimerManager().SetTimer(ReloadSoundTimerHandle, this, &AATGMWeapon::StopReloadSound, ReloadTime, false);

	if (!MissileClass || !EBarrel)
	{
		return;
	}

	// Every machine that needs to run its own homing updates (the server, for authoritative/other-viewer
	// purposes; the gunner's own client, for an instant, un-laggy local view - see UpdateHomingPoint) needs
	// these cached, not just the server.
	GunnerChar = Cast<AALSBaseCharacter>(GetOwner());
	if (const ABaseUnit* Unit = Cast<ABaseUnit>(GetOwner()))
	{
		GunnerSeat = Cast<UGunnerSeatComponent>(Unit->OccupiedSeat);
	}

	// bMissileActive is belt-and-suspenders alongside EBarrel->ShootingBlocked (which already prevents
	// SpawnBullet while blocked) - replicated so a client also reliably knows it can't fire again yet, not
	// just the server.
	if (HasAuthority() && !bMissileActive)
	{
		const FTransform SpawnTransform(EBarrel->GetComponentRotation(), EBarrel->GetComponentLocation());
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = GetInstigator();
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AATGMMissile* Missile = GetWorld()->SpawnActor<AATGMMissile>(MissileClass, SpawnTransform, SpawnParams))
		{
			ActiveMissile = Missile;
			Missile->SetOwnerGun(this);
			bMissileActive = true;
			EBarrel->ShootingBlocked = true;
		}
	}

	// The server always runs this (authoritative hit detection, and the view every other/non-gunner machine
	// sees via replication). The gunner's own client separately runs it too, using its own instant aim instead
	// of waiting for that aim to round-trip through the server first - see the GunnerSeat comment in the header.
	if (HasAuthority() || (GunnerSeat.IsValid() && GunnerSeat->IsLocallyAimed()))
	{
		GetWorldTimerManager().SetTimer(HomingPointTimerHandle, this, &AATGMWeapon::UpdateHomingPoint, 0.01f, true);
	}
}

void AATGMWeapon::UpdateHomingPoint()
{
	if (!bMissileActive)
	{
		// Also handles the gunner's own client's cleanup: DeactivateMissile() only ever runs on the server, but
		// bMissileActive is plainly replicated (unlike ActiveMissile, see its own COND_SkipOwner-adjacent note)
		// so every machine reliably learns it went false and can stop its own timer here.
		GetWorldTimerManager().ClearTimer(HomingPointTimerHandle);
		return;
	}
	if (!ActiveMissile || !GunnerChar.IsValid() || !GunnerSeat.IsValid())
	{
		// Not a stop condition: ActiveMissile in particular can be transiently null for a tick or two right
		// after firing, simply because its replication to this client hasn't arrived yet - only bMissileActive
		// going false above means "done."
		return;
	}

	// GetAim() (yaw/pitch relative to the vehicle) is this seat's own client-predicted, server-replicated aim -
	// the turret/MCT socket is actually driven by it. GetControlRotation() is not reliably kept in sync on the
	// server for a remote client's seated gunner (that mismatch is why the seat has its own aim replication at
	// all), so using it here gave a wrong, jittery homing direction for anyone but the host.
	const FVector2D Aim = GunnerSeat->GetAim();
	const AActor* Vehicle = GunnerSeat->GetOwner();
	const FRotator RelativeAimRotation(Aim.Y, Aim.X, 0.0f);
	const FRotator WorldAimRotation = Vehicle
		? (Vehicle->GetActorQuat() * RelativeAimRotation.Quaternion()).Rotator()
		: RelativeAimRotation;

	const FVector Start = GunnerChar->GetFirstPersonCameraTarget();
	const FVector End = Start + WorldAimRotation.Vector() * 100000.0f;

	TArray<AActor*> IgnoreActors;
	if (AActor* Interceptor = GunnerChar->ControlledInputInterceptor)
	{
		IgnoreActors.Add(Interceptor);
	}
	// The missile itself (flying roughly down this same sightline) and any other in-flight bullet would
	// otherwise self-block the trace right as it's about to reach the actual target.
	IgnoreActors.Add(ActiveMissile);
	TArray<AActor*> Bullets;
	UGameplayStatics::GetAllActorsOfClass(this, AEBBullet::StaticClass(), Bullets);
	IgnoreActors.Append(Bullets);

	FHitResult Hit;
	UKismetSystemLibrary::LineTraceSingle(this, Start, End, ETraceTypeQuery::TraceTypeQuery1, false,
		IgnoreActors, EDrawDebugTrace::None, Hit, true);

	// A blocking hit (something on the EBullet trace channel) homes the missile onto that exact point/object;
	// otherwise Hit.Location is left at the zero vector (not the trace's far end), which previously sent the
	// missile lurching toward world origin whenever the gunner aimed at open sky/past the map edge.
	ActiveMissile->SetGuidanceLine(Start, Hit.bBlockingHit ? Hit.Location : Hit.TraceEnd);
}

void AATGMWeapon::StopReloadSound()
{
	if (ReloadAudioComponent.IsValid())
	{
		ReloadAudioComponent->Stop();
	}
}

void AATGMWeapon::DeactivateMissile()
{
	bMissileActive = false;
	ActiveMissile = nullptr;
	GetWorldTimerManager().ClearTimer(HomingPointTimerHandle);
	if (EBarrel)
	{
		EBarrel->ShootingBlocked = false;
	}
}
