// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#include "Components/ALSMantleComponent.h"


#include "Character/ALSCharacter.h"
#include "Character/Animation/ALSCharacterAnimInstance.h"
#include "Components/ALSDebugComponent.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Library/ALSMathLibrary.h"


const FName NAME_MantleEnd(TEXT("MantleEnd"));
const FName NAME_MantleUpdate(TEXT("MantleUpdate"));
const FName NAME_MantleTimeline(TEXT("MantleTimeline"));
const FName NAME_VaultEnd(TEXT("VaultEnd"));
const FName NAME_VaultUpdate(TEXT("VaultUpdate"));
const FName NAME_VaultTimeline(TEXT("VaultTimeline"));

FName UALSMantleComponent::NAME_IgnoreOnlyPawn(TEXT("IgnoreOnlyPawn"));


UALSMantleComponent::UALSMantleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	MantleTimeline = CreateDefaultSubobject<UTimelineComponent>(NAME_MantleTimeline);
	VaultTimeline = CreateDefaultSubobject<UTimelineComponent>(NAME_VaultTimeline);
}

void UALSMantleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner())
	{
		OwnerCharacter = Cast<AALSBaseCharacter>(GetOwner());
		if (OwnerCharacter)
		{
			ALSDebugComponent = OwnerCharacter->FindComponentByClass<UALSDebugComponent>();

			AddTickPrerequisiteActor(OwnerCharacter); // Always tick after owner, so we'll use updated values

			// Bindings
			FOnTimelineFloat TimelineUpdated;
			FOnTimelineEvent TimelineFinished;
			TimelineUpdated.BindUFunction(this, NAME_MantleUpdate);
			TimelineFinished.BindUFunction(this, NAME_MantleEnd);
			MantleTimeline->SetTimelineFinishedFunc(TimelineFinished);
			MantleTimeline->SetLooping(false);
			MantleTimeline->SetTimelineLengthMode(TL_TimelineLength);
			MantleTimeline->AddInterpFloat(MantleTimelineCurve, TimelineUpdated);

			VaultBlendCurve = NewObject<UCurveFloat>(this, NAME_None, RF_Transient);
			VaultBlendCurve->FloatCurve.AddKey(0.0f, 0.0f);
			FKeyHandle EndKeyHandle = VaultBlendCurve->FloatCurve.AddKey(1.0f, 1.0f);
			VaultBlendCurve->FloatCurve.SetKeyInterpMode(EndKeyHandle, RCIM_Cubic);

			FOnTimelineFloat VaultTimelineUpdated;
			FOnTimelineEvent VaultTimelineFinished;
			VaultTimelineUpdated.BindUFunction(this, NAME_VaultUpdate);
			VaultTimelineFinished.BindUFunction(this, NAME_VaultEnd);
			VaultTimeline->SetTimelineFinishedFunc(VaultTimelineFinished);
			VaultTimeline->SetLooping(false);
			VaultTimeline->SetTimelineLengthMode(TL_TimelineLength);
			// VaultBlendCurve's keys stay fixed at [0, 1]; VaultStart rescales how fast playback moves
			// through that range (via SetPlayRate) to match each vault's montage length.
			VaultTimeline->SetTimelineLength(1.0f);
			VaultTimeline->AddInterpFloat(VaultBlendCurve, VaultTimelineUpdated);

			OwnerCharacter->JumpPressedDelegate.AddUniqueDynamic(this, &UALSMantleComponent::OnOwnerJumpInput);
			OwnerCharacter->RagdollStateChangedDelegate.AddUniqueDynamic(
				this, &UALSMantleComponent::OnOwnerRagdollStateChanged);
		}
	}
}


void UALSMantleComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PostVaultDebugFrames > 0 && OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] PostVaultTick[%d]: Role=%d ActorLoc=%s Rot=%s Velocity=%s Acceleration=%s PendingInput=%s MovementAction=%d MovementState=%d HasMovementInput=%d HasAnyRootMotion=%d MovementMode=%d RequestedVelocity=%s Controller=%s"),
			PostVaultDebugFrames, (int32)OwnerCharacter->GetLocalRole(), *OwnerCharacter->GetActorLocation().ToString(),
			*OwnerCharacter->GetActorRotation().ToString(),
			*OwnerCharacter->GetCharacterMovement()->Velocity.ToString(),
			*OwnerCharacter->GetCharacterMovement()->GetCurrentAcceleration().ToString(),
			*OwnerCharacter->GetPendingMovementInputVector().ToString(),
			(int32)OwnerCharacter->GetMovementAction(), (int32)OwnerCharacter->GetMovementState(),
			OwnerCharacter->HasMovementInput(), OwnerCharacter->HasAnyRootMotion(),
			(int32)OwnerCharacter->GetCharacterMovement()->MovementMode.GetValue(),
			*OwnerCharacter->GetCharacterMovement()->RequestedVelocity.ToString(),
			*GetNameSafe(OwnerCharacter->GetController()));
		--PostVaultDebugFrames;
	}

	if (OwnerCharacter && OwnerCharacter->GetMovementState() == EALSMovementState::InAir)
	{
		// Perform a mantle check if falling while movement input is pressed. This runs every tick
		// regardless of jump input, so without trying Vault first here too, this automatic check grabs
		// any mantleable-height obstacle via mantling before a deliberate jump press (which also tries
		// Vault first, in OnOwnerJumpInput) ever gets a chance to - same Vault-first, Mantle-fallback
		// pattern as that function.
		if (OwnerCharacter->HasMovementInput())
		{
			const bool bVaulted = VaultCheck(VaultTraceSettings, EDrawDebugTrace::Type::ForOneFrame);
			if (!bVaulted)
			{
				MantleCheck(FallingTraceSettings, EDrawDebugTrace::Type::ForOneFrame);
			}
		}
	}
}

