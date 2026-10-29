// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseVehicle.h"
#include "GunnerSeatComponent.h"
#include "BaseVehicleAnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/IPlayer.h"
#include "OrderSystem/OrdersManager.h"
#include "Weapon/WeaponChildComponent.h"
#include "TimerManager.h"
#include "AnimationRuntime.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

ABaseVehicle::ABaseVehicle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));

	VehicleSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Vehicle Skeletal Mesh"));
	VehicleSkeletalMesh->SetupAttachment(VehicleMesh);

	DriverSeat = CreateDefaultSubobject<UVehicleSeatComponent>(TEXT("DriverSeat"));
	DriverSeat->SetupAttachment(VehicleSkeletalMesh);
	DriverSeat->SeatRole = EALSStationaryRole::Driver;

	OrdersManagerComponent = CreateDefaultSubobject<UOrdersManager>(TEXT("Order Manager Component"));

	// Third person (debug) camera orbiting the vehicle. Rotation comes from the driver look, set in CalcCamera.
	ThirdPersonCameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonCameraArm"));
	ThirdPersonCameraArm->SetupAttachment(VehicleMesh);
	ThirdPersonCameraArm->TargetArmLength = 900.0f;
	ThirdPersonCameraArm->TargetOffset = FVector(0.0f, 0.0f, 250.0f);
	ThirdPersonCameraArm->bUsePawnControlRotation = false;
	ThirdPersonCameraArm->bInheritRoll = false;
	ThirdPersonCameraArm->bDoCollisionTest = true;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonCameraArm, USpringArmComponent::SocketName);

	// Wheel, turret and gun bones are animated in C++, no anim blueprint needed
	VehicleSkeletalMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	VehicleSkeletalMesh->AnimClass = UBaseVehicleAnimInstance::StaticClass();

	// Forward / backward input drives and reverses without the driver touching the shifter
	AutomaticShifterPositon = true;

	// Created wheels have no mesh of their own to measure, the radius is set here
	DefaultWheelConfig.WheelMode = EWheelMode::Raycast;
	DefaultWheelConfig.AutoWheelRadius = false;
}

//WHEELS
void ABaseVehicle::OnConstruction(const FTransform& Transform)
{
	// AVS collects and constructs its wheel components in its construction, so ours have to exist first
	BuildWheelsFromSetup();
	Super::OnConstruction(Transform);
	ApplyHiddenBones();
}

//MESH
void ABaseVehicle::ApplyHiddenBones()
{
	if (!VehicleSkeletalMesh || !VehicleSkeletalMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	for (const FName& BoneName : HiddenBones)
	{
		if (VehicleSkeletalMesh->GetBoneIndex(BoneName) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("ABaseVehicle::ApplyHiddenBones - %s: skeletal mesh has no bone '%s'"),
				*GetName(), *BoneName.ToString());
			continue;
		}
		VehicleSkeletalMesh->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
	}
}

TArray<FVehicleWheelSetup> ABaseVehicle::GetEffectiveWheelSetups() const
{
	if (WheelSetups.Num() > 0)
	{
		return WheelSetups;
	}

	TArray<FVehicleWheelSetup> Setups;
	const USkeletalMesh* MeshAsset = VehicleSkeletalMesh ? VehicleSkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	if (!MeshAsset)
	{
		return Setups;
	}

	// lf_wheel_N_jnt / rt_wheel_N_jnt
	TArray<int32> Axles;
	const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();
	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
		FString AxleString = BoneName.ToString();
		if (!AxleString.RemoveFromEnd(TEXT("_jnt"))
			|| !(AxleString.RemoveFromStart(TEXT("lf_wheel_")) || AxleString.RemoveFromStart(TEXT("rt_wheel_")))
			|| !AxleString.IsNumeric())
		{
			continue;
		}
		Setups.AddDefaulted_GetRef().BoneName = BoneName;
		Axles.Add(FCString::Atoi(*AxleString));
	}

	const int32 LastAxle = Axles.Num() > 0 ? FMath::Max(Axles) : 0;
	for (int32 Index = 0; Index < Setups.Num(); ++Index)
	{
		const int32 Axle = Axles[Index];
		Setups[Index].bSteerable = SteerableAxles.Contains(Axle);
		Setups[Index].bDriving = DrivenAxles.Num() == 0 || DrivenAxles.Contains(Axle);
		Setups[Index].bHandbrake = HandbrakeAxles.Num() == 0 ? Axle == LastAxle : HandbrakeAxles.Contains(Axle);
	}
	return Setups;
}

