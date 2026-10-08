// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h" 
#include "BaseSight.generated.h"

UCLASS()
class ADBELLUM_API ABaseSight : public AActor 
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseSight();

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class USceneComponent* RootSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SightView;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> ADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	FName SocketName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* SightArrow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AimConfig")
	float BaseMouseSensitivity = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AimConfig")
	float SightFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AimConfig")
	bool bIsScoped = false;

	virtual void NotifyAim(bool bIsAiming);
	float GetBaseSensitivity();
	float GetSightFOV() const;
	bool GetIsScoped() const;
	void SetAimingMeshScale(bool bIsAiming);

	// Max angle between SightArrow direction and the line of sight before setup is reported as invalid
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Calibration")
	float MaxSightAxisDeviationDegrees = 0.5f;

	// How far in front of SightView (along SightArrow) the reticle appears to be.
	// 0 - reticle drawn directly at SightView (line of sight goes ADS -> SightView, sensitive to SightView placement).
	// Large value (e.g. 100000 = 1 km) - collimated/parallax reticle (red dot, reflex, holo using BumpOffset),
	// line of sight is effectively parallel to SightArrow and SightView placement errors do not matter.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Calibration", meta = (ClampMin = "0.0", Units = "cm"))
	float ReticleDistance = 0.0f;

	// Zero distances (meters) the player can step through with ScopeZeroUp/Down. Keep them sorted ascending.
	// Empty - zero distance of this sight cannot be changed, weapon's SightTargetDistance is used.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Calibration")
	TArray<float> ZeroDistances;

	// Index in ZeroDistances the sight is zeroed at by default
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Calibration", meta = (ClampMin = "0"))
	int32 DefaultZeroDistanceIndex = 0;

	// Point the eye looks at through the reticle: SightView + SightArrow direction * ReticleDistance
	UFUNCTION(BlueprintCallable, Category = "Sight|Calibration")
	FVector GetReticlePoint() const;

	// Eye point of the line of sight (ADS location, where the camera is placed)
	UFUNCTION(BlueprintCallable, Category = "Sight|Calibration")
	FVector GetSightLineOrigin() const;

	// Normalized direction of the line of sight: from ADS through the reticle point
	UFUNCTION(BlueprintCallable, Category = "Sight|Calibration")
	FVector GetSightLineDirection() const;

	// Direction the reticle is authored to look at: SightArrow forward, or SightView forward if there is no arrow
	FVector GetSightAxisDirection() const;

	// Checks that ADS lies on SightView's axis. Logs a warning once per sight when invalid.
	UFUNCTION(BlueprintCallable, Category = "Sight|Calibration")
	bool ValidateSightSetup() const;

	void DrawSightLineDebug(float Length) const;

	// Value of adb.Sight.DebugCalibration: 0 - off, 1 - locally controlled player weapon, 2 - all weapons
	static int32 GetCalibrationDebugMode();

private:
	mutable bool bSetupWarningLogged = false;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API USightChildActorComponent : public UChildActorComponent
{
	GENERATED_BODY()

public:
	USightChildActorComponent(const FObjectInitializer& Initializer);

	UFUNCTION(BlueprintCallable, Category = "Sight")
	ABaseSight* GetSightActor() const;
};