void UALSMantleComponent::MantleStart(float MantleHeight, const FALSComponentAndTransform& MantleLedgeWS,
                                      EALSMantleType MantleType)
{
	if (OwnerCharacter == nullptr || !IsValid(MantleLedgeWS.Component) || !IsValid(MantleTimeline))
	{
		return;
	}

	if (MantleType != EALSMantleType::LowMantle && OwnerCharacter->IsA(AALSCharacter::StaticClass()))
	{
		Cast<AALSCharacter>(OwnerCharacter)->ClearHeldObject();
	}

	// Disable ticking during mantle
	SetComponentTickEnabledAsync(false);

	// Step 1: Get the Mantle Asset and use it to set the new Mantle Params.
	const FALSMantleAsset MantleAsset = GetMantleAsset(MantleType, OwnerCharacter->GetOverlayState());
	check(MantleAsset.PositionCorrectionCurve)

	MantleParams.AnimMontage = MantleAsset.AnimMontage;
	MantleParams.PositionCorrectionCurve = MantleAsset.PositionCorrectionCurve;
	MantleParams.StartingOffset = MantleAsset.StartingOffset;
	MantleParams.StartingPosition = FMath::GetMappedRangeValueClamped<float, float>({MantleAsset.LowHeight, MantleAsset.HighHeight},
	                                                                  {
		                                                                  MantleAsset.LowStartPosition,
		                                                                  MantleAsset.HighStartPosition
	                                                                  },
	                                                                  MantleHeight);
	MantleParams.PlayRate = FMath::GetMappedRangeValueClamped<float, float>({MantleAsset.LowHeight, MantleAsset.HighHeight},
	                                                          {MantleAsset.LowPlayRate, MantleAsset.HighPlayRate},
	                                                          MantleHeight);

	// Step 2: Convert the world space target to the mantle component's local space for use in moving objects.
	MantleLedgeLS.Component = MantleLedgeWS.Component;
	MantleLedgeLS.Transform = MantleLedgeWS.Transform * MantleLedgeWS.Component->GetComponentToWorld().Inverse();

	// Step 3: Set the Mantle Target and calculate the Starting Offset
	// (offset amount between the actor and target transform).
	MantleTarget = MantleLedgeWS.Transform;
	MantleActualStartOffset = UALSMathLibrary::TransformSub(OwnerCharacter->GetActorTransform(), MantleTarget);

	// Step 4: Calculate the Animated Start Offset from the Target Location.
	// This would be the location the actual animation starts at relative to the Target Transform.
	FVector RotatedVector = MantleTarget.GetRotation().Vector() * MantleParams.StartingOffset.Y;
	RotatedVector.Z = MantleParams.StartingOffset.Z;
	const FTransform StartOffset(MantleTarget.Rotator(), MantleTarget.GetLocation() - RotatedVector,
	                             FVector::OneVector);
	MantleAnimatedStartOffset = UALSMathLibrary::TransformSub(StartOffset, MantleTarget);

	// Step 5: Clear the Character Movement Mode and set the Movement State to Mantling
	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_None);
	OwnerCharacter->SetMovementState(EALSMovementState::Mantling);

	// Step 6: Configure the Mantle Timeline so that it is the same length as the
	// Lerp/Correction curve minus the starting position, and plays at the same speed as the animation.
	// Then start the timeline.
	float MinTime = 0.0f;
	float MaxTime = 0.0f;
	MantleParams.PositionCorrectionCurve->GetTimeRange(MinTime, MaxTime);
	MantleTimeline->SetTimelineLength(MaxTime - MantleParams.StartingPosition);
	MantleTimeline->SetPlayRate(MantleParams.PlayRate);
	MantleTimeline->PlayFromStart();

	// Step 7: Play the Anim Montage if valid.
	if (MantleParams.AnimMontage && OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(MantleParams.AnimMontage, MantleParams.PlayRate,
		                                                    EMontagePlayReturnType::MontageLength,
		                                                    MantleParams.StartingPosition, false);
	}
}