void ABaseVehicle::BuildWheelsFromSetup()
{
	if (!VehicleSkeletalMesh || !VehicleSkeletalMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	// Everything is placed from the reference pose and relative transforms: during construction the
	// components may not be registered, so their world transforms and bone poses are not usable yet
	FTransform MeshToVehicle = FTransform::Identity;
	for (const USceneComponent* Component = VehicleSkeletalMesh; Component && Component != VehicleMesh; Component = Component->GetAttachParent())
	{
		MeshToVehicle = MeshToVehicle * Component->GetRelativeTransform();
	}
	const FReferenceSkeleton& RefSkeleton = VehicleSkeletalMesh->GetSkeletalMeshAsset()->GetRefSkeleton();

	for (const FVehicleWheelSetup& Setup : GetEffectiveWheelSetups())
	{
		const int32 BoneIndex = RefSkeleton.FindBoneIndex(Setup.BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("ABaseVehicle::BuildWheelsFromSetup - %s: skeletal mesh has no bone '%s', wheel skipped"),
				*GetName(), *Setup.BoneName.ToString());
			continue;
		}

		// Same creation method the construction script uses: the wheels are thrown away and rebuilt
		// every time construction reruns, nothing of them is saved with the actor
		UAVS_Wheel* Wheel = NewObject<UAVS_Wheel>(this);
		Wheel->CreationMethod = EComponentCreationMethod::UserConstructionScript;

		Wheel->WheelConfig = DefaultWheelConfig;
		Wheel->WheelConfig.IsDrivingWheel = Setup.bDriving;
		Wheel->WheelConfig.InvertTorque = Setup.bInvertTorque;
		Wheel->WheelConfig.IsSteerableWheel = Setup.bSteerable;
		Wheel->WheelConfig.InvertSteering = Setup.bInvertSteering;
		Wheel->WheelConfig.IsHandbrakeWheel = Setup.bHandbrake;

		// The named bone is moved with the wheel by UBaseVehicleAnimInstance. The wheel itself hangs on the
		// vehicle body at the bone's reference location (not on the bone, it would ride on what it animates),
		// X forward and Z up of the vehicle, whatever way the skeletal mesh or the bone is turned.
		Wheel->BoneName = Setup.BoneName;
		Wheel->SetupAttachment(VehicleMesh);
		Wheel->SetRelativeLocation(MeshToVehicle.TransformPosition(FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, BoneIndex).GetLocation()));
		Wheel->RegisterComponent();
	}
}

void ABaseVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Crew, gunners and the RTS view all need vehicle health, not only the driver who owns the vehicle
	DOREPLIFETIME(ABaseVehicle, HP);
	DOREPLIFETIME(ABaseVehicle, TeamIndex);
	DOREPLIFETIME(ABaseVehicle, PlayerPtr);
	DOREPLIFETIME(ABaseVehicle, OwningFormation);
	// The driver's machine has its own, newer value
	DOREPLIFETIME_CONDITION(ABaseVehicle, ReplicatedDriverLook, COND_SkipOwner);
}

void ABaseVehicle::BeginPlay()
{
	Super::BeginPlay();
	BackupAIController = Cast<AUnitAIController>(GetController());
	ApplyHiddenBones();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseVehicle::OnAnyDamageReceived);

		// Weapons are attached to turret / gun bones and AI gunners shoot from the server pose,
		// so the server has to keep the bones up to date even though it renders nothing
		VehicleSkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}

//SEATS
UVehicleSeatComponent* ABaseVehicle::GetSeatWithRole(EALSStationaryRole StationaryRole) const
{
	TInlineComponentArray<UVehicleSeatComponent*> Seats(this);
	for (UVehicleSeatComponent* Seat : Seats)
	{
		if (Seat->GetRole() == StationaryRole)
		{
			return Seat;
		}
	}
	return nullptr;
}

TArray<UGunnerSeatComponent*> ABaseVehicle::GetGunnerSeats() const
{
	TArray<UGunnerSeatComponent*> OutSeats;
	GetComponents<UGunnerSeatComponent>(OutSeats);
	return OutSeats;
}

//POSSESSION

void ABaseVehicle::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (NewController->IsPlayerController())
	{
		SetHandbrakeInput(false);
		SetEngineRunning(true);
	}
	// A new driver starts looking straight ahead
	ReplicatedDriverLook = FRotator::ZeroRotator;
}

void ABaseVehicle::UnPossessed()
{
	Super::UnPossessed();
	// Nobody controls the vehicle now, so the server owns its inputs and can let go of them
	ReleaseDriverInputs();
}

void ABaseVehicle::PossessByAIController()
{
	if (HasAuthority() && HP > 0.0f && BackupAIController)
	{
		BackupAIController->Possess(this);
	}
}

//DRIVER INPUT
void ABaseVehicle::ForwardMovementAction_Implementation(float Value)
{
	SetThrottleAndBrakeInput(Value);
}

void ABaseVehicle::ForwardMovementActionCompleted_Implementation()
{
	SetThrottleAndBrakeInput(0.0f);
}

