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

ABaseVehicle::ABaseVehicle(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));

	VehicleSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Vehicle Skeletal Mesh"));
	VehicleSkeletalMesh->SetupAttachment(VehicleMesh);

	DriverSeat = CreateDefaultSubobject<UVehicleSeatComponent>(TEXT("DriverSeat"));
	DriverSeat->SetupAttachment(VehicleSkeletalMesh);
	DriverSeat->SeatRole = EALSStationaryRole::Driver;

	OrdersManagerComponent = CreateDefaultSubobject<UOrdersManager>(TEXT("Order Manager Component"));

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
	if (!bValue)
	{
		ReleaseHandbrake();
		return;
	}

	if (!GetHandbrakeInput())
	{
		SetHandbrakeInput(true);
	}
	constexpr float HandbrakeHoldTime = 0.15f;
	GetWorldTimerManager().SetTimer(HandbrakeReleaseTimer, this, &ABaseVehicle::ReleaseHandbrake, HandbrakeHoldTime, false);
}

void ABaseVehicle::ReleaseHandbrake()
{
	GetWorldTimerManager().ClearTimer(HandbrakeReleaseTimer);
	if (GetHandbrakeInput())
	{
		SetHandbrakeInput(false);
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