bool UALSMantleComponent::MantleCheck(const FALSMantleTraceSettings& TraceSettings, EDrawDebugTrace::Type DebugType)
{
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] MantleCheck called, ReachDistance=%.1f MinLedgeHeight=%.1f MaxLedgeHeight=%.1f ForwardTraceRadius=%.1f DownwardTraceRadius=%.1f"),
		TraceSettings.ReachDistance, TraceSettings.MinLedgeHeight, TraceSettings.MaxLedgeHeight,
		TraceSettings.ForwardTraceRadius, TraceSettings.DownwardTraceRadius);

	if (!OwnerCharacter)
	{
		return false;
	}

	// Step 1: Trace forward to find a wall / object the character cannot walk on.
	const FVector& TraceDirection = OwnerCharacter->GetActorForwardVector();
	const FVector& CapsuleBaseLocation = UALSMathLibrary::GetCapsuleBaseLocation(
		2.0f, OwnerCharacter->GetCapsuleComponent());
	FVector TraceStart = CapsuleBaseLocation + TraceDirection * -30.0f;
	TraceStart.Z += (TraceSettings.MaxLedgeHeight + TraceSettings.MinLedgeHeight) / 2.0f;
	const FVector TraceEnd = TraceStart + TraceDirection * TraceSettings.ReachDistance;
	const float HalfHeight = 1.0f + (TraceSettings.MaxLedgeHeight - TraceSettings.MinLedgeHeight) / 2.0f;

	UWorld* World = GetWorld();
	check(World);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	FHitResult HitResult;
	{
		const FCollisionShape CapsuleCollisionShape = FCollisionShape::MakeCapsule(TraceSettings.ForwardTraceRadius, HalfHeight);
		const bool bHit = World->SweepSingleByProfile(HitResult, TraceStart, TraceEnd, FQuat::Identity, MantleObjectDetectionProfile,
	                                                  CapsuleCollisionShape, Params);

		if (ALSDebugComponent && ALSDebugComponent->GetShowTraces())
		{
			UALSDebugComponent::DrawDebugCapsuleTraceSingle(World,
			                                                TraceStart,
			                                                TraceEnd,
			                                                CapsuleCollisionShape,
			                                                DebugType,
			                                                bHit,
			                                                HitResult,
			                                                FLinearColor::Black,
			                                                FLinearColor::Black,
			                                                1.0f);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] MantleCheck Step1: bHit=%d Walkable=%d Component=%s ImpactZ=%.2f"),
		HitResult.IsValidBlockingHit(),
		HitResult.IsValidBlockingHit() && OwnerCharacter->GetCharacterMovement()->IsWalkable(HitResult),
		*GetNameSafe(HitResult.GetComponent()), HitResult.ImpactPoint.Z);

	if (!HitResult.IsValidBlockingHit() || OwnerCharacter->GetCharacterMovement()->IsWalkable(HitResult))
	{
		// Not a valid surface to mantle
		return false;
	}

	if (HitResult.GetComponent() != nullptr)
	{
		UPrimitiveComponent* PrimitiveComponent = HitResult.GetComponent();
		if (PrimitiveComponent && PrimitiveComponent->GetComponentVelocity().Size() > AcceptableVelocityWhileMantling)
		{
			// The surface to mantle moves too fast
			return false;
		}
	}

	const FVector InitialTraceImpactPoint = HitResult.ImpactPoint;
	const FVector InitialTraceNormal = HitResult.ImpactNormal;

	// Step 2: Trace downward from the first trace's Impact Point and determine if the hit location is walkable.
	FVector DownwardTraceEnd = InitialTraceImpactPoint;
	DownwardTraceEnd.Z = CapsuleBaseLocation.Z;
	DownwardTraceEnd += InitialTraceNormal * -15.0f;
	FVector DownwardTraceStart = DownwardTraceEnd;
	DownwardTraceStart.Z += TraceSettings.MaxLedgeHeight + TraceSettings.DownwardTraceRadius + 1.0f;

	{
		const FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(TraceSettings.DownwardTraceRadius);
		const bool bHit = World->SweepSingleByChannel(HitResult, DownwardTraceStart, DownwardTraceEnd, FQuat::Identity,
	                                                  WalkableSurfaceDetectionChannel, SphereCollisionShape,
	                                                  Params);

		if (ALSDebugComponent && ALSDebugComponent->GetShowTraces())
		{
			UALSDebugComponent::DrawDebugSphereTraceSingle(World,
			                                               TraceStart,
			                                               TraceEnd,
			                                               SphereCollisionShape,
			                                               DebugType,
			                                               bHit,
			                                               HitResult,
			                                               FLinearColor::Black,
			                                               FLinearColor::Black,
			                                               1.0f);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] MantleCheck Step2 downward trace: Start=%s End=%s bHit=%d Component=%s ImpactZ=%.2f Walkable=%d TraceSettings(MaxLedgeHeight=%.1f DownwardTraceRadius=%.1f)"),
		*DownwardTraceStart.ToString(), *DownwardTraceEnd.ToString(), HitResult.IsValidBlockingHit(),
		*GetNameSafe(HitResult.GetComponent()), HitResult.ImpactPoint.Z,
		HitResult.IsValidBlockingHit() && OwnerCharacter->GetCharacterMovement()->IsWalkable(HitResult),
		TraceSettings.MaxLedgeHeight, TraceSettings.DownwardTraceRadius);

	if (!OwnerCharacter->GetCharacterMovement()->IsWalkable(HitResult))
	{
		// Not a valid surface to mantle
		return false;
	}

	const FVector DownTraceLocation(HitResult.Location.X, HitResult.Location.Y, HitResult.ImpactPoint.Z);
	UPrimitiveComponent* HitComponent = HitResult.GetComponent();

	// Step 3: Check if the capsule has room to stand at the downward trace's location.
	// If so, set that location as the Target Transform and calculate the mantle height.
	const FVector& CapsuleLocationFBase = UALSMathLibrary::GetCapsuleLocationFromBase(
		DownTraceLocation, 2.0f, OwnerCharacter->GetCapsuleComponent());
	const bool bCapsuleHasRoom = UALSMathLibrary::CapsuleHasRoomCheck(OwnerCharacter->GetCapsuleComponent(),
	                                                                  CapsuleLocationFBase, 0.0f,
	                                                                  0.0f, DebugType, ALSDebugComponent && ALSDebugComponent->GetShowTraces());

	if (!bCapsuleHasRoom)
	{
		// Capsule doesn't have enough room to mantle
		return false;
	}

	const FTransform TargetTransform(
		(InitialTraceNormal * FVector(-1.0f, -1.0f, 0.0f)).ToOrientationRotator(),
		CapsuleLocationFBase,
		FVector::OneVector);

	const float MantleHeight = (CapsuleLocationFBase - OwnerCharacter->GetActorLocation()).Z;

	// Step 4: Determine the Mantle Type by checking the movement mode and Mantle Height.
	EALSMantleType MantleType;
	if (OwnerCharacter->GetMovementState() == EALSMovementState::InAir)
	{
		MantleType = EALSMantleType::FallingCatch;
	}
	else
	{
		MantleType = MantleHeight > 125.0f ? EALSMantleType::HighMantle : EALSMantleType::LowMantle;
	}

	// Step 5: If everything checks out, start the Mantle
	FALSComponentAndTransform MantleWS;
	MantleWS.Component = HitComponent;
	MantleWS.Transform = TargetTransform;
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] MantleCheck succeeded: MantleHeight=%.2f MantleType=%d"), MantleHeight, (int32)MantleType);
	MantleStart(MantleHeight, MantleWS, MantleType);
	Server_MantleStart(MantleHeight, MantleWS, MantleType);

	return true;
}

void UALSMantleComponent::Server_MantleStart_Implementation(float MantleHeight,
                                                            const FALSComponentAndTransform& MantleLedgeWS,
                                                            EALSMantleType MantleType)
{
	Multicast_MantleStart(MantleHeight, MantleLedgeWS, MantleType);
}

void UALSMantleComponent::Multicast_MantleStart_Implementation(float MantleHeight,
                                                               const FALSComponentAndTransform& MantleLedgeWS,
                                                               EALSMantleType MantleType)
{
	if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled())
	{
		MantleStart(MantleHeight, MantleLedgeWS, MantleType);
	}
}

// This function is called by "MantleTimeline" using BindUFunction in UALSMantleComponent::BeginPlay during the default settings initialization.
void UALSMantleComponent::MantleUpdate(float BlendIn)
{
	if (!OwnerCharacter)
	{
		return;
	}

	// Step 1: Continually update the mantle target from the stored local transform to follow along with moving objects
	MantleTarget = UALSMathLibrary::MantleComponentLocalToWorld(MantleLedgeLS);

	// Step 2: Update the Position and Correction Alphas using the Position/Correction curve set for each Mantle.
	const FVector CurveVec = MantleParams.PositionCorrectionCurve
	                                     ->GetVectorValue(
		                                     MantleParams.StartingPosition + MantleTimeline->GetPlaybackPosition());
	const float PositionAlpha = CurveVec.X;
	const float XYCorrectionAlpha = CurveVec.Y;
	const float ZCorrectionAlpha = CurveVec.Z;

	// Step 3: Lerp multiple transforms together for independent control over the horizontal
	// and vertical blend to the animated start position, as well as the target position.

	// Blend into the animated horizontal and rotation offset using the Y value of the Position/Correction Curve.
	const FTransform TargetHzTransform(MantleAnimatedStartOffset.GetRotation(),
	                                   {
		                                   MantleAnimatedStartOffset.GetLocation().X,
		                                   MantleAnimatedStartOffset.GetLocation().Y,
		                                   MantleActualStartOffset.GetLocation().Z
	                                   },
	                                   FVector::OneVector);
	const FTransform& HzLerpResult =
		UKismetMathLibrary::TLerp(MantleActualStartOffset, TargetHzTransform, XYCorrectionAlpha);

	// Blend into the animated vertical offset using the Z value of the Position/Correction Curve.
	const FTransform TargetVtTransform(MantleActualStartOffset.GetRotation(),
	                                   {
		                                   MantleActualStartOffset.GetLocation().X,
		                                   MantleActualStartOffset.GetLocation().Y,
		                                   MantleAnimatedStartOffset.GetLocation().Z
	                                   },
	                                   FVector::OneVector);
	const FTransform& VtLerpResult =
		UKismetMathLibrary::TLerp(MantleActualStartOffset, TargetVtTransform, ZCorrectionAlpha);

	const FTransform ResultTransform(HzLerpResult.GetRotation(),
	                                 {
		                                 HzLerpResult.GetLocation().X, HzLerpResult.GetLocation().Y,
		                                 VtLerpResult.GetLocation().Z
	                                 },
	                                 FVector::OneVector);

	// Blend from the currently blending transforms into the final mantle target using the X
	// value of the Position/Correction Curve.
	const FTransform& ResultLerp = UKismetMathLibrary::TLerp(
		UALSMathLibrary::TransformAdd(MantleTarget, ResultTransform), MantleTarget,
		PositionAlpha);

	// Initial Blend In (controlled in the timeline curve) to allow the actor to blend into the Position/Correction
	// curve at the midpoint. This prevents pops when mantling an object lower than the animated mantle.
	const FTransform& LerpedTarget =
		UKismetMathLibrary::TLerp(UALSMathLibrary::TransformAdd(MantleTarget, MantleActualStartOffset), ResultLerp,
		                          BlendIn);

	// Step 4: Set the actors location and rotation to the Lerped Target.
	OwnerCharacter->SetActorLocationAndTargetRotation(LerpedTarget.GetLocation(), LerpedTarget.GetRotation().Rotator());
}

