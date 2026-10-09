// Optional sight diagnostics (ADS camera vs calibrated sight), off by default.
// Enable with console variable adb.Sight.DiagnosticsFile 1, output goes to Saved/Logs/SightDiagnostics.csv.
// Column values are milliradians, pitch up and yaw right positive.

#include "BaseWeapon.h"
#include "EBBarrel.h"
#include "Character/ALSCharacter.h"
#include "Character/ALSPlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static TAutoConsoleVariable<int32> CVarSightDiagnosticsFile(
	TEXT("adb.Sight.DiagnosticsFile"),
	0,
	TEXT("1 - while the local player aims down sights, write camera / sight alignment rows to Saved/Logs/SightDiagnostics.csv"),
	ECVF_Default);

namespace
{
	constexpr float SightDiagnosticsInterval = 0.05f;

	// Pitch (up positive) and yaw (right positive) of Direction seen from Frame, in milliradians
	FVector2D PitchYawMrad(const FRotator& Frame, const FVector& Direction)
	{
		const FVector Local = Frame.UnrotateVector(Direction);
		return FVector2D(FMath::Atan2(Local.Z, Local.X), FMath::Atan2(Local.Y, Local.X)) * 1000.0f;
	}

	FString SightDiagnosticsFilePath()
	{
		return FPaths::ProjectLogDir() / TEXT("SightDiagnostics.csv");
	}
}

bool ABaseWeapon::IsSightDiagnosticsEnabled()
{
	return CVarSightDiagnosticsFile.GetValueOnGameThread() > 0;
}

