// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#include "Character/ALSCharacter.h"
#include "Library/ALSADSInterface.h"
#include "Components/CapsuleComponent.h"

#include "Engine/StaticMesh.h"
#include "AI/ALSAIController.h"
#include "Kismet/GameplayStatics.h"

AALSCharacter::AALSCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeldObjectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HeldObjectRoot"));
	HeldObjectRoot->SetupAttachment(GetMesh());

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(HeldObjectRoot);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(HeldObjectRoot);

	AIControllerClass = AALSAIController::StaticClass();
}

void AALSCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME(AALSCharacter, ActiveWeaponActor);
}

void AALSCharacter::ClearHeldObject()
{
	StaticMesh->SetStaticMesh(nullptr);
	SkeletalMesh->SetSkeletalMesh(nullptr);
	SkeletalMesh->SetAnimInstanceClass(nullptr);
}

void AALSCharacter::AttachToHand(UStaticMesh* NewStaticMesh, USkeletalMesh* NewSkeletalMesh, UClass* NewAnimClass,
	bool bLeftHand, FVector Offset)
{
	ClearHeldObject();

	if (IsValid(NewStaticMesh))
	{
		StaticMesh->SetStaticMesh(NewStaticMesh);
	}
	else if (IsValid(NewSkeletalMesh))
	{
		SkeletalMesh->SetSkeletalMesh(NewSkeletalMesh);
		if (IsValid(NewAnimClass))
		{
			SkeletalMesh->SetAnimInstanceClass(NewAnimClass);
		}
	}

	FName AttachBone;
	if (bLeftHand)
	{
		AttachBone = TEXT("VB LHS_ik_hand_gun");
	}
	else
	{
		AttachBone = TEXT("VB RHS_ik_hand_gun");
	}

	HeldObjectRoot->AttachToComponent(GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachBone);
	HeldObjectRoot->SetRelativeLocation(Offset);
}

void AALSCharacter::RagdollStart()
{
	ClearHeldObject();
	Super::RagdollStart();
}

void AALSCharacter::RagdollEnd()
{
	Super::RagdollEnd();
	UpdateHeldObject();
}

ECollisionChannel AALSCharacter::GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius)
{
	const FName CameraSocketName = bRightShoulder ? TEXT("TP_CameraTrace_R") : TEXT("TP_CameraTrace_L");
	TraceOrigin = GetMesh()->GetSocketLocation(CameraSocketName);
	TraceRadius = 15.0f;
	return ECC_Camera;
}

FTransform AALSCharacter::GetThirdPersonPivotTarget()
{
	return FTransform(GetActorRotation(),
		(GetMesh()->GetSocketLocation(TEXT("Head")) + GetMesh()->GetSocketLocation(TEXT("root"))) / 2.0f,
		FVector::OneVector);
}

// Weapon of a seated gunner when it knows its own ADS point. Only actually used for Gunner_Standing (an
// open mount with no vehicle sight of its own, so the weapon's own sight is genuinely what the gunner looks
// through) - bUseStationaryADS already reflects this per seat (UVehicleSeatComponent::GetUseADS gates it to
// that role). Every other seat (an enclosed Gunner, or Driver/Passenger) instead uses the vehicle's own
// camera transform (ControlledInputInterceptor, via bUseMainCameraTransform), e.g. a hull-mounted sight.
static AActor* GetGunnerADSWeapon(const AALSCharacter* Character)
{
	const EALSStationaryRole Role = Character->GetStationaryRole();
	if (Role != EALSStationaryRole::Gunner && Role != EALSStationaryRole::Gunner_Standing)
	{
		return nullptr;
	}
	// bUseStationaryADS directly, not the GetUseStationaryADS() accessor: that getter isn't const-qualified
	// and Character is const here; the backing UPROPERTY is public and this is a plain read either way.
	if (!Character->bUseStationaryADS)
	{
		return nullptr;
	}
	AActor* Weapon = Character->ActiveWeaponActor;
	return Weapon && Weapon->GetClass()->ImplementsInterface(UALSADSInterface::StaticClass()) ? Weapon : nullptr;
}

FVector AALSCharacter::GetFirstPersonCameraTarget()
{
	if (bUseMainCameraTransform)
	{
		if (AActor* GunnerWeapon = GetGunnerADSWeapon(this))
		{
			return IALSADSInterface::Execute_getADSTarget(GunnerWeapon);
		}
		if (ControlledInputInterceptor)
		{
			return IALSADSInterface::Execute_getCameraTransform(ControlledInputInterceptor, StationaryRole).GetLocation();
		}
		else return FVector::ZeroVector;
	}
	else
	{
		return GetMesh()->GetSocketLocation("FP_Camera");
	}
}

