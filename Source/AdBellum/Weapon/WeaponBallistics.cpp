// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponBallistics.h"
#include "EBBarrel.h"
#include "EBBullet.h"
#include "Engine/World.h"

TSubclassOf<AEBBullet> WeaponBallistics::GetCalibrationBulletClass(const UEBBarrel* Barrel)
{
	if (!Barrel)
	{
		return nullptr;
	}
	if (Barrel->ChamberedBullet)
	{
		return Barrel->ChamberedBullet;
	}
	if (Barrel->Ammo.Num() > 0)
	{
		return Barrel->Ammo[0];
	}
	return nullptr;
}

bool WeaponBallistics::SimulateTrajectoryPoint(UWorld* World, const UEBBarrel* Barrel, TSubclassOf<AEBBullet> BulletClass,
	const FVector& Start, float DistanceMeters, FVector& OutRangePoint, float& OutFlightTime, TArray<FVector>* OutTrajectory)
{
	const AEBBullet* Bullet = BulletClass ? BulletClass->GetDefaultObject<AEBBullet>() : nullptr;
	if (!World || !Bullet || !Barrel || DistanceMeters <= 0.0f)
	{
		return false;
	}

	constexpr float TimeStep = 0.001f;
	constexpr float MaxTime = 10.0f;
	constexpr int32 TrajectorySampleInterval = 10;

	const float TargetX = DistanceMeters * 100.0f;
	// When collecting trajectory for debug, continue past the zero point to show the drop behind it
	const float EndX = OutTrajectory ? TargetX * 1.5f : TargetX;

	// Same muzzle velocity as EBBarrel prediction functions use
	const float MuzzleSpeed = FMath::Lerp(Barrel->MuzzleVelocityMultiplierMin, Barrel->MuzzleVelocityMultiplierMax, 0.5f)
		* FMath::Lerp(Bullet->MuzzleVelocityMin, Bullet->MuzzleVelocityMax, 0.5f);
	if (MuzzleSpeed <= 0.0f)
	{
		return false;
	}

	// Virtual level frame: shooting along world +X, so gravity (world -Z) acts as if the weapon was held level.
	// Range frame X = downrange, Z = up.
	FVector Location = Start;
	FVector Velocity(MuzzleSpeed, 0.0f, 0.0f);
	float Time = 0.0f;
	int32 StepIndex = 0;
	bool bFound = false;

	if (OutTrajectory)
	{
		OutTrajectory->Reset();
		OutTrajectory->Add(FVector::ZeroVector);
	}

	while (Time < MaxTime)
	{
		const FVector PreviousVelocity = Velocity;
		Velocity = Bullet->UpdateVelocity(World, Location, Velocity, TimeStep);
		const FVector NewLocation = Location + (PreviousVelocity + Velocity) * 0.5f * TimeStep;

		if (!bFound && NewLocation.X - Start.X >= TargetX)
		{
			const float Alpha = (TargetX - (Location.X - Start.X)) / (NewLocation.X - Location.X);
			OutRangePoint = FMath::Lerp(Location, NewLocation, Alpha) - Start;
			OutFlightTime = Time + Alpha * TimeStep;
			bFound = true;
		}

		Location = NewLocation;
		Time += TimeStep;

		const bool bEnd = Location.X - Start.X >= EndX || Velocity.X <= KINDA_SMALL_NUMBER;
		if (OutTrajectory && (++StepIndex % TrajectorySampleInterval == 0 || bEnd))
		{
			OutTrajectory->Add(Location - Start);
		}
		if (bEnd)
		{
			break;
		}
	}
	return bFound;
}

float WeaponBallistics::SignedAngleAroundAxis(const FVector& From, const FVector& To, const FVector& Axis)
{
	const FVector FromProjected = FVector::VectorPlaneProject(From, Axis).GetSafeNormal();
	const FVector ToProjected = FVector::VectorPlaneProject(To, Axis).GetSafeNormal();
	if (FromProjected.IsNearlyZero() || ToProjected.IsNearlyZero())
	{
		return 0.0f;
	}
	return FMath::Atan2(FVector::DotProduct(Axis, FVector::CrossProduct(FromProjected, ToProjected)),
		FVector::DotProduct(FromProjected, ToProjected));
}