void UALSMantleComponent::MantleEnd()
{
	// Set the Character Movement Mode to Walking
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		if (OwnerCharacter->IsA(AALSCharacter::StaticClass()))
		{
			Cast<AALSCharacter>(OwnerCharacter)->UpdateHeldObject();
		}
	}

	// Enable ticking back after mantle ends
	SetComponentTickEnabledAsync(true);
}

bool UALSMantleComponent::VaultCheck(const FALSMantleTraceSettings& TraceSettings, EDrawDebugTrace::Type DebugType)
{
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultCheck called, MaxVaultableObstacleDepth=%.1f ReachDistance=%.1f MinLedgeHeight=%.1f MaxLedgeHeight=%.1f ForwardTraceRadius=%.1f DownwardTraceRadius=%.1f"),
		MaxVaultableObstacleDepth, TraceSettings.ReachDistance, TraceSettings.MinLedgeHeight, TraceSettings.MaxLedgeHeight,
		TraceSettings.ForwardTraceRadius, TraceSettings.DownwardTraceRadius);

	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultCheck: no OwnerCharacter"));
		return false;
	}

	// Step 1: Trace forward to find a wall / object the character cannot walk on (same approach as MantleCheck).
	const FVector& TraceDirection = OwnerCharacter->GetActorForwardVector();
	const FVector& CapsuleBaseLocation = UALSMathLibrary::GetCapsuleBaseLocation(
		2.0f, OwnerCharacter->GetCapsuleComponent());
	FVector TraceStart = CapsuleBaseLocation + TraceDirection * -30.0f;
	TraceStart.Z += (TraceSettings.MaxLedgeHeight + TraceSettings.MinLedgeHeight) / 2.0f;
	const FVector TraceEnd = TraceStart + TraceDirection * TraceSettings.ReachDistance;
	const float HalfHeight = 1.0f + (TraceSettings.MaxLedgeHeight - TraceSettings.MinLedgeHeight) / 2.0f;

	UWorld* World = GetWorld();
	check(World);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	FHitResult HitResult;
	{
		const FCollisionShape CapsuleCollisionShape = FCollisionShape::MakeCapsule(TraceSettings.ForwardTraceRadius, HalfHeight);
		const bool bHit = World->SweepSingleByProfile(HitResult, TraceStart, TraceEnd, FQuat::Identity, MantleObjectDetectionProfile,
	                                                  CapsuleCollisionShape, Params);

		if (ALSDebugComponent && ALSDebugComponent->GetShowTraces())
		{
			UALSDebugComponent::DrawDebugCapsuleTraceSingle(World, TraceStart, TraceEnd, CapsuleCollisionShape, DebugType,
			                                                bHit, HitResult, FLinearColor::Yellow, FLinearColor::Yellow, 1.0f);
		}
	}

	if (!HitResult.IsValidBlockingHit() || OwnerCharacter->GetCharacterMovement()->IsWalkable(HitResult))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step1 forward trace: no valid non-walkable obstacle hit (bHit=%d Walkable=%d Component=%s)"),
			HitResult.IsValidBlockingHit(), HitResult.IsValidBlockingHit() && OwnerCharacter->GetCharacterMovement()->IsWalkable(HitResult),
			*GetNameSafe(HitResult.GetComponent()));
		return false;
	}

	if (UPrimitiveComponent* HitPrimitive = HitResult.GetComponent())
	{
		if (HitPrimitive->GetComponentVelocity().Size() > AcceptableVelocityWhileMantling)
		{
			UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step1: obstacle %s moves too fast (%.1f > %.1f)"),
				*GetNameSafe(HitPrimitive), HitPrimitive->GetComponentVelocity().Size(), AcceptableVelocityWhileMantling);
			return false;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step1 OK: hit %s at %s normal=%s"),
		*GetNameSafe(HitResult.GetComponent()), *HitResult.ImpactPoint.ToString(), *HitResult.ImpactNormal.ToString());

	const FVector InitialTraceImpactPoint = HitResult.ImpactPoint;
	const FVector InitialTraceNormal = HitResult.ImpactNormal;

	// Vault's own downward/far probes only ever need to search up to roughly MaxVaultableHeight, not the
	// full (much taller) TraceSettings.MaxLedgeHeight band shared with Mantle's tall-ledge search. Keeping
	// these probes tight avoids snagging unrelated geometry well above a vaultable obstacle's height
	// (e.g. a nearby vehicle's hull/turret) and having it get mistaken for the ground.
	const float VaultSearchHeight = FMath::Min(TraceSettings.MaxLedgeHeight, MaxVaultableHeight + 50.0f);

	// Step 2: Trace downward from the first trace's impact point to find the obstacle's near-side top surface.
	FVector DownwardTraceEnd = InitialTraceImpactPoint;
	DownwardTraceEnd.Z = CapsuleBaseLocation.Z;
	DownwardTraceEnd += InitialTraceNormal * -15.0f;
	FVector DownwardTraceStart = DownwardTraceEnd;
	DownwardTraceStart.Z += VaultSearchHeight + TraceSettings.DownwardTraceRadius + 1.0f;

	// Uses a thin line trace rather than a sphere sweep, same reasoning as Step 3's far probe: a sphere's
	// radius can make the sweep start already overlapping unrelated geometry well above the real ledge
	// (e.g. something floating over the obstacle), registering as a non-blocking "start penetrating" hit
	// instead of cleanly finding the obstacle's own top.
	FHitResult NearTopHit;
	{
		World->LineTraceSingleByChannel(NearTopHit, DownwardTraceStart, DownwardTraceEnd, WalkableSurfaceDetectionChannel, Params);

		if (ALSDebugComponent && ALSDebugComponent->GetShowTraces())
		{
			const FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(1.0f);
			UALSDebugComponent::DrawDebugSphereTraceSingle(World, DownwardTraceStart, DownwardTraceEnd, SphereCollisionShape,
			                                               DebugType, NearTopHit.IsValidBlockingHit(), NearTopHit,
			                                               FLinearColor::Yellow, FLinearColor::Yellow, 1.0f);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step2 downward trace: Start=%s End=%s bHit=%d Component=%s ImpactZ=%.2f"),
		*DownwardTraceStart.ToString(), *DownwardTraceEnd.ToString(), NearTopHit.IsValidBlockingHit(),
		*GetNameSafe(NearTopHit.GetComponent()), NearTopHit.ImpactPoint.Z);

	if (!OwnerCharacter->GetCharacterMovement()->IsWalkable(NearTopHit))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step2 downward trace: top surface not walkable or no hit (bHit=%d Component=%s)"),
			NearTopHit.IsValidBlockingHit(), *GetNameSafe(NearTopHit.GetComponent()));
		return false;
	}

	const FVector NearTopLocation(NearTopHit.Location.X, NearTopHit.Location.Y, NearTopHit.ImpactPoint.Z);
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step2 OK: NearTopLocation=%s"), *NearTopLocation.ToString());

	const float ObstacleHeight = NearTopLocation.Z - CapsuleBaseLocation.Z;
	if (ObstacleHeight > MaxVaultableHeight)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step2: obstacle height %.1f exceeds MaxVaultableHeight %.1f - too high to vault"),
			ObstacleHeight, MaxVaultableHeight);
		return false;
	}

	// Step 3: Probe whether the obstacle's top surface ends within MaxVaultableObstacleDepth. If it's
	// still there that far in, the obstacle is too wide to vault - leave it for MantleCheck to stand on.
	// The search band is centered on the character's own ground level (not the obstacle's height), since
	// a vault landing should be roughly a normal step up/down, not defined by how tall the fence is.
	const FVector FarProbeXY = NearTopLocation + TraceDirection * MaxVaultableObstacleDepth;
	FVector FarProbeStart = FarProbeXY;
	FarProbeStart.Z = CapsuleBaseLocation.Z + VaultSearchHeight + TraceSettings.DownwardTraceRadius + 1.0f;
	FVector FarProbeEnd = FarProbeXY;
	FarProbeEnd.Z = CapsuleBaseLocation.Z - VaultSearchHeight - TraceSettings.DownwardTraceRadius - 1.0f;

	// Uses a thin line trace rather than a sphere: a sphere's own radius bleeds backward past the probe
	// point, so it can still touch the obstacle's near side even once the point itself has cleared it,
	// making shallow obstacles look deeper than they are.
	FHitResult FarHit;
	{
		World->LineTraceSingleByChannel(FarHit, FarProbeStart, FarProbeEnd, WalkableSurfaceDetectionChannel, Params);

		if (ALSDebugComponent && ALSDebugComponent->GetShowTraces())
		{
			const FCollisionShape SphereCollisionShape = FCollisionShape::MakeSphere(1.0f);
			UALSDebugComponent::DrawDebugSphereTraceSingle(World, FarProbeStart, FarProbeEnd, SphereCollisionShape,
			                                               DebugType, FarHit.IsValidBlockingHit(), FarHit,
			                                               FLinearColor::Blue, FLinearColor::Blue, 1.0f);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step3 far probe: Start=%s End=%s bHit=%d ImpactZ=%.1f Walkable=%d"),
		*FarProbeStart.ToString(), *FarProbeEnd.ToString(), FarHit.IsValidBlockingHit(), FarHit.ImpactPoint.Z,
		FarHit.IsValidBlockingHit() && OwnerCharacter->GetCharacterMovement()->IsWalkable(FarHit));

	if (!FarHit.IsValidBlockingHit() || !OwnerCharacter->GetCharacterMovement()->IsWalkable(FarHit))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step3: nothing solid to land on past the obstacle"));
		return false;
	}

	if (FarHit.ImpactPoint.Z > NearTopLocation.Z - 5.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step3: top surface still present at far probe (FarZ=%.1f NearTopZ=%.1f) - obstacle too deep"),
			FarHit.ImpactPoint.Z, NearTopLocation.Z);
		return false;
	}

	// The obstacle ends within range, but land VaultLandingClearance further out than that edge so the
	// standing capsule isn't immediately clipping whatever's right at the boundary (a post, brace, etc).
	FVector LandingProbeXY = NearTopLocation + TraceDirection * (MaxVaultableObstacleDepth + VaultLandingClearance);
	FVector LandingProbeFrom = LandingProbeXY;
	LandingProbeFrom.Z = FarProbeStart.Z;
	FVector LandingProbeTo = LandingProbeXY;
	LandingProbeTo.Z = FarProbeEnd.Z;
	FHitResult LandingHit;
	World->LineTraceSingleByChannel(LandingHit, LandingProbeFrom, LandingProbeTo, WalkableSurfaceDetectionChannel, Params);
	if (!LandingHit.IsValidBlockingHit() || !OwnerCharacter->GetCharacterMovement()->IsWalkable(LandingHit))
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step3: no walkable ground at the buffered landing distance (+%.0f)"),
			MaxVaultableObstacleDepth + VaultLandingClearance);
		return false;
	}

	// Sanity check: the landing surface should be roughly at the character's own ground level (a normal
	// step up/down), not something unrelated picked up within the search band (e.g. a nearby vehicle).
	if (FMath::Abs(LandingHit.ImpactPoint.Z - CapsuleBaseLocation.Z) > VaultSearchHeight)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step3: landing surface at Z=%.1f is too far from ground level Z=%.1f - likely unrelated geometry"),
			LandingHit.ImpactPoint.Z, CapsuleBaseLocation.Z);
		return false;
	}

	const FVector LandingLocation(LandingHit.Location.X, LandingHit.Location.Y, LandingHit.ImpactPoint.Z);
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step3 OK: LandingLocation=%s"), *LandingLocation.ToString());

	// Step 4: Check the capsule has room to land on the far side.
	const FVector& LandingCapsuleLocation = UALSMathLibrary::GetCapsuleLocationFromBase(
		LandingLocation, 2.0f, OwnerCharacter->GetCapsuleComponent());
	const bool bLandingHasRoom = UALSMathLibrary::CapsuleHasRoomCheck(OwnerCharacter->GetCapsuleComponent(),
	                                                                  LandingCapsuleLocation, 0.0f, 0.0f, DebugType,
	                                                                  ALSDebugComponent && ALSDebugComponent->GetShowTraces());
	if (!bLandingHasRoom)
	{
		// Diagnostic-only repeat of CapsuleHasRoomCheck's own sweep, just to log what's actually blocking it.
		const float ZTarget = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight_WithoutHemisphere();
		FVector RoomTraceStart = LandingCapsuleLocation;
		RoomTraceStart.Z += ZTarget;
		FVector RoomTraceEnd = LandingCapsuleLocation;
		RoomTraceEnd.Z -= ZTarget;
		FHitResult RoomHit;
		World->SweepSingleByChannel(RoomHit, RoomTraceStart, RoomTraceEnd, FQuat::Identity, ECC_Visibility,
		                            FCollisionShape::MakeSphere(OwnerCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius()), Params);
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step4: no room to land at %s - blocked by %s at %s"),
			*LandingCapsuleLocation.ToString(), *GetNameSafe(RoomHit.GetComponent()), *RoomHit.ImpactPoint.ToString());
		return false;
	}
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step4 OK: room to land at %s"), *LandingCapsuleLocation.ToString());

	// Step 5: Check there's clearance for the whole body to pass above the obstacle, not just the
	// narrow ledge-height band Step 1's trace covered. Sweeps purely horizontally at the obstacle's own
	// top height (not diagonally down to the far landing spot), since a diagonal sweep would cut straight
	// through the obstacle's own volume instead of passing over it.
	const FVector& NearCapsuleLocation = UALSMathLibrary::GetCapsuleLocationFromBase(
		NearTopLocation, 2.0f, OwnerCharacter->GetCapsuleComponent());
	const FVector OverheadSweepEnd(LandingCapsuleLocation.X, LandingCapsuleLocation.Y, NearCapsuleLocation.Z);
	// Half-height capsule: a vaulting body is leaned forward/compact over the obstacle, not standing
	// fully upright, so it doesn't need the full standing capsule's overhead clearance.
	const FCollisionShape OverheadCapsuleShape = FCollisionShape::MakeCapsule(
		OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius(),
		OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f);

	FHitResult OverheadHit;
	const bool bOverheadBlocked = World->SweepSingleByProfile(OverheadHit, NearCapsuleLocation, OverheadSweepEnd,
	                                                           FQuat::Identity, MantleObjectDetectionProfile,
	                                                           OverheadCapsuleShape, Params);
	if (ALSDebugComponent && ALSDebugComponent->GetShowTraces())
	{
		UALSDebugComponent::DrawDebugCapsuleTraceSingle(World, NearCapsuleLocation, OverheadSweepEnd, OverheadCapsuleShape,
		                                                DebugType, bOverheadBlocked, OverheadHit,
		                                                FLinearColor::Green, FLinearColor::Green, 1.0f);
	}

	if (bOverheadBlocked)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step5: overhead blocked by %s at %s"),
			*GetNameSafe(OverheadHit.GetComponent()), *OverheadHit.ImpactPoint.ToString());
		return false;
	}
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] Step5 OK: clearance confirmed, starting vault"));

	const float VaultHeight = (NearCapsuleLocation - OwnerCharacter->GetActorLocation()).Z;

	// Ledge transform doubles as the apex the position blend passes through (standing on top of the
	// ledge) - location is NearCapsuleLocation, rotation faces squarely into the obstacle.
	FALSComponentAndTransform LedgeWS;
	LedgeWS.Component = NearTopHit.GetComponent();
	LedgeWS.Transform = FTransform((InitialTraceNormal * FVector(-1.0f, -1.0f, 0.0f)).ToOrientationRotator(),
	                               NearCapsuleLocation, FVector::OneVector);

	FALSComponentAndTransform LandingWS;
	LandingWS.Component = LandingHit.GetComponent();
	LandingWS.Transform = FTransform(LedgeWS.Transform.GetRotation(), LandingCapsuleLocation, FVector::OneVector);

	const bool bStarted = VaultStart(VaultHeight, LedgeWS, LandingWS);
	if (!bStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultCheck: VaultStart failed to start, falling back to mantle"));
		return false;
	}
	Server_VaultStart(VaultHeight, LedgeWS, LandingWS);

	return true;
}