void ABaseVehicle::RightMovementAction_Implementation(float Value)
{
	SetSteeringInput(Value);
}

void ABaseVehicle::RightMovementActionCompleted_Implementation()
{
	SetSteeringInput(0.0f);
}

void ABaseVehicle::JumpAction_Implementation(bool bValue)
{
	// JumpAction has a Pressed and a Released trigger: true when the key goes down, false when it comes up
	if (bValue != GetHandbrakeInput())
	{
		SetHandbrakeInput(bValue);
	}
}

void ABaseVehicle::ReleaseDriverInputs()
{
	SetThrottleInput(0.0f);
	SetBrakeInput(0.0f);
	SetSteeringInput(0.0f);
}

//INTERACTION
void ABaseVehicle::InteractionAction_Implementation()
{
	//find vehicle driver seat and invoke Exit delegate with possess
	Server_InteractionAction();
	//IALSInputInterface::Execute_InteractionAction(DriverSeat->GetOccupyingUnit());
}

void ABaseVehicle::Server_InteractionAction_Implementation()
{
	AALSBaseCharacter* Driver = DriverSeat ? DriverSeat->GetOccupyingUnit() : nullptr;
	AController* DriverController = GetController();
	if (!Driver || !DriverController)
	{
		return;
	}

	DriverController->Possess(Driver);

	// Parked: the driver is gone, the vehicle stays where it was left
	ReleaseDriverInputs();
	SetHandbrakeInput(true);
	SetEngineRunning(false);

	PossessByAIController();
	DriverSeat->ExitSeat();
}

//DAMAGE
void ABaseVehicle::OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// Bound on the server only
	if (HP > 0.0f)
	{
		HP -= Damage;
		if (HP <= 0.0f)
		{
			//NotifyDeath();
		}
	}
}

//vehicle interface
float ABaseVehicle::GetSteeringValue_Implementation()
{
	return GetCurrentSteering();
}

//BOOKING
UVehicleAccessPoint* ABaseVehicle::TryBookSeat_Implementation(AALSBaseCharacter* InUnit)
{
	TArray<UVehicleAccessPoint*> AccessPoints = GetAllAccessPoints_Implementation();
	for (UVehicleAccessPoint* AccessPoint : AccessPoints)
	{
		if (AccessPoint->BookSeat(InUnit))
		{
			return AccessPoint;
		}
	}
	return nullptr;
}

TArray<UVehicleAccessPoint*> ABaseVehicle::GetAllAccessPoints_Implementation()
{
	TArray<UVehicleAccessPoint*> OutComponents;
	GetComponents<UVehicleAccessPoint>(OutComponents);
	return OutComponents;
}


//ALSADS INTERFACE
FVector ABaseVehicle::getADSTarget_Implementation()
{
	return FVector(0.0f,0.0f,0.0f);
}

FRotator ABaseVehicle::getADSRotation_Implementation()
{
	return FRotator(0.0f, 0.0f, 0.0f);
}

FTransform ABaseVehicle::getCameraTransform_Implementation(EALSStationaryRole StationaryRole)
{
	// Gunners take their camera from their own weapon, this covers the roles that have no weapon (driver)
	if (const UVehicleSeatComponent* Seat = GetSeatWithRole(StationaryRole))
	{
		return Seat->GetComponentTransform();
	}
	return GetActorTransform();
}

void ABaseVehicle::SetFormation_Implementation(ABaseFormation* Formation)
{
	OwningFormation = Formation;
}

ABaseFormation* ABaseVehicle::GetFormation_Implementation()
{
	return OwningFormation;
}

void ABaseVehicle::SetSelectionCircle_Implementation(bool Visible)
{

}

class UOrdersManager* ABaseVehicle::GetOrdersManagerComponent_Implementation()
{
	return OrdersManagerComponent;
}

bool ABaseVehicle::IsAlive_Implementation()
{
	return HP > 0.0f;
}
void ABaseVehicle::SetOwningPlayer_Implementation(AActor* Player)
{
	PlayerPtr = Player;
}

AActor* ABaseVehicle::GetOwningPlayer_Implementation()
{
	return PlayerPtr;
}

void ABaseVehicle::SetTeamIndex_Implementation(int32 Index)
{
	TeamIndex = Index;
}

int32 ABaseVehicle::GetTeamIndex_Implementation()
{
	return TeamIndex;
}

void ABaseVehicle::SetInstanceIndex_Implementation(int32 Index)
{
	SelectionCircleIndex = Index;
}

int32 ABaseVehicle::GetInstanceIndex_Implementation()
{
	return SelectionCircleIndex;
}

int ABaseVehicle::GetUnitType_Implementation()
{
	return 1;
}

