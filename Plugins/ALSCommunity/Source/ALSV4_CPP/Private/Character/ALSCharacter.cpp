// Copyright:       Copyright (C) 2022 Doğa Can Yanıkoğlu
// Source Code:     https://github.com/dyanikoglu/ALS-Community


#include "Character/ALSCharacter.h"
#include "Library/ALSADSInterface.h"

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

FVector AALSCharacter::GetFirstPersonCameraTarget()
{
	if (bUseMainCameraTransform)
	{
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
		return IALSADSInterface::Execute_getADSRotation(ControlledInputInterceptor);
	}
	else return FRotator::ZeroRotator;
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