bool UALSMantleComponent::VaultStart(float VaultHeight, const FALSComponentAndTransform& VaultLedgeWS,
                                     const FALSComponentAndTransform& VaultLandingWS)
{
	if (OwnerCharacter == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultStart: no OwnerCharacter"));
		return false;
	}

	const FALSVaultAsset VaultAsset = GetVaultAsset(OwnerCharacter->GetOverlayState());
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultStart: GetVaultAsset OverlayState=%d AnimMontage=%s PlayRate=%.2f"),
		(int32)OwnerCharacter->GetOverlayState(), *GetNameSafe(VaultAsset.AnimMontage), VaultAsset.PlayRate);
	if (!VaultAsset.AnimMontage || !OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultStart: GetVaultAsset returned no AnimMontage, aborting vault"));
		return false;
	}

	if (OwnerCharacter->IsA(AALSCharacter::StaticClass()))
	{
		Cast<AALSCharacter>(OwnerCharacter)->ClearHeldObject();
	}

	// Disable ticking during vault, same as mantling
	SetComponentTickEnabledAsync(false);

	// ForwardMovementAction_Implementation only checks MovementState (Grounded/InAir/Crawl/Stationary),
	// not MovementAction, so without this, held movement input keeps calling AddMovementInput throughout
	// the vault and fights the explicit position blend below. Mantle avoids this the same way.
	OwnerCharacter->SetMovementState(EALSMovementState::Mantling);

	VaultParams.AnimMontage = VaultAsset.AnimMontage;
	VaultParams.PositionCorrectionCurve = VaultAsset.PositionCorrectionCurve;
	VaultParams.PlayRate = VaultAsset.PlayRate > 0.0f ? VaultAsset.PlayRate : 1.0f;
	if (OwnerCharacter->GetGait() == EALSGait::Sprinting && VaultAsset.SprintPlayRateMultiplier > 0.0f)
	{
		VaultParams.PlayRate *= VaultAsset.SprintPlayRateMultiplier;
	}
	VaultParams.StartingOffset = VaultAsset.StartingOffset;
	VaultParams.ApexAlpha = FMath::Clamp(VaultAsset.ApexAlpha, 0.01f, 0.99f);
	VaultParams.PelvisHeightAtHandContact = VaultAsset.PelvisHeightAtHandContact;

	VaultStartTransform = OwnerCharacter->GetActorTransform();
	VaultApexTransform = VaultLedgeWS.Transform;
	if (VaultParams.PositionCorrectionCurve)
	{
		// VaultLedgeWS (NearCapsuleLocation) assumes a full upright stand on the ledge - capsule base at
		// the obstacle's top surface, same assumption Mantle makes for MantleTarget. That's wrong for
		// vaulting: since the montage has no root motion, the mesh's own bone animation keeps rising on
		// its own (independent of the capsule) past the hand-contact frame - the capsule only needs to be
		// positioned so THAT frame's hand/pelvis height lands on the obstacle's raw surface. Recover the
		// raw surface height (undo GetCapsuleLocationFromBase's capsule-half-height + 2.0 offset) and solve
		// for the capsule height that puts PelvisHeightAtHandContact exactly there - NOT the full-stand
		// height, and NOT adding any further climb-to-apex amount (the animation already provides that on
		// its own, on top of wherever the capsule ends up).
		const float CapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float ObstacleSurfaceZ = VaultApexTransform.GetLocation().Z - CapsuleHalfHeight - 2.0f;
		FVector ApexLocation = VaultApexTransform.GetLocation();
		ApexLocation.Z = ObstacleSurfaceZ + CapsuleHalfHeight - VaultParams.PelvisHeightAtHandContact;
		VaultApexTransform.SetLocation(ApexLocation);
	}
	VaultTargetTransform = VaultLandingWS.Transform;

	// Same as Mantle's Step 3/4 (MantleStart): capture where the character actually is relative to the
	// ledge (VaultActualStartOffset), and where the authored animation assumes the hand-grab happens
	// relative to the ledge (VaultAnimatedStartOffset, from StartingOffset). VaultUpdate blends between
	// these per-axis instead of snapping, so the hands land on the ledge correctly regardless of exactly
	// where VaultCheck caught the character.
	VaultActualStartOffset = UALSMathLibrary::TransformSub(VaultStartTransform, VaultApexTransform);

	FVector RotatedVector = VaultApexTransform.GetRotation().Vector() * VaultParams.StartingOffset.Y;
	RotatedVector.Z = VaultParams.StartingOffset.Z;
	const FTransform VaultStartOffsetWS(VaultApexTransform.Rotator(), VaultApexTransform.GetLocation() - RotatedVector,
	                                    FVector::OneVector);
	VaultAnimatedStartOffset = UALSMathLibrary::TransformSub(VaultStartOffsetWS, VaultApexTransform);

	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultStart: VaultHeight=%.2f StartZ=%.2f ApexZ=%.2f TargetZ=%.2f PositionCorrectionCurve=%s"),
		VaultHeight, VaultStartTransform.GetLocation().Z, VaultApexTransform.GetLocation().Z, VaultTargetTransform.GetLocation().Z,
		*GetNameSafe(VaultParams.PositionCorrectionCurve));
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultStart: ActualStartOffset(rel to Apex)=%s AnimatedStartOffset(rel to Apex)=%s ApexAlpha=%.3f StartingOffset=%s"),
		*VaultActualStartOffset.GetLocation().ToString(), *VaultAnimatedStartOffset.GetLocation().ToString(),
		VaultParams.ApexAlpha, *VaultParams.StartingOffset.ToString());

	VaultPreVaultVelocity = OwnerCharacter->GetCharacterMovement()->Velocity;
	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_None);

	const float VaultDuration = VaultAsset.AnimMontage->GetPlayLength() / VaultParams.PlayRate;
	VaultTimeline->SetPlayRate(VaultDuration > 0.0f ? 1.0f / VaultDuration : 1.0f);
	VaultTimeline->PlayFromStart();

	if (OwnerCharacter->GetMesh()->GetAnimInstance())
	{
		OwnerCharacter->GetMesh()->GetAnimInstance()->Montage_Play(VaultAsset.AnimMontage, VaultParams.PlayRate);
	}

	return true;
}