//DRIVER CAMERA
FRotator ABaseVehicle::GetDriverLookRotation() const
{
	return IsLocallyControlled() ? LocalDriverLook : ReplicatedDriverLook;
}

FRotator ABaseVehicle::GetDriverLookWorldRotation() const
{
	return (VehicleMesh->GetComponentQuat() * GetDriverLookRotation().Quaternion()).Rotator();
}

FRotator ABaseVehicle::ClampDriverLook(FRotator Look) const
{
	if (bDriverFirstPerson)
	{
		Look.Yaw = FMath::Clamp(FRotator::NormalizeAxis(Look.Yaw), FirstPersonYawLimit.X, FirstPersonYawLimit.Y);
		Look.Pitch = FMath::Clamp(Look.Pitch, FirstPersonPitchLimit.X, FirstPersonPitchLimit.Y);
	}
	else
	{
		Look.Yaw = FRotator::NormalizeAxis(Look.Yaw);
		Look.Pitch = FMath::Clamp(Look.Pitch, ThirdPersonPitchLimit.X, ThirdPersonPitchLimit.Y);
	}
	Look.Roll = 0.0f;
	return Look;
}

void ABaseVehicle::AddDriverLook(float YawInput, float PitchInput)
{
	if (!IsLocallyControlled())
	{
		return;
	}
	LocalDriverLook = ClampDriverLook(LocalDriverLook + FRotator(PitchInput * DriverLookRate, YawInput * DriverLookRate, 0.0f));

	if (HasAuthority())
	{
		ReplicatedDriverLook = LocalDriverLook;
	}
	else if (!GetWorldTimerManager().IsTimerActive(DriverLookSendTimer))
	{
		// A fixed rate instead of once per input event, the timer also delivers the last value after the mouse stops
		constexpr float DriverLookSendInterval = 1.0f / 20.0f;
		GetWorldTimerManager().SetTimer(DriverLookSendTimer, this, &ABaseVehicle::SendDriverLook, DriverLookSendInterval, false);
	}
}

void ABaseVehicle::SendDriverLook()
{
	Server_SetDriverLookRotation(LocalDriverLook);
}

void ABaseVehicle::Server_SetDriverLookRotation_Implementation(FRotator NewLook)
{
	ReplicatedDriverLook = ClampDriverLook(NewLook);
}

void ABaseVehicle::CameraUpAction_Implementation(float Value)
{
	AddDriverLook(0.0f, bInvertDriverLookPitch ? -Value : Value);
}

void ABaseVehicle::CameraRightAction_Implementation(float Value)
{
	AddDriverLook(Value, 0.0f);
}

void ABaseVehicle::CameraHeldAction_Implementation()
{
	bDriverFirstPerson = !bDriverFirstPerson;
	// First person limits are tighter, bring the look back inside them
	LocalDriverLook = ClampDriverLook(LocalDriverLook);
}

void ABaseVehicle::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	AALSBaseCharacter* Driver = DriverSeat ? DriverSeat->GetOccupyingUnit() : nullptr;
	const bool bFirstPerson = bDriverFirstPerson && Driver && Driver->GetMesh()->DoesSocketExist(FirstPersonCameraSocket);
	UpdateDriverHeadVisibility(bFirstPerson && IsLocallyControlled());

	if (bFirstPerson)
	{
		OutResult.Location = Driver->GetMesh()->GetSocketLocation(FirstPersonCameraSocket);
		OutResult.Rotation = GetDriverLookWorldRotation();
		OutResult.FOV = FirstPersonFOV;
		return;
	}

	ThirdPersonCameraArm->SetRelativeRotation(GetDriverLookRotation());
	Super::CalcCamera(DeltaTime, OutResult);
}

void ABaseVehicle::UpdateDriverHeadVisibility(bool bHide)
{
	AALSBaseCharacter* Driver = bHide && DriverSeat ? DriverSeat->GetOccupyingUnit() : nullptr;
	AALSBaseCharacter* Hidden = HiddenHeadUnit.Get();
	if (Hidden == Driver)
	{
		return;
	}
	// Local only: everybody else keeps seeing the driver's head
	if (Hidden && Hidden->HeadComponent)
	{
		Hidden->HeadComponent->SetVisibility(true, true);
	}
	if (Driver && Driver->HeadComponent)
	{
		Driver->HeadComponent->SetVisibility(false, true);
	}
	HiddenHeadUnit = Driver;
}

void ABaseVehicle::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (IsLocallyControlled())
	{
		LocalDriverLook = FRotator::ZeroRotator;
	}
	else
	{
		// CalcCamera stops running here once this machine no longer drives, show the head again
		UpdateDriverHeadVisibility(false);
	}
}

void ABaseVehicle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UpdateDriverHeadVisibility(false);
	Super::EndPlay(EndPlayReason);
}
