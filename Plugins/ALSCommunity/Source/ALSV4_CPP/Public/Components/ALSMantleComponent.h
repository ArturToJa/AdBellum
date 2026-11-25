// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community

#pragma once

#include "CoreMinimal.h"

#include "Character/ALSBaseCharacter.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "ALSMantleComponent.generated.h"

// forward declarations
class UALSDebugComponent;


UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API UALSMantleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UALSMantleComponent();

	UFUNCTION(BlueprintCallable, Category = "ALS|Mantle System")
	bool MantleCheck(const FALSMantleTraceSettings& TraceSettings,
	                 EDrawDebugTrace::Type DebugType);

	UFUNCTION(BlueprintCallable, Category = "ALS|Mantle System")
	void MantleStart(float MantleHeight, const FALSComponentAndTransform& MantleLedgeWS,
	                EALSMantleType MantleType);

	UFUNCTION(BlueprintCallable, Category = "ALS|Mantle System")
	void MantleUpdate(float BlendIn);

	UFUNCTION(BlueprintCallable, Category = "ALS|Mantle System")
	void MantleEnd();

	UFUNCTION(BlueprintCallable, Category = "ALS|Mantle System")
	void OnOwnerJumpInput();

	UFUNCTION(BlueprintCallable, Category = "ALS|Mantle System")
	void OnOwnerRagdollStateChanged(bool bRagdollState);

	/** Implement on BP to get correct mantle parameter set according to character state */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|Mantle System")
	FALSMantleAsset GetMantleAsset(EALSMantleType MantleType, EALSOverlayState CurrentOverlayState);

	/** Checks whether the obstacle ahead is thin enough to vault over (see MaxVaultableObstacleDepth) and,
	 * if there's room to land on the far side and clearance to pass above it, starts the vault. Returns
	 * false (doing nothing) if there's no obstacle, it's too deep, or there's no room/clearance - callers
	 * should fall back to MantleCheck in that case. */
	UFUNCTION(BlueprintCallable, Category = "ALS|Vault System")
	bool VaultCheck(const FALSMantleTraceSettings& TraceSettings, EDrawDebugTrace::Type DebugType);

	// Returns false (without starting anything) if GetVaultAsset has no montage for the current overlay
	// state, so VaultCheck can fall back to MantleCheck instead of silently doing nothing. anim_Vault has
	// no usable root motion (its root bone is static throughout - all travel is on the Pelvis bone
	// instead), so position is driven explicitly via VaultTimeline/VaultUpdate, same architecture as
	// Mantle: blended through an apex (standing on the ledge) so the path goes up and over the obstacle
	// instead of a straight line through it.
	UFUNCTION(BlueprintCallable, Category = "ALS|Vault System")
	bool VaultStart(float VaultHeight, const FALSComponentAndTransform& VaultLedgeWS,
	                const FALSComponentAndTransform& VaultLandingWS);

	UFUNCTION(BlueprintCallable, Category = "ALS|Vault System")
	void VaultUpdate(float Alpha);

	UFUNCTION(BlueprintCallable, Category = "ALS|Vault System")
	void VaultEnd();

	/** Implement on BP to get the vault montage (and play rate) for the current overlay state */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|Vault System")
	FALSVaultAsset GetVaultAsset(EALSOverlayState CurrentOverlayState);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// Called when the game starts
	virtual void BeginPlay() override;

	/** Mantling*/
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "ALS|Mantle System")
	void Server_MantleStart(float MantleHeight, const FALSComponentAndTransform& MantleLedgeWS,
	                        EALSMantleType MantleType);

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "ALS|Mantle System")
	void Multicast_MantleStart(float MantleHeight, const FALSComponentAndTransform& MantleLedgeWS,
	                           EALSMantleType MantleType);

	/** Vaulting */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "ALS|Vault System")
	void Server_VaultStart(float VaultHeight, const FALSComponentAndTransform& VaultLedgeWS,
	                       const FALSComponentAndTransform& VaultLandingWS);

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable, Category = "ALS|Vault System")
	void Multicast_VaultStart(float VaultHeight, const FALSComponentAndTransform& VaultLedgeWS,
	                          const FALSComponentAndTransform& VaultLandingWS);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Mantle System")
	TObjectPtr<UTimelineComponent> MantleTimeline = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Mantle System")
	FALSMantleTraceSettings GroundedTraceSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Mantle System")
	FALSMantleTraceSettings AutomaticTraceSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Mantle System")
	FALSMantleTraceSettings FallingTraceSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Mantle System")
	TObjectPtr<UCurveFloat> MantleTimelineCurve;

	static FName NAME_IgnoreOnlyPawn;
	/** Profile to use to detect objects we allow mantling */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Mantle System")
	FName MantleObjectDetectionProfile = NAME_IgnoreOnlyPawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Mantle System")
	TEnumAsByte<ECollisionChannel> WalkableSurfaceDetectionChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Mantle System")
	FALSMantleParams MantleParams;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Mantle System")
	FALSComponentAndTransform MantleLedgeLS;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Mantle System")
	FTransform MantleTarget = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Mantle System")
	FTransform MantleActualStartOffset = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Mantle System")
	FTransform MantleAnimatedStartOffset = FTransform::Identity;

	/** If a dynamic object has a velocity bigger than this value, do not start mantle */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Mantle System")
	float AcceptableVelocityWhileMantling = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ALS|Vault System")
	TObjectPtr<UTimelineComponent> VaultTimeline = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Vault System")
	FALSMantleTraceSettings VaultTraceSettings = {250.0f, 40.0f, 75.0f, 30.0f, 30.0f};

	/** Obstacles shallower (front-to-back) than this are vaulted over; deeper ones fall back to mantling
	 * and standing on top. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Vault System")
	float MaxVaultableObstacleDepth = 50.0f;

	/** Extra distance landed past where the obstacle's top is measured to end, so the standing capsule
	 * doesn't clip a post/brace right at that boundary. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Vault System")
	float VaultLandingClearance = 40.0f;

	/** Obstacles taller than this (measured from the character's feet to the ledge) are too high to
	 * vault even if thin enough - falls back to mantling and standing on top instead. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ALS|Vault System")
	float MaxVaultableHeight = 120.0f;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Vault System")
	FTransform VaultStartTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Vault System")
	FTransform VaultTargetTransform = FTransform::Identity;

	// Blended through at the midpoint of the vault (see VaultUpdate) so the path goes up and over the
	// ledge instead of a straight line from before the obstacle to after it, which would cut through its
	// own volume (collision is off for the duration of the vault, so nothing else would stop that).
	// Also doubles as the "grab" target for the Mantle-style position-correction blend below, the same
	// way MantleTarget does in Mantle - it's the transform the character's hands should land on.
	UPROPERTY(BlueprintReadOnly, Category = "ALS|Vault System")
	FTransform VaultApexTransform = FTransform::Identity;

	// Resolved per-vault copy of GetVaultAsset's fields (AnimMontage/PositionCorrectionCurve/PlayRate/
	// StartingOffset), set in VaultStart. Mirrors MantleParams.
	UPROPERTY(BlueprintReadOnly, Category = "ALS|Vault System")
	FALSVaultParams VaultParams;

	// Mirrors MantleActualStartOffset/MantleAnimatedStartOffset, but relative to VaultApexTransform
	// instead of MantleTarget: VaultActualStartOffset is where the character really started relative to
	// the ledge, VaultAnimatedStartOffset is where the authored animation assumes the grab starts
	// (derived from VaultParams.StartingOffset). Only meaningful when VaultParams.PositionCorrectionCurve
	// is set - see VaultUpdate.
	UPROPERTY(BlueprintReadOnly, Category = "ALS|Vault System")
	FTransform VaultActualStartOffset = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Vault System")
	FTransform VaultAnimatedStartOffset = FTransform::Identity;

private:
	// Procedural ease-in/out curve driving VaultTimeline's blend alpha; its single interior key is
	// rescaled to match each vault's montage length in VaultStart, so no curve asset needs authoring.
	UPROPERTY()
	TObjectPtr<UCurveFloat> VaultBlendCurve = nullptr;

	// Diagnostic-only: while > 0, TickComponent logs position/velocity each frame and decrements this,
	// to catch any post-vault "sweep" in the act.
	int32 PostVaultDebugFrames = 0;

	UPROPERTY()
	TObjectPtr<AALSBaseCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UALSDebugComponent> ALSDebugComponent = nullptr;
};
