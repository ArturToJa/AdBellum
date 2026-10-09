// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h" 
#include "BaseSight.generated.h"

class USoundBase;

// Sounds a sight makes when the player adjusts it
UENUM(BlueprintType)
enum class ESightAdjustSound : uint8
{
	ZoomIn,
	ZoomOut,
	ZeroChange
};

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

	// Played at the sight when zoom switched to a higher magnification. Give the sound an attenuation
	// to limit how far other players hear it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Sound")
	TObjectPtr<USoundBase> ZoomInSound;

	// Played at the sight when zoom switched to a lower magnification
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Sound")
	TObjectPtr<USoundBase> ZoomOutSound;

	// Played at the sight when the zero distance was changed
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Sound")
	TObjectPtr<USoundBase> ZeroChangeSound;

	USoundBase* GetAdjustSound(ESightAdjustSound Sound) const;

	virtual void NotifyAim(bool bIsAiming);
	float GetBaseSensitivity();
	float GetSightFOV() const;
	bool GetIsScoped() const;
	void SetAimingMeshScale(bool bIsAiming);

	// Magnifications the player can switch between with ZoomAction, e.g. 2, 4, 8, 16 for 2x, 4x, 8x, 16x.
	// 1x is the unmagnified view (UnmagnifiedFOV). Empty - fixed zoom given by SightFOV.
	// Only used by scoped sights (bIsScoped), which zoom with the camera FOV. SightFOV stays the FOV the
	// sight mesh is authored for: at that zoom the mesh has its authored size, at other zooms it is scaled
	// so the scope keeps the same size on screen.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom")
	TArray<float> ZoomLevels;

	// Index in ZoomLevels the sight starts at
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom", meta = (ClampMin = "0"))
	int32 DefaultZoomLevelIndex = 0;

	// Camera FOV of the unmagnified (1x) view, magnification is measured against it
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom", meta = (ClampMin = "1.0", ClampMax = "170.0"))
	float UnmagnifiedFOV = 90.0f;

	// True when the player can switch zoom on this sight
	UFUNCTION(BlueprintCallable, Category = "Sight|Zoom")
	bool HasZoomLevels() const;

	// Current magnification: selected zoom level, or the one SightFOV corresponds to when there are no zoom levels
	UFUNCTION(BlueprintCallable, Category = "Sight|Zoom")
	float GetCurrentMagnification() const;

	// Selects the next zoom level, wraps around after the last one. Returns false when there is nothing to switch.
	bool CycleZoomLevel();

	// Seconds a switch between two zoom levels takes, 0 switches instantly
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom", meta = (ClampMin = "0.0", Units = "s"))
	float ZoomTransitionTime = 0.2f;

	// Scales BaseMouseSensitivity with the visible FOV (BaseMouseSensitivity then applies at SightFOV).
	// Leave OFF while the look actions in the input mapping context have a FOV Scaling modifier (IMC_Default
	// does): that modifier already scales mouse input with the camera FOV, enabling both scales it twice.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom")
	bool bScaleSensitivityWithZoom = false;

	// Advances the transition between zoom levels. Returns true while the visible zoom is still changing
	// (FOV, mesh scale and sensitivity have to be applied again).
	bool TickZoomTransition(float DeltaTime);

	// Scalar parameters of the SightView (reticle) material that are adjusted with zoom.
	// While aiming: parameter = authored value * MeshZoomScale ^ exponent, where MeshZoomScale is how much
	// the scope mesh is enlarged at the current zoom (1 at SightFOV, 2 at half that magnification, ...).
	// Exponent 0 leaves the parameter alone. NAME_None or a parameter the material does not have is skipped.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom|Reticle Material")
	FName ReticleParallaxParameter = TEXT("ParalaxDistance");

	// -1 cancels the effect the enlarged reticle plane has on a BumpOffset parallax reticle,
	// so the reticle looks the same at every zoom
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom|Reticle Material")
	float ReticleParallaxZoomExponent = -1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom|Reticle Material")
	FName ReticleScaleParameter = TEXT("Scale");

	// 0 keeps the reticle the same size on screen at every zoom. 1 or -1 makes it grow or shrink with zoom.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sight|Zoom|Reticle Material")
	float ReticleScaleZoomExponent = 0.0f;

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

	float MagnificationToFOV(float Magnification) const;
	int32 GetZoomLevelIndex() const;

	// Selected entry of ZoomLevels, INDEX_NONE until first used (then DefaultZoomLevelIndex)
	int32 CurrentZoomLevelIndex = INDEX_NONE;

	// Magnification currently shown, moves towards the selected zoom level during a transition
	float GetDisplayedMagnification() const;
	// How much wider the visible FOV is than SightFOV: tan(FOV / 2) / tan(SightFOV / 2)
	float GetZoomFOVScale() const;
	float ZoomTransitionFrom = 0.0f;
	float ZoomTransitionAlpha = 1.0f;

	// Authored transforms of the parts SetAimingMeshScale changes, captured before the first change
	void CacheAuthoredSightTransforms();
	bool bAuthoredSightTransformsCached = false;
	FVector AuthoredSightMeshLocation = FVector::ZeroVector;
	FVector AuthoredSightMeshScale = FVector::OneVector;
	FVector AuthoredSightViewScale = FVector::OneVector;

	// Reticle material of SightView (slot 0) and its authored parameter values, only for sights with zoom levels
	void ApplyZoomToReticleMaterial(float MeshZoomScale);
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ReticleMaterial;
	bool bReticleMaterialCached = false;
	bool bHasReticleParallaxParameter = false;
	bool bHasReticleScaleParameter = false;
	float AuthoredReticleParallax = 0.0f;
	float AuthoredReticleScale = 0.0f;
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