void UALSMantleComponent::VaultUpdate(float Alpha)
{
	if (!OwnerCharacter)
	{
		return;
	}

	// Two-segment blend through the apex (standing on the ledge), not a straight line start->landing,
	// so the path actually goes up and over the obstacle instead of through it.
	FTransform LerpedTarget;
	float DebugPositionAlpha = 0.0f;
	float DebugXYCorrectionAlpha = 0.0f;
	float DebugZCorrectionAlpha = 0.0f;
	if (Alpha < VaultParams.ApexAlpha)
	{
		const float GrabAlpha = Alpha / VaultParams.ApexAlpha;
		if (VaultParams.PositionCorrectionCurve)
		{
			// Same per-axis correction as Mantle's MantleUpdate, just re-targeted at VaultApexTransform
			// instead of MantleTarget: GrabAlpha (0-1 across this first half) is mapped onto the curve's
			// own authored time range, so the curve can be keyed over any convenient domain (e.g. 0-1,
			// or the montage's own grab-phase seconds) rather than needing to match VaultTimeline's units.
			float MinTime = 0.0f;
			float MaxTime = 0.0f;
			VaultParams.PositionCorrectionCurve->GetTimeRange(MinTime, MaxTime);
			const FVector CurveVec = VaultParams.PositionCorrectionCurve->GetVectorValue(
				FMath::Lerp(MinTime, MaxTime, GrabAlpha));
			const float PositionAlpha = CurveVec.X;
			const float XYCorrectionAlpha = CurveVec.Y;
			const float ZCorrectionAlpha = CurveVec.Z;
			DebugPositionAlpha = PositionAlpha;
			DebugXYCorrectionAlpha = XYCorrectionAlpha;
			DebugZCorrectionAlpha = ZCorrectionAlpha;

			// Blend into the animated horizontal/rotation offset using the Y channel.
			const FTransform TargetHzTransform(VaultAnimatedStartOffset.GetRotation(),
			                                   {
				                                   VaultAnimatedStartOffset.GetLocation().X,
				                                   VaultAnimatedStartOffset.GetLocation().Y,
				                                   VaultActualStartOffset.GetLocation().Z
			                                   },
			                                   FVector::OneVector);
			const FTransform& HzLerpResult =
				UKismetMathLibrary::TLerp(VaultActualStartOffset, TargetHzTransform, XYCorrectionAlpha);

			// Blend into the animated vertical offset using the Z channel.
			const FTransform TargetVtTransform(VaultActualStartOffset.GetRotation(),
			                                   {
				                                   VaultActualStartOffset.GetLocation().X,
				                                   VaultActualStartOffset.GetLocation().Y,
				                                   VaultAnimatedStartOffset.GetLocation().Z
			                                   },
			                                   FVector::OneVector);
			const FTransform& VtLerpResult =
				UKismetMathLibrary::TLerp(VaultActualStartOffset, TargetVtTransform, ZCorrectionAlpha);

			const FTransform ResultTransform(HzLerpResult.GetRotation(),
			                                 {
				                                 HzLerpResult.GetLocation().X, HzLerpResult.GetLocation().Y,
				                                 VtLerpResult.GetLocation().Z
			                                 },
			                                 FVector::OneVector);

			// Blend from the still-offset transform fully onto the ledge using the X channel.
			LerpedTarget = UKismetMathLibrary::TLerp(
				UALSMathLibrary::TransformAdd(VaultApexTransform, ResultTransform), VaultApexTransform,
				PositionAlpha);
		}
		else
		{
			// No curve authored yet - plain Lerp (previous behavior).
			LerpedTarget = UKismetMathLibrary::TLerp(VaultStartTransform, VaultApexTransform, GrabAlpha);
		}
	}
	else
	{
		const float LandAlpha = (Alpha - VaultParams.ApexAlpha) / (1.0f - VaultParams.ApexAlpha);
		LerpedTarget = UKismetMathLibrary::TLerp(VaultApexTransform, VaultTargetTransform, LandAlpha);
	}
	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultUpdate: Alpha=%.3f Phase=%s PositionAlpha=%.3f XYCorrectionAlpha=%.3f ZCorrectionAlpha=%.3f LerpedLoc=%s"),
		Alpha, Alpha < VaultParams.ApexAlpha ? TEXT("Grab") : TEXT("Land"), DebugPositionAlpha, DebugXYCorrectionAlpha,
		DebugZCorrectionAlpha, *LerpedTarget.GetLocation().ToString());
	OwnerCharacter->SetActorLocationAndTargetRotation(LerpedTarget.GetLocation(), LerpedTarget.GetRotation().Rotator());
}

