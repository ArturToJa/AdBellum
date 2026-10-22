// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class AEBBullet;
class UEBBarrel;
class UWorld;

// Ballistic helpers shared by every weapon that zeroes a line of sight (ABaseWeapon sights, ASimpleWeapon ADS)
namespace WeaponBallistics
{
	// Bullet used for calibration: chambered bullet, otherwise first bullet in Barrel ammo, otherwise nullptr
	ADBELLUM_API TSubclassOf<AEBBullet> GetCalibrationBulletClass(const UEBBarrel* Barrel);

	// Simulates BulletClass without collision in a level frame (X downrange, Z up), starting at Start (world
	// location, air density depends on altitude). OutRangePoint is the trajectory point at DistanceMeters,
	// relative to Start, in that frame.
	ADBELLUM_API bool SimulateTrajectoryPoint(UWorld* World, const UEBBarrel* Barrel, TSubclassOf<AEBBullet> BulletClass,
		const FVector& Start, float DistanceMeters, FVector& OutRangePoint, float& OutFlightTime,
		TArray<FVector>* OutTrajectory = nullptr);

	// Signed angle (radians) rotating From to To around Axis, both projected onto the plane perpendicular to Axis
	ADBELLUM_API float SignedAngleAroundAxis(const FVector& From, const FVector& To, const FVector& Axis);
}
