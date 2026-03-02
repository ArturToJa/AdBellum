// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseVehicle.h"
#include "Net/UnrealNetwork.h"
#include "Player/IPlayer.h"
#include "OrderSystem/OrdersManager.h"
#include "Weapon/WeaponChildComponent.h"

ABaseVehicle::ABaseVehicle(const FObjectInitializer& ObjectInitializer)
{
	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));

	VehicleSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Vehicle Skeletal Mesh"));
	VehicleSkeletalMesh->SetupAttachment(VehicleMesh);

	GunnerADS = CreateDefaultSubobject<UArrowComponent>(TEXT("GunnerADS"));
	GunnerADS->AttachToComponent(VehicleSkeletalMesh, FAttachmentTransformRules::KeepRelativeTransform, ADSSocketName);

	OrdersManagerComponent = CreateDefaultSubobject<UOrdersManager>(TEXT("Order Manager Component"));
}

void ABaseVehicle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate the Health property
	DOREPLIFETIME_CONDITION(ABaseVehicle, HP, COND_OwnerOnly);
	DOREPLIFETIME(ABaseVehicle, TeamIndex);
	DOREPLIFETIME(ABaseVehicle, PlayerPtr);
	DOREPLIFETIME(ABaseVehicle, TurretYaw);
	DOREPLIFETIME(ABaseVehicle, GunPitch);
}

void ABaseVehicle::BeginPlay()
{
	Super::BeginPlay();
	BackupAIController = Cast<AUnitAIController>(GetController());
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseVehicle::OnAnyDamageReceived);
	}
	//todo: check const component name
	DriverSeat = Cast<UVehicleSeatComponent>(GetDefaultSubobjectByName(TEXT("Driver_Seat")));
	CalibrateGunnerADS();
}
//POSSESSION

void ABaseVehicle::PossessedBy(AController* NewController)
{
	if (NewController->IsPlayerController())
	{
	}
	else
	{
	}
	Super::PossessedBy(NewController);
}

void ABaseVehicle::UnPossessed()
{
	if (!GetController()->IsPlayerController())
	{
		//BackupAIController->Deactivate();
		//RecoilComponent->Deactivate();
		//BackupAIController->Possess(this);
	}
	Super::UnPossessed();
}

void ABaseVehicle::PossessByAIController()
{
	if (HP > 0.0f)
	{
		//BackupAIController->Possess(this);
		//BackupAIController->Activate();
	}
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
	GetController()->Possess(DriverSeat->GetOccupyingUnit());
	PossessByAIController();
	DriverSeat->ExitSeat();
}
//DAMAGE
void ABaseVehicle::OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (HP > 0)
	{
		ServerUpdateHealth(Damage);
	}
}

void ABaseVehicle::ServerUpdateHealth_Implementation(float Value)
{
	// Server-side: Update the Health value
	HP += Value;

	if (HP <= 0)
	{
		//NotifyDeath();
		
	}
}

bool ABaseVehicle::ServerUpdateHealth_Validate(float Value)
{
	return true;
}

//CAMERA INPUT
void ABaseVehicle::GunnerCameraRightAction_Implementation(float Value) 
{
	Multicast_GunnerCameraRightAction(Value);
}

void ABaseVehicle::Multicast_GunnerCameraRightAction_Implementation(float Value)
{
	TurretYaw = TurretYaw + (Value * TurretTurnRate);
}

void ABaseVehicle::GunnerCameraUpAction_Implementation(float Value) 
{
	Multicast_GunnerCameraUpAction(Value);
}

void ABaseVehicle::Multicast_GunnerCameraUpAction_Implementation(float Value)
{
	GunPitch = GunPitch + (-1 * Value * GunPitchRate);
}

//SHOOTING
void ABaseVehicle::CalibrateGunnerADS() 
{

}


//vehicle interface
void ABaseVehicle::CalibrateADS_Implementation(FVector TargetVector)
{
	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(GunnerADS->GetComponentLocation(), TargetVector);
	GunnerADS->SetWorldRotation(TargetRotation);
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
	return FTransform();
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
	return true;
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