void UALSMantleComponent::VaultEnd()
{
	if (OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] VaultEnd: ActorLoc=%s Velocity=%s PendingInput=%s"),
			*OwnerCharacter->GetActorLocation().ToString(),
			*OwnerCharacter->GetCharacterMovement()->Velocity.ToString(),
			*OwnerCharacter->GetPendingMovementInputVector().ToString());

		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		// MOVE_None skipped PerformMovement entirely for the whole vault, so any input held during it
		// was never consumed by ConsumeMovementInputVector() and kept accumulating unbounded in
		// ControlInputVector. StopMovementImmediately only clears Velocity/Acceleration, not that - left
		// unflushed, it gets applied all at once the instant Walking resumes, causing a sudden snap.
		OwnerCharacter->ConsumeMovementInputVector();
		OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
		// StopMovementImmediately just zeroed Velocity - re-apply whatever speed/direction the character
		// had going into the vault (e.g. a sprint) so they carry it out instead of resuming from a dead
		// stop. CharacterMovementComponent will correct/override this on the next tick if held input
		// disagrees, same as it would after any other momentary external velocity set.
		OwnerCharacter->GetCharacterMovement()->Velocity = VaultPreVaultVelocity;
		OwnerCharacter->SetMovementState(EALSMovementState::Grounded);

		if (OwnerCharacter->IsA(AALSCharacter::StaticClass()))
		{
			Cast<AALSCharacter>(OwnerCharacter)->UpdateHeldObject();
		}
	}

	SetComponentTickEnabledAsync(true);
	PostVaultDebugFrames = 40;
}

