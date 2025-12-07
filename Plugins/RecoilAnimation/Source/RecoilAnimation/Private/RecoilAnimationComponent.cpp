// Designed by Hitman's Store, 2023

#include "RecoilAnimationComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Curves/CurveVector.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "HelperWidget.h"
#include "StoredData.h"
#include "Math/UnitConversion.h"

/** Pseudonym for AnimData.StoredData */
#define InterpData AnimData.StoredData

DEFINE_LOG_CATEGORY(LogRecoilAnimation);

using UKismetMath = UKismetMathLibrary;

URecoilAnimationComponent::URecoilAnimationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void URecoilAnimationComponent::BeginPlay()
{
	Super::BeginPlay();
	
	SetupComponent();
	SetupStateMachine();
	SetComponentTickEnabled(false);
}

void URecoilAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!InterpData)
	{
		return;
	}

	TickAdvancedSolver(DeltaTime);

	if(IsComponentTickEnabled() && !AnimTimeline.IsPlaying())
	{
		// OutValue = LerpedValue + AdditiveLayerValue
		if(LerpedRot.Equals(OutRot, 0.001) && LerpedLoc.Equals(OutLoc, 0.001))
		{
			SetComponentTickEnabled(false);
		}
	}
}

void URecoilAnimationComponent::TickAdvancedSolver(float DeltaTime)
{
	if(!(TempRot && TempLoc))
	{
		return;
	}

	AnimTimeline.TickTimeline(DeltaTime);
	FTransform Finalized = FTransform(LerpedRot, LerpedLoc);
		
	ApplyNoise(Finalized, DeltaTime);
	ApplyPushback(Finalized, DeltaTime);
	ApplyProgression(Finalized, DeltaTime);

	FVector PivotOffset = Finalized.Rotator().RotateVector(InterpData->PivotOffset) - InterpData->PivotOffset;
	Finalized.AddToTranslation(PivotOffset);

	OutLoc = Finalized.GetLocation();
	OutRot = Finalized.Rotator();

	OutLoc *= GlobalTranslationMultiplier;
	OutRot.Pitch *= GlobalRotationMultiplier.Y;
	OutRot.Yaw *= GlobalRotationMultiplier.Z;
	OutRot.Roll *= GlobalRotationMultiplier.X;
}

void URecoilAnimationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(URecoilAnimationComponent, OutRot, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(URecoilAnimationComponent, OutLoc, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(URecoilAnimationComponent, InputScale, COND_SkipOwner);
}

void URecoilAnimationComponent::ApplySmoothing(float DeltaTime)
{
	if(bEnableSmoothing)
	{
		auto ApplyInterp = [&](float Raw, float Out, float Smooth) -> float
		{
			if(FMath::IsNearlyZero(Smooth, 0.01f))
			{
				return Raw;
			}

			Raw = AnimTimeline.IsPlaying() ? Raw : 0.f;
			return GLerp(Out, Raw, Smooth, DeltaTime);
		};

		LerpedRot.Pitch = ApplyInterp(RawOutRot.Pitch, LerpedRot.Pitch, InterpData->SmoothRot.Pitch);
		LerpedRot.Yaw = ApplyInterp(RawOutRot.Yaw, LerpedRot.Yaw, InterpData->SmoothRot.Yaw);
		LerpedRot.Roll = ApplyInterp(RawOutRot.Roll, LerpedRot.Roll, InterpData->SmoothRot.Roll);

		LerpedLoc.X = ApplyInterp(RawOutLoc.X, LerpedLoc.X, InterpData->SmoothLoc.X);
		LerpedLoc.Y = ApplyInterp(RawOutLoc.Y, LerpedLoc.Y, InterpData->SmoothLoc.Y);
		LerpedLoc.Z = ApplyInterp(RawOutLoc.Z, LerpedLoc.Z, InterpData->SmoothLoc.Z);
	}
	else
	{
		LerpedRot = RawOutRot;
		LerpedLoc = RawOutLoc;
	}
}

void URecoilAnimationComponent::ApplyNoise(FTransform& Finalized, float DeltaTime)
{
	Noise2DTarget.X = GLerp(Noise2DTarget.X, 0.f, InterpData->Noise2DZero.X, DeltaTime);
	Noise2DTarget.Y = GLerp(Noise2DTarget.Y, 0.f, InterpData->Noise2DZero.Y, DeltaTime);
	
	Noise2DCurrent.X = GLerp(Noise2DCurrent.X, Noise2DTarget.X, NoiseSpeed.X, DeltaTime);
	Noise2DCurrent.Y = GLerp(Noise2DCurrent.Y, Noise2DTarget.Y, NoiseSpeed.Y, DeltaTime);

	const float NoiseScalar = bIsAiming ? InterpData->NoiseAimScalar : 1.f;

	Noise2DCurrent.X = Noise2DCurrent.X * NoiseScalar;
	Noise2DCurrent.Y = Noise2DCurrent.Y * NoiseScalar;
	
	const FVector Temp = Finalized.GetLocation() + FVector(0.0, Noise2DCurrent.X, Noise2DCurrent.Y);
	
	Finalized.SetLocation(Temp);
}

void URecoilAnimationComponent::ApplyPushback(FTransform& Finalized, float DeltaTime)
{
	PushbackTarget = GLerp(PushbackTarget, 0.0, InterpData->PushbackZero, DeltaTime);
	PushbackCurrent = GLerp(PushbackCurrent, PushbackTarget, PushbackSpeed, DeltaTime);
	
	const FVector Temp = Finalized.GetLocation() + FVector(PushbackCurrent, 0.0, 0.0);
	Finalized.SetLocation(Temp);
}

void URecoilAnimationComponent::ApplyProgression(FTransform& Finalized, float DeltaTime)
{
	PitchProgress.X = GLerp(PitchProgress.X, PitchProgress.Y, PitchProgressSpeed, DeltaTime);
	PitchProgress.Y = GLerp(PitchProgress.Y, 0.f, InterpData->PitchProgress.Damping, DeltaTime);
	
	ZProgress.X = GLerp(ZProgress.X, ZProgress.Y, ZProgressSpeed, DeltaTime);
	ZProgress.Y = GLerp(ZProgress.Y, 0.f, InterpData->ZProgress.Damping, DeltaTime);

	const FVector Loc = Finalized.GetLocation() + FVector(0.0, 0.0, ZProgress.X);
	const FQuat Rot = Finalized.GetRotation() * FRotator(PitchProgress.X, 0.0, 0.0).Quaternion();

	Finalized.SetLocation(Loc);
	Finalized.SetRotation(Rot);
}

void URecoilAnimationComponent::SetupComponent()
{
	SetIsReplicated(true);
	ProgressFunction.BindUFunction(this, TEXT("AnimationUpdate"));

	FOnTimelineEvent Event;
	Event.BindUFunction(this, TEXT("CalculateTargetData"));
	AnimTimeline.AddEvent(0.f, Event);

	InputScale.Reset();

	if(GIsEditor)
	{
		if (!WidgetClass)
		{
			const FString TheClassPath = "Class'/RecoilAnimation/UI/PRAS_UI_BP.PRAS_UI_BP_C'";
			const TCHAR* ClassName = *TheClassPath;
			UClass* Template = LoadObject<UClass>(nullptr, ClassName);
			Widget = CreateWidget<UHelperWidget>(GetWorld(), Template);
		}
		else
		{
			Widget = CreateWidget<UHelperWidget>(GetWorld(), WidgetClass);
		}

		Widget->SetComponent(this);
	}
}

void URecoilAnimationComponent::AddEvent(float Time, FOnTimelineEvent Event)
{
	if(Event.IsBound())
	{
		Time = FMath::Max(0.f, Time);
		AnimTimeline.AddEvent(Time, Event);
	}
	else
	{
		UE_LOG(LogRecoilAnimation, Warning, TEXT("AddEvent: not bound!"))
	}
}

float URecoilAnimationComponent::DampSpring(float Current, FFloatSpringState& SpringState, float Stiffness,
                                            float Damping, float DeltaTime)
{
	DeltaTime = FMath::Min(DeltaTime, 1.f);
	return UKismetMath::FloatSpringInterp(Current, 0.f, SpringState, Stiffness, Damping, DeltaTime);
}

void URecoilAnimationComponent::Init(const FRecoilAnimData Data, const float Rate, const int Bursts)
{
	/** Copy input Animation Data struct */
	AnimData = Data;
	InterpData = AnimData.StoredData;
	
	if (!InterpData && GIsEditor)
	{
		InterpData = UStoredData::FindDataAsset(AnimData.SlotName, GetWorld());
		UE_LOG(LogRecoilAnimation, Warning, TEXT("Loading asset %s"), *AnimData.SlotName)
	}

	BurstAmount = BurstCounter = Bursts;
	FireRate = FMath::Max(Rate, 0.001f);
	
	if (!(AnimData.SingleRot && AnimData.SingleLoc))
	{
		/** not a valid state */
		UE_LOG(LogRecoilAnimation, Warning, TEXT("One of single curves is (or both of them are) NULL"));
		return;
	}

	TargetRot = FRotator::ZeroRotator;
	TargetLoc = FVector::ZeroVector;
	Noise2DTarget = FVector2D::ZeroVector;
	PitchProgress.Y = ZProgress.Y = PushbackTarget = 0.0;

	NoiseSpeed.X = GetNonZero(NoiseSpeed.X, InterpData->Noise2DAccel.X);
	NoiseSpeed.Y = GetNonZero(NoiseSpeed.Y, InterpData->Noise2DAccel.Y);

	PitchProgressSpeed = GetNonZero(PitchProgressSpeed, InterpData->PitchProgress.Accel);
	ZProgressSpeed = GetNonZero(ZProgressSpeed, InterpData->ZProgress.Accel);
	PushbackSpeed =  GetNonZero(PushbackSpeed, InterpData->PushbackAccel);

	if (bTimelineInitialized)
	{
		AnimTimeline.SetVectorCurve(AnimData.SingleRot, TEXT("Rotation"));
		AnimTimeline.SetVectorCurve(AnimData.SingleLoc, TEXT("Location"));
	}
	else
	{
		AnimTimeline.AddInterpVector(AnimData.SingleRot, ProgressFunction, NAME_None, TEXT("Rotation"));
		AnimTimeline.AddInterpVector(AnimData.SingleLoc, ProgressFunction, NAME_None, TEXT("Location"));
		bTimelineInitialized = true;
	}

	if(GIsEditor && Widget)
	{
		Widget->UpdateValues();
	}
}

void URecoilAnimationComponent::Play()
{
	if(!InterpData)
	{
		return;
	}

	if(!IsComponentTickEnabled())
	{
		SetComponentTickEnabled(true);
	}
	
	AnimTimeline.SetPlayRate(InterpData->PlayRate);
	bAnimActive = true;
	
	if (AnimData.SolverType == ESolverType_PRAS::Advanced)
	{
		for (uint16 i = 0; i < StateMachine.Num(); i++)
		{
			FCheckCondition Condition = StateMachine[i].TransitionCondition;
			if (Condition.Execute())
			{
				CurrentStateIndex = i;
				break;
			}
		}

		if (!StateMachine[CurrentStateIndex].OnPlay.ExecuteIfBound())
		{
			UE_LOG(LogRecoilAnimation, Warning, TEXT("Start transition %2.i is not bound"), CurrentStateIndex);
		}
		
		if(TempLoc && TempRot)
		{
			SetSimulationTimer(AnimTimeline.GetTimelineLength());
		}
	}
	else
	{
		SetSimulationTimer(60.f / FireRate + 1.f);
	}
	
	LastShotTime = UKismetSystemLibrary::GetGameTimeInSeconds(GetWorld());
}

void URecoilAnimationComponent::Stop()
{
	AnimTimeline.SetLooping(false);
	
	if (!StateMachine[CurrentStateIndex].OnStop.ExecuteIfBound())
	{
		UE_LOG(LogRecoilAnimation, Warning, TEXT("Stop transition %2i is not bound"), CurrentStateIndex)
	}

	bAnimActive = false;
}

void URecoilAnimationComponent::Simulate(const bool bStartSim)
{
	bSim = bStartSim;

	if (!AnimData.SingleRot && !AnimData.SingleLoc)
	{
		return;
	}

	if (bSim)
	{
		Play();
		UE_LOG(LogRecoilAnimation, Warning, TEXT("Simulation started"));
	}
	else
	{
		Stop();
		UE_LOG(LogRecoilAnimation, Warning, TEXT("Simulation ended"));
	}
}

void URecoilAnimationComponent::ShowUI()
{
	const APawn* Owner = Cast<APawn>(GetOwner());

	if (!Owner)
	{
		UE_LOG(LogRecoilAnimation, Error, TEXT("Can't find PawnOwner Reference"));
		return;
	}

	APlayerController* PC_Ptr = Cast<APlayerController>(Owner->GetController());

	if (!(PC_Ptr && Widget))
	{
		UE_LOG(LogRecoilAnimation, Error, TEXT("Player Controller is NULL"));
		return;
	}

	PC_Ptr->bShowMouseCursor = true;
	FInputModeGameAndUI InputModeData;

	InputModeData.SetWidgetToFocus(Widget->TakeWidget());
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC_Ptr->SetInputMode(InputModeData);

	Widget->AddToViewport();
}

void URecoilAnimationComponent::HideUI()
{
	const APawn* Owner = Cast<APawn>(GetOwner());

	if (!Owner)
	{
		UE_LOG(LogRecoilAnimation, Error, TEXT("Can't find PawnOwner Reference"));
		return;
	}

	APlayerController* PC_Ptr = Cast<APlayerController>(Owner->GetController());

	if (!(PC_Ptr && Widget))
	{
		UE_LOG(LogRecoilAnimation, Error, TEXT("Player Controller is NULL"));
		return;
	}

	PC_Ptr->bShowMouseCursor = false;

	FInputModeGameOnly InputModeData;
	InputModeData.SetConsumeCaptureMouseDown(true);

	PC_Ptr->SetInputMode(InputModeData);
	Widget->RemoveFromParent();
}

void URecoilAnimationComponent::SaveData()
{
	if(GIsEditor)
	{
		InterpData->SaveData(nullptr, AnimData.SlotName);
	}
}

void URecoilAnimationComponent::ScaleInput(FInputScale_PRAS NewScale)
{
	InputScale *= NewScale;
}

void URecoilAnimationComponent::UnscaleInput(FInputScale_PRAS NewScale)
{
	InputScale /= NewScale;
}

void URecoilAnimationComponent::ResetInputScale()
{
	InputScale.Reset();
}

void URecoilAnimationComponent::SetFireMode(const EFireMode_PRAS NewMode)
{
	FireMode = NewMode;
}

void URecoilAnimationComponent::SetAimingStatus(bool bStatus)
{
	const bool bUpdateWidget = bIsAiming != bStatus;
	bIsAiming = bStatus;

	if(bUpdateWidget)
	{
		if(Widget)
		{
			Widget->UpdateValues();
		}
	}
}

double URecoilAnimationComponent::GetAngularRecoil(const FVector4 RecoilProfile)
{
	const bool RandomBool = UKismetMath::RandomBool();

	const double Max = FMath::FRandRange(RecoilProfile.Z, RecoilProfile.W);
	const double Min = FMath::FRandRange(RecoilProfile.X, RecoilProfile.Y);

	return RandomBool ? Max : Min;
}

void URecoilAnimationComponent::CorrectAlpha(const UCurveVector* Rotation, const UCurveVector* Location,
                                             const float TimeCorrection)
{
	/** Alpha (value of a semi curve) at the corrected time */
	const FVector RotationAlpha = Rotation->GetVectorValue(TimeCorrection);
	
	StartValRot.Pitch = FMath::Lerp(StartValRot.Pitch, TargetRot.Pitch, RotationAlpha.Y);
	StartValRot.Yaw = FMath::Lerp(StartValRot.Yaw, TargetRot.Yaw, RotationAlpha.Z);
	StartValRot.Roll = FMath::Lerp(StartValRot.Roll, TargetRot.Roll, RotationAlpha.X);
	
	StartValLoc = FMath::Lerp(StartValLoc, TargetLoc, Location->GetVectorValue(TimeCorrection));
}

void URecoilAnimationComponent::SetupTransition(FRotator StartRot, FVector StartLoc, UCurveVector* Rot,
                                                UCurveVector* Loc)
{
	if(!(Rot && Loc))
	{
		UE_LOG(LogRecoilAnimation, Error, TEXT("Rot or Loc curve is nullptr"))
		return;
	}
	
	StartValRot = StartRot;
	StartValLoc = StartLoc;
	
	CanRestRot = CanRestLoc = FStartRest(true, true, true);

	TempRot = Rot;
	TempLoc = Loc;

	AnimTimeline.SetVectorCurve(TempRot, TEXT("Rotation"));
	AnimTimeline.SetVectorCurve(TempLoc, TEXT("Location"));

	AnimTimeline.PlayFromStart();
}

float URecoilAnimationComponent::GetDelta() const
{
	return UKismetSystemLibrary::GetGameTimeInSeconds(GetWorld()) - LastShotTime;
}

void URecoilAnimationComponent::AnimationUpdate(FVector VectorAlpha)
{
	/** Current playback position */
	const float Playback = AnimTimeline.GetPlaybackPosition();
	float LastPlayback = Playback - GetWorld()->GetDeltaSeconds() * AnimTimeline.GetPlayRate();
	LastPlayback = FMath::Max(LastPlayback, 0.f);

	FVector Alpha = TempRot->GetVectorValue(Playback);
	FVector LastAlpha = TempRot->GetVectorValue(LastPlayback);

	FRotator ExtraRot = bEnableSmoothing ? InterpData->MultiRot : FRotator(1.f,1.f, 1.f);
	FVector ExtraLoc = bEnableSmoothing ? InterpData->MultiLoc : FVector(1.f,1.f, 1.f);
	
	RawOutRot.Roll = !MuteRot.X ? FMath::Lerp(CorrectStart(LastAlpha.X, Alpha.X, CanRestRot.X, StartValRot.Roll),
	                          TargetRot.Roll * ExtraRot.Roll, Alpha.X) : 0.f;

	RawOutRot.Pitch = !MuteRot.Y ? FMath::Lerp(CorrectStart(LastAlpha.Y, Alpha.Y, CanRestRot.Y, StartValRot.Pitch),
	                           TargetRot.Pitch * ExtraRot.Pitch, Alpha.Y) : 0.f;

	RawOutRot.Yaw = !MuteRot.Z ? FMath::Lerp(CorrectStart(LastAlpha.Z, Alpha.Z, CanRestRot.Z, StartValRot.Yaw),
	                         TargetRot.Yaw * ExtraRot.Yaw, Alpha.Z) : 0.f;

	Alpha = TempLoc->GetVectorValue(Playback);
	LastAlpha = TempLoc->GetVectorValue(LastPlayback);

	RawOutLoc.X = !MuteLoc.X ? FMath::Lerp(CorrectStart(LastAlpha.X, Alpha.X, CanRestLoc.X, StartValLoc.X),
	                       TargetLoc.X * ExtraLoc.X, Alpha.X) : 0.f;

	RawOutLoc.Y = !MuteLoc.Y ? FMath::Lerp(CorrectStart(LastAlpha.Y, Alpha.Y, CanRestLoc.Y, StartValLoc.Y),
	                       TargetLoc.Y * ExtraLoc.Y, Alpha.Y) : 0.f;

	RawOutLoc.Z = !MuteLoc.Z ? FMath::Lerp(CorrectStart(LastAlpha.Z, Alpha.Z, CanRestLoc.Z, StartValLoc.Z),
	                       TargetLoc.Z * ExtraLoc.Z, Alpha.Z) : 0.f;

	const float DeltaSeconds = GetWorld()->GetDeltaSeconds();
	ApplySmoothing(DeltaSeconds);
}

void URecoilAnimationComponent::CalculateTargetData()
{
	auto RandomRot = [&](FVector4 Roll, FVector2D Pitch, FVector4 Yaw) -> void
	{
		Roll *= InputScale.Roll;
		Pitch *= InputScale.Pitch;
		Yaw *= InputScale.Yaw;
		
		const float Rand = GetAngularRecoil(Roll);
		const bool bSameSide = TargetRot.Roll * Rand > 0.f;

		TargetRot.Roll = InterpData->bSmoothRoll && bSameSide ? Rand * -1.f : Rand;
		TargetRot.Pitch = FMath::FRandRange(Pitch.X, Pitch.Y);
		TargetRot.Yaw = GetAngularRecoil(Yaw);
	};

	auto RandomLoc = [&](FVector2D Kick, FVector2D KickR, FVector2D KickUp) -> void
	{
		Kick *= InputScale.Kick;
		KickR *= InputScale.KickR;
		KickUp *= InputScale.KickUp;
		
		TargetLoc.X = FMath::FRandRange(Kick.X, Kick.Y);
		TargetLoc.Y = FMath::FRandRange(KickR.X, KickR.Y);
		TargetLoc.Z = FMath::FRandRange(KickUp.X, KickUp.Y);
	};

	auto RandomNoise = [this]() -> void
	{
		const auto Noise2DScaled = FVector2D(InterpData->Noise2DTarget.X * InputScale.Noise.X,
			InterpData->Noise2DTarget.Y * InputScale.Noise.Y);
		
		Noise2DTarget.X += FMath::FRandRange(-Noise2DScaled.X, Noise2DScaled.X);
		Noise2DTarget.Y += FMath::FRandRange(-Noise2DScaled.Y, Noise2DScaled.Y);
	};

	auto TargetProgress = [this]() -> void
	{
		const float Aim = bIsAiming ? InterpData->ProgressAimAlpha : 1.f;
		
		PitchProgress.Y += InterpData->PitchProgress.Amount * InputScale.Progression.X * Aim;
		ZProgress.Y += InterpData->ZProgress.Amount * InputScale.Progression.Y * Aim;
	};
	
	if (bIsAiming)
	{
		RandomRot(InterpData->RollAim, InterpData->PitchAim, InterpData->YawAim);
		RandomLoc(InterpData->KickAim, InterpData->KickAimR, InterpData->KickAimUp);
	}
	else
	{
		RandomRot(InterpData->RollHip, InterpData->PitchHip, InterpData->YawHip);
		RandomLoc(InterpData->KickHip, InterpData->KickHipR, InterpData->KickHipUp);
	}

	RandomNoise();
	TargetProgress();
}

double URecoilAnimationComponent::CorrectStart(double& Last, const double Current, bool& bStartRest, double& StartVal)
{
	if (FMath::Abs(Last) > FMath::Abs(Current) && bStartRest && !AnimTimeline.IsLooping())
	{
		StartVal = 0.0;
		bStartRest = false;
	}
	
	Last = Current;
	
	return StartVal;
}

FVector URecoilAnimationComponent::GLerp(FVector A, FVector B, FVector Alpha, float DeltaTime)
{
	FVector Out;

	Out.X = GLerp(A.X, B.X, Alpha.X, DeltaTime);
	Out.Y = GLerp(A.Y, B.Y, Alpha.Y, DeltaTime);
	Out.Z = GLerp(A.Z, B.Z, Alpha.Z, DeltaTime);

	return Out;
}

FRotator URecoilAnimationComponent::GLerp(FRotator A, FRotator B, FVector Alpha, float DeltaTime)
{
	FRotator Out;
	
	Out.Roll = GLerp(A.Roll, B.Roll, Alpha.X, DeltaTime);
	Out.Pitch = GLerp(A.Pitch, B.Pitch, Alpha.Y, DeltaTime);
	Out.Yaw = GLerp(A.Yaw, B.Yaw, Alpha.Z, DeltaTime);

	return Out;
}

void URecoilAnimationComponent::SetupStateMachine()
{
	/** Setup state machine here */

	FAnimState_PRAS SemiState;
	FAnimState_PRAS AutoState;

	SemiState.TransitionCondition.BindLambda([this]() -> bool
	{
		float TimerError = (60.f / FireRate) / GetWorld()->GetDeltaSeconds() + 1;
		TimerError *= GetWorld()->GetDeltaSeconds();

		/** First shot - disable smoothing */
		if(bEnableSmoothing)
		{
			bEnableSmoothing = false;
		}
		
		return FireMode == Semi || (GetDelta() > TimerError + 0.01f && !AnimTimeline.IsLooping());
	});

	SemiState.OnPlay.BindLambda([this]() -> void
	{
		SetupTransition(LerpedRot, LerpedLoc, AnimData.SingleRot, AnimData.SingleLoc);
	});

	SemiState.OnStop.BindLambda([this]() -> void
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	});

	AutoState.TransitionCondition.BindLambda([this]()-> bool
	{
		return true;
	});

	AutoState.OnPlay.BindLambda([this]() -> void
	{
		// Simplified advanced solver
		const bool bCurvesValid = !(AnimData.AutoRot && AnimData.AutoLoc && AnimData.RestRot && AnimData.RestLoc);
		bEnableSmoothing = bCurvesValid;

		if(bCurvesValid && !AnimTimeline.IsLooping())
		{
			const float Correction = 60.f / FireRate + InterpData->PlaybackOffset;
			AnimTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
			AnimTimeline.SetTimelineLength(60.f / FireRate);
			
			if(AnimData.AutoRot && AnimData.AutoLoc)
			{
				CorrectAlpha(AnimData.AutoRot, AnimData.AutoLoc, Correction);
				SetupTransition(StartValRot, StartValLoc, AnimData.AutoRot, AnimData.AutoLoc);
			}
			else
			{
				CorrectAlpha(AnimData.SingleRot, AnimData.SingleLoc, Correction);
				SetupTransition(StartValRot, StartValLoc, AnimData.SingleRot, AnimData.SingleLoc);
			}
			
			AnimTimeline.SetLooping(true);
			
			PushbackTarget = InterpData->Pushback;
			return;
		}
		
		if (AnimTimeline.IsLooping())
		{
			return;
		}

		// Legacy advanced solver

		/** Timer is frame rate dependent, so it can't be always accurate. Big deltas are taken into account */
		
		if (AnimTimeline.GetPlaybackPosition() > 60.f / FireRate + 0.015f)
		{
			CorrectAlpha(AnimData.AutoRot, AnimData.AutoLoc, 60.f / FireRate);

			UE_LOG(LogRecoilAnimation, Warning, TEXT("Timer delay is too long, applying correction %2.5f -> %2.5f"),
			       AnimTimeline.GetPlaybackPosition(), 60.f / FireRate);
		}
		else
		{
			StartValRot = LerpedRot;
			StartValLoc = LerpedLoc;
		}
		
		SetupTransition(StartValRot, StartValLoc, AnimData.AutoRot, AnimData.AutoLoc);
		AnimTimeline.SetLooping(true);
	});

	AutoState.OnStop.BindLambda([this]() -> void
	{
		if(!bAnimActive)
		{
			/** OnStop event has already been called -> not a valid state */
			return;
		}
		
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
		
		if(bEnableSmoothing)
		{
			AnimTimeline.SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
			return;
		}

		TargetRot = OutRot;
		TargetLoc = OutLoc;
		
		SetupTransition(FRotator::ZeroRotator, FVector::ZeroVector, AnimData.RestRot, AnimData.RestLoc);
	});
	
	/** Add all states to the state machine */
	StateMachine.Add(SemiState);
	StateMachine.Add(AutoState);
}

void URecoilAnimationComponent::SetSimulationTimer(float SemiDelay)
{
	if (!bSim)
	{
		/** not a valid state */
		return;
	}

	const float Delay = FireMode == Semi || (FireMode == Burst && BurstCounter == 1) ? SemiDelay : 60.f / FireRate;

	BurstCounter = BurstCounter > 1 && FireMode == Burst ? --BurstCounter : BurstAmount;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &URecoilAnimationComponent::Play, Delay);
}