FVector AALSCharacter::GetFirstPersonCameraTargetADS()
{
	if (ControlledInputInterceptor) {
		if (bUseStationaryADS) {
			if (AActor* GunnerWeapon = GetGunnerADSWeapon(this))
			{
				return IALSADSInterface::Execute_getADSTarget(GunnerWeapon);
			}
			return IALSADSInterface::Execute_getADSTarget(ControlledInputInterceptor);
		}
	}
	else if (ActiveWeaponActor && OverlayState != EALSOverlayState::Default) {
		if (ActiveWeaponActor->GetClass()->ImplementsInterface(UALSADSInterface::StaticClass())) {
			return IALSADSInterface::Execute_getADSTarget(ActiveWeaponActor);
		}
	}
	return SkeletalMesh->GetSocketLocation(TEXT("ADS"));
}

FRotator AALSCharacter::GetFirstPersonCameraRotationADS() {
	if (ControlledInputInterceptor) {
		if (AActor* GunnerWeapon = GetGunnerADSWeapon(this))
		{
			return IALSADSInterface::Execute_getADSRotation(GunnerWeapon);
		}
		// Use Main Camera Transform: the vehicle's own camera transform (e.g. MCT_Gunner socket) tracks the
		// turret/sight, unlike the capsule-rotation fallback below - must be checked before that fallback,
		// since an enclosed Gunner (no weapon ADS of its own) would otherwise always hit it instead.
		if (bUseMainCameraTransform)
		{
			return IALSADSInterface::Execute_getADSRotation(ControlledInputInterceptor);
		}
		// Gunner seat without a weapon or vehicle camera transform to use: look where the unit faces
		if (StationaryRole == EALSStationaryRole::Gunner || StationaryRole == EALSStationaryRole::Gunner_Standing)
		{
			return GetCapsuleComponent()->GetComponentRotation();
		}
		return IALSADSInterface::Execute_getADSRotation(ControlledInputInterceptor);
	}
	else return FRotator::ZeroRotator;
}

FRotator AALSCharacter::GetFirstPersonCameraCalibrationRotationADS() {
	// Enclosed gunner (Use Main Camera Transform): the MCT socket gives a fixed look direction, the weapon's
	// own sight calibration (when it has one) still needs to be added on top so the gunner's zero actually
	// changes what they see, not just the weapon's own aim point. Gunner_Standing is excluded - its
	// GetFirstPersonCameraRotationADS already reads straight from the weapon's own (already calibrated)
	// getADSRotation, so adding this again there would double the correction.
	if (ControlledInputInterceptor && bUseMainCameraTransform && GetStationaryRole() == EALSStationaryRole::Gunner
		&& ActiveWeaponActor && ActiveWeaponActor->GetClass()->ImplementsInterface(UALSADSInterface::StaticClass())) {
		return IALSADSInterface::Execute_getADSCalibrationRotation(ActiveWeaponActor);
	}
	// Same conditions as the hand held weapon case of GetFirstPersonCameraTargetADS
	if (!ControlledInputInterceptor && ActiveWeaponActor && OverlayState != EALSOverlayState::Default
		&& ActiveWeaponActor->GetClass()->ImplementsInterface(UALSADSInterface::StaticClass())) {
		return IALSADSInterface::Execute_getADSCalibrationRotation(ActiveWeaponActor);
	}
	return FRotator::ZeroRotator;
}

void AALSCharacter::OnOverlayStateChanged(EALSOverlayState PreviousState)
{
	Super::OnOverlayStateChanged(PreviousState);
	UpdateHeldObject();
}

void AALSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHeldObjectAnimations();
}

void AALSCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHeldObject();
}

bool AALSCharacter::GetUsingADS()
{
	return UsingADS;
}

void AALSCharacter::SetUsingADS(bool Value)
{
	UsingADS = Value;
}

EALSGait AALSCharacter::GetGait_Implementation() 
{
	return Gait;
}

EALSStance AALSCharacter::GetStance_Implementation() 
{
	return Stance;
}

float AALSCharacter::GetHP()
{
	return HP;
}

float AALSCharacter::GetMaxHP()
{
	return MaxHP;
}