void ABaseWeapon::WriteSightDiagnostics(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
	if (CVarSightDiagnosticsFile.GetValueOnGameThread() <= 0 || World != GetWorld())
	{
		return;
	}

	AALSCharacter* OwnerCharacter = Cast<AALSCharacter>(GetOwner());
	APlayerController* PlayerController = OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
	if (!PlayerController || !PlayerController->IsLocalController() || OwnerCharacter->ActiveWeaponActor != this || !OwnerCharacter->GetUsingADS())
	{
		return;
	}

	const AALSPlayerCameraManager* CameraManager = Cast<AALSPlayerCameraManager>(PlayerController->PlayerCameraManager);
	ABaseSight* Sight = SightComponent ? SightComponent->GetSightActor() : nullptr;
	const USceneComponent* SightRoot = Sight ? Sight->GetRootComponent() : nullptr;
	if (!CameraManager || !SightRoot)
	{
		return;
	}

	SightDiagnosticsTimer += DeltaSeconds;
	if (SightDiagnosticsTimer < SightDiagnosticsInterval)
	{
		return;
	}
	SightDiagnosticsTimer = 0.0f;

	// Attachment of the sight parts, once per sight
	if (SightDiagnosticsLoggedSight != Sight)
	{
		SightDiagnosticsLoggedSight = Sight;
		UE_LOG(LogTemp, Warning, TEXT("SIGHT DIAGNOSTICS %s: Root parent=%s | ADS parent=%s rel=%s | SightView parent=%s rel=%s | SightMesh parent=%s scale=%s | ReticleDistance=%.1f"),
			*Sight->GetClass()->GetName(),
			*GetNameSafe(SightRoot->GetAttachParent()),
			*GetNameSafe(Sight->ADS ? Sight->ADS->GetAttachParent() : nullptr),
			Sight->ADS ? *Sight->ADS->GetRelativeLocation().ToString() : TEXT("-"),
			*GetNameSafe(Sight->SightView ? Sight->SightView->GetAttachParent() : nullptr),
			Sight->SightView ? *Sight->SightView->GetRelativeLocation().ToString() : TEXT("-"),
			*GetNameSafe(Sight->SightMesh ? Sight->SightMesh->GetAttachParent() : nullptr),
			Sight->SightMesh ? *Sight->SightMesh->GetRelativeScale3D().ToString() : TEXT("-"),
			Sight->ReticleDistance);
	}

	const FVector CameraLocation = CameraManager->GetCameraLocation();
	const FRotator CameraRotation = CameraManager->GetCameraRotation();
	const FRotator ControlRotation = PlayerController->GetControlRotation();

	// Rotation calibration applied to the sight in the current weapon pose
	FQuat CalibrationRotation = FQuat::Identity;
	if (BaseRotationSight == Sight)
	{
		const USceneComponent* Parent = SightRoot->GetAttachParent();
		const FQuat ParentRotation = Parent ? Parent->GetSocketQuaternion(SightRoot->GetAttachSocketName()) : FQuat::Identity;
		CalibrationRotation = SightRoot->GetComponentQuat() * (ParentRotation * BaseSightRelativeRotation.Quaternion()).Inverse();
	}

	const FVector SightDirection = Sight->GetSightLineDirection();
	const FVector UncalibratedDirection = CalibrationRotation.UnrotateVector(SightDirection);

	const FVector2D SightVsCamera = PitchYawMrad(CameraRotation, SightDirection);
	const FVector2D UncalibratedVsCamera = PitchYawMrad(CameraRotation, UncalibratedDirection);
	const FVector2D SightVsControl = PitchYawMrad(ControlRotation, SightDirection);
	const FVector2D UncalibratedVsControl = PitchYawMrad(ControlRotation, UncalibratedDirection);
	const FVector2D CameraVsControl = PitchYawMrad(ControlRotation, CameraRotation.Vector());

	const float WeightADS = CameraManager->GetCameraBehaviorParam(TEXT("Weight_ADS"));
	const float WeightFirstPerson = CameraManager->GetCameraBehaviorParam(TEXT("Weight_FirstPerson"));
	const float RotationLagSpeed = CameraManager->GetCameraBehaviorParam(TEXT("RotationLagSpeed"));

	static bool bHeaderWritten = false;
	FString Output;
	if (!bHeaderWritten)
	{
		bHeaderWritten = true;
		Output = TEXT("Time,PIE,Weapon,Sight,ZeroM,ViewMode,RotMode,SecSinceShot,")
			TEXT("CamPitchDeg,CamYawDeg,CamRollDeg,CtrlPitchDeg,CtrlYawDeg,FOV,")
			TEXT("SightVsCam_P,SightVsCam_Y,UncalVsCam_P,UncalVsCam_Y,")
			TEXT("SightVsCtrl_P,SightVsCtrl_Y,UncalVsCtrl_P,UncalVsCtrl_Y,")
			TEXT("CamVsCtrl_P,CamVsCtrl_Y,CalibAngleMrad,")
			TEXT("CamToADS_cm,W_ADS,W_FP,LagSpeed,DeltaTime,SmoothAlpha\n");
		// Start a fresh file for this editor session
		FFileHelper::SaveStringToFile(TEXT(""), *SightDiagnosticsFilePath());
	}

	Output += FString::Printf(
		TEXT("%.3f,%d,%s,%s,%.0f,%d,%d,%.3f,")
		TEXT("%.4f,%.4f,%.4f,%.4f,%.4f,%.2f,")
		TEXT("%.3f,%.3f,%.3f,%.3f,")
		TEXT("%.3f,%.3f,%.3f,%.3f,")
		TEXT("%.3f,%.3f,%.3f,")
		TEXT("%.3f,%.3f,%.3f,%.2f,%.4f,%.3f\n"),
		World->GetTimeSeconds(), World->GetOutermost()->GetPIEInstanceID(), *GetClass()->GetName(), *Sight->GetClass()->GetName(), CalibratedDistance,
		(int32)OwnerCharacter->GetViewMode(), (int32)OwnerCharacter->GetRotationMode(), World->GetTimeSeconds() - LastShotWorldTime,
		CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll, ControlRotation.Pitch, ControlRotation.Yaw, CameraManager->GetFOVAngle(),
		SightVsCamera.X, SightVsCamera.Y, UncalibratedVsCamera.X, UncalibratedVsCamera.Y,
		SightVsControl.X, SightVsControl.Y, UncalibratedVsControl.X, UncalibratedVsControl.Y,
		CameraVsControl.X, CameraVsControl.Y, CalibrationRotation.GetAngle() * 1000.0f,
		FVector::Dist(CameraLocation, Sight->GetSightLineOrigin()), WeightADS, WeightFirstPerson, RotationLagSpeed, DeltaSeconds,
		FMath::Clamp(DeltaSeconds * RotationLagSpeed, 0.0f, 1.0f));

	FFileHelper::SaveStringToFile(Output, *SightDiagnosticsFilePath(), FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(), EFileWrite::FILEWRITE_Append);
#endif
}