void UALSMantleComponent::Server_VaultStart_Implementation(float VaultHeight, const FALSComponentAndTransform& VaultLedgeWS,
                                                           const FALSComponentAndTransform& VaultLandingWS)
{
	Multicast_VaultStart(VaultHeight, VaultLedgeWS, VaultLandingWS);
}

void UALSMantleComponent::Multicast_VaultStart_Implementation(float VaultHeight, const FALSComponentAndTransform& VaultLedgeWS,
                                                              const FALSComponentAndTransform& VaultLandingWS)
{
	if (OwnerCharacter && !OwnerCharacter->IsLocallyControlled())
	{
		VaultStart(VaultHeight, VaultLedgeWS, VaultLandingWS);
	}
}

void UALSMantleComponent::OnOwnerJumpInput()
{
	// Check if character is able to do one of the special mantling

	UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] OnOwnerJumpInput: OwnerCharacter=%s MovementAction=%d MovementState=%d HasMovementInput=%d"),
		*GetNameSafe(OwnerCharacter),
		OwnerCharacter ? (int32)OwnerCharacter->GetMovementAction() : -1,
		OwnerCharacter ? (int32)OwnerCharacter->GetMovementState() : -1,
		OwnerCharacter ? OwnerCharacter->HasMovementInput() : 0);

	if (OwnerCharacter && OwnerCharacter->GetMovementAction() == EALSMovementAction::None)
	{
		if (OwnerCharacter->GetMovementState() == EALSMovementState::Grounded)
		{
			if (OwnerCharacter->HasMovementInput())
			{
				const bool bVaulted = VaultCheck(VaultTraceSettings, EDrawDebugTrace::Type::ForDuration);
				UE_LOG(LogTemp, Warning, TEXT("[VaultDebug] OnOwnerJumpInput: VaultCheck returned %d"), bVaulted);
				if (!bVaulted)
				{
					MantleCheck(GroundedTraceSettings, EDrawDebugTrace::Type::ForDuration);
				}
			}
		}
		else if (OwnerCharacter->GetMovementState() == EALSMovementState::InAir)
		{
			// Same Vault-first, Mantle-fallback pattern as the Grounded branch above - without this,
			// falling+jump always mantled even when there was room to vault.
			const bool bVaulted = VaultCheck(VaultTraceSettings, EDrawDebugTrace::Type::ForDuration);
			if (!bVaulted)
			{
				MantleCheck(FallingTraceSettings, EDrawDebugTrace::Type::ForDuration);
			}
		}
	}
}

void UALSMantleComponent::OnOwnerRagdollStateChanged(bool bRagdollState)
{
	// If owner is going into ragdoll state, stop mantling immediately
	if (bRagdollState)
	{
		MantleTimeline->Stop();
	}
}
