// Fill out your copyright notice in the Description page of Project Settings.


#include "Unit/BaseUnit.h"
#include "Character/RecoilComponent.h"
#include "OrderSystem/OrdersManager.h"
#include "Math/UnrealMathUtility.h"
#include "Weapon/IWeapon.h"
#include "GameFramework/Actor.h"
#include "System/AdBellumGameMode.h"
#include "AIController.h"
#include "Weapon/BaseWeapon.h"
#include "Unit/UnitAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/ALSInteractionInterface.h"
#include "System/AdBellumGameState.h"
#include "Components/CapsuleComponent.h"
#include "Library/NetworkComponent.h"
#include "Vehicle/Vehicle.h"
#include "EBBarrel.h"
#include "Formation/BaseFormation.h"
#include "Player/IPlayer.h"


class ABaseWeapon;

ABaseUnit::ABaseUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RecoilAnimationComponent = CreateDefaultSubobject<URecoilAnimationComponent>(TEXT("Recoil Animation Component"));
	OrdersManagerComponent = CreateDefaultSubobject<UOrdersManager>(TEXT("Order Manager Component"));
	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));
	AIPerception = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("AIPerception"));
	AIPerception->SetIsReplicated(true);
	//init empty weapon array
	WeaponArray.Add(nullptr);
	WeaponArray.Add(nullptr);
	WeaponArray.Add(nullptr);
}

void ABaseUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ABaseUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate the Health property
	DOREPLIFETIME_CONDITION(ABaseUnit, HP, COND_OwnerOnly);
	DOREPLIFETIME(ABaseUnit, TeamIndex);
	DOREPLIFETIME(ABaseUnit, PlayerPtr);
	DOREPLIFETIME(ABaseUnit, OwningFormation);
}

void ABaseUnit::BeginPlay()
{
	Super::BeginPlay();
	BackupAIController = Cast<AUnitAIController>(GetController());
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABaseUnit::OnAnyDamageReceived);
	}
	//RecoilComponent->Deactivate();
	RecoilAnimationComponent->AddTickPrerequisiteComponent(GetMesh());
}

void ABaseUnit::AddRecoil_Implementation()
{
	Server_PlayMontage(RecoilMontage, 1.0f);
	if (RecoilAnimationComponent->IsActive())
	{
		Server_PlayRecoil(true);
	}
}

void ABaseUnit::StopRecoil_Implementation()
{
	Server_PlayRecoil(false);
}

void ABaseUnit::Server_PlayRecoil_Implementation(bool Active)
{
	Multicast_PlayRecoil(Active);
}

void ABaseUnit::Multicast_PlayRecoil_Implementation(bool Active) 
{
	if (Active)
	{
		RecoilAnimationComponent->Play();
	}
	else
	{
		RecoilAnimationComponent->Stop();
	}
}

void ABaseUnit::PossessedBy(AController* NewController)
{
	if (NewController->IsPlayerController())
	{
		SetViewMode(EALSViewMode::FirstPerson);
		RecoilAnimationComponent->Activate();
		IIWeapon::Execute_SetupAim(ActiveWeaponActor, nullptr);
		OrdersManagerComponent->SetStopOrder();
	}
	else
	{
		RecoilAnimationComponent->Deactivate();
	}
	Super::PossessedBy(NewController);
}

void ABaseUnit::UnPossessed() 
{
	if (!GetController()->IsPlayerController())
	{
		BackupAIController->Deactivate();
		RecoilAnimationComponent->Deactivate();
	}
	Super::UnPossessed();
}

void ABaseUnit::PossessByAIController() 
{
	if (HP > 0.0f) 
	{
		BackupAIController->Possess(this);
		BackupAIController->Activate();
	}
}

//INPUT INTERFACE
void ABaseUnit::AimAction_Implementation(bool Value)
{
	if (StationaryRole == EALSStationaryRole::None)
	{
		// add check timer to disable rotation mode
		if (Value) 
		{
			CheckAimCollision();
			GetWorld()->GetTimerManager().SetTimer(CheckAimCollisionTimer, this, &ABaseUnit::CheckAimCollision, 0.16f, true);
		}
		else
		{
			GetWorld()->GetTimerManager().ClearTimer(CheckAimCollisionTimer);
			HandleNonStationaryAimAction(false);
		}
	}
	else
	{
		HandleStationaryAimAction(Value);
	}
}

void ABaseUnit::CheckAimCollision() 
{
	if (LineTraceForObjectType(65.f, EObjectTypeQuery::ObjectTypeQuery1) == nullptr) 
	{
		if (EALSRotationMode::Aiming != GetRotationMode()) 
		{
			HandleNonStationaryAimAction(true);
		}
	}
	else
	{
		HandleNonStationaryAimAction(false);
	}
}

void ABaseUnit::HandleNonStationaryAimAction(bool Value)
{ 
	if (Value && TargetWeaponSocket == EWeaponSocketEnum::NONE && ActiveWeaponActor)
	{
		HandlePressedADS();
		SetRotationMode(EALSRotationMode::Aiming);
	}
	else
	{
		if (TriggerActive) 
		{
			HipFire = true;
		}
		else if (ViewMode == EALSViewMode::ThirdPerson)
		{
			SetRotationMode(DesiredRotationMode);
		}
		else if (ViewMode == EALSViewMode::FirstPerson)
		{
			SetRotationMode(EALSRotationMode::LookingDirection);
		}
	}
}


void ABaseUnit::HandleStationaryAimAction(bool Value)
{
	if (StationaryRole == EALSStationaryRole::Gunner_Standing)
	{
		if (Value)
		{
			SetRotationMode(EALSRotationMode::Aiming);
		} else SetRotationMode(EALSRotationMode::LookingDirection);


		Server_AimAction(Value);
	}

	else if (StationaryRole == EALSStationaryRole::Gunner) {
		if (Value) 
		{
			CameraPOV = 45.0f;
		}
		else 
		{
			CameraPOV = 90.0f;
		}
	}
}

void ABaseUnit::Server_AimAction_Implementation(bool bValue)
{
	if (ControlledInputInterceptor)
	{
		//cancel weapon pitch 0
		IALSInputInterface::Execute_AimAction(ControlledInputInterceptor, bValue);
	}
}

void ABaseUnit::AimActionCompleted_Implementation()
{
	Server_AimActionCompleted();
	HandleReleasedADS();
}

void ABaseUnit::Server_AimActionCompleted_Implementation()
{
	if (ControlledInputInterceptor && GetStationaryRole() == EALSStationaryRole::Gunner_Standing) 
	{
		IALSInputInterface::Execute_AimActionCompleted(ControlledInputInterceptor);
	}
	if (ControlledInputInterceptor && (StationaryRole == EALSStationaryRole::Gunner || StationaryRole == EALSStationaryRole::Gunner_Standing)) 
	{
		HandleTriggerAction(false);
	}
}

void ABaseUnit::HandlePressedADS() 
{
	UsingADS = true;
	if (ActiveWeaponActor) 
	{
		IIWeapon::Execute_NotifyAim(ActiveWeaponActor, true);
		float CameraMovementRate = IIWeapon::Execute_GetCameraSensitivity(ActiveWeaponActor);
		LookLeftRightRate = CameraMovementRate;
		LookUpDownRate = CameraMovementRate;
		if (HipFire) 
		{
			HipFire = false;
		}
	}
	if (EALSStationaryRole::None == StationaryRole)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundADSPressed, HeldObjectRoot->GetComponentLocation());
		RecoilAnimationComponent->SetAimingStatus(true);
	}
}

void ABaseUnit::HandleReleasedADS()
{
	UsingADS = false;
	if (ActiveWeaponActor) 
	{
		IIWeapon::Execute_NotifyAim(ActiveWeaponActor, false);
	}
	LookLeftRightRate = 1.25f;
	LookUpDownRate =  1.25f;
	if (EALSStationaryRole::None == StationaryRole)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundADSReleased, HeldObjectRoot->GetComponentLocation());
		RecoilAnimationComponent->SetAimingStatus(false);
	}
}

void ABaseUnit::TriggerAction_Implementation(bool Value)
{
	TriggerActive = Value;
	if (StationaryRole == EALSStationaryRole::None) 
	{
		if (TargetWeaponSocket == EWeaponSocketEnum::NONE)
		{
			HandleTriggerAction(Value);
		}
	}
	else 
	{
		HandleTriggerAction(Value);
	}
}

void ABaseUnit::TriggerActionCompleted_Implementation() 
{
	if (StationaryRole == EALSStationaryRole::None) 
	{
		if (ActiveWeaponActor)
		{
			EFireMode WeaponFireMode = IIWeapon::Execute_GetFireMode(ActiveWeaponActor);
			HandleTriggerAction(false);
			Server_PlayRecoil(false);
			if (HipFire)
			{
				HipFire = false;
				SetRotationMode(EALSRotationMode::LookingDirection);
			}
		}
	}
}

void ABaseUnit::HandleTriggerAction(bool Value)
{
	if (ControlledInputInterceptor)
	{
		if (StationaryRole == EALSStationaryRole::Gunner_Standing || StationaryRole == EALSStationaryRole::Gunner) {
			if (ActiveWeaponActor) 
			{
				IIWeapon::Execute_Trigger(ActiveWeaponActor, Value);
			}
			else 
			{
				if (GunnerWeaponChangedDelegate.IsBound()) 
				{
					ActiveWeaponActor = GunnerWeaponChangedDelegate.Execute(0);
					if (ActiveWeaponActor) 
					{ 
						IIWeapon::Execute_Trigger(ActiveWeaponActor, Value); 
					}
				}
			}
		}
	}
	else
	{
		if (ActiveWeaponActor) 
		{
			if (!UsingADS)
			{
				HipFire = true;
				SetRotationMode(EALSRotationMode::Aiming);
			}
			IIWeapon::Execute_Trigger(ActiveWeaponActor, Value);
		}
	}
}

void ABaseUnit::InteractionAction_Implementation()
{
	Server_InteractionAction();
}

void ABaseUnit::Server_InteractionAction_Implementation()
{
	if (ControlledInputInterceptor)
	{
		if (ExitDelegate.IsBound())
		{
			ExitDelegate.ExecuteIfBound();
			return;
		}
		//IALSInputInterface::Execute_InteractionAction(ControlledInputInterceptor);
		return;
	}
	//interaction channel 
	UObject* Interactable = LineTraceForObjectType(200.0f, EObjectTypeQuery::ObjectTypeQuery9);
	if (Interactable)
	{
		IALSInteractionInterface::Execute_Interact(Interactable, this);
		return;
	}
}

void ABaseUnit::PrimarySelectionAction_Implementation()
{
	if (MovementState == EALSMovementState::Stationary) 
	{
		HandleStationaryWeaponSwitch(0);
	}
	else
	{
		if (WeaponArray[0] && !IIWeapon::Execute_IsReloading(ActiveWeaponActor))
		{
			HandleWeaponSwitch(EWeaponSocketEnum::PRIMARY);
		}
		
	}
}
void ABaseUnit::SecondarySelectionAction_Implementation() 
{
	if (MovementState == EALSMovementState::Stationary)
	{
		HandleStationaryWeaponSwitch(1);
	}
	else
	{
		if (WeaponArray[1] && !IIWeapon::Execute_IsReloading(ActiveWeaponActor))
		{
			HandleWeaponSwitch(EWeaponSocketEnum::SECONDARY);
		}	
	}
}
void ABaseUnit::ThirdSelectionAction_Implementation() 
{
	if (MovementState == EALSMovementState::Stationary)
	{
		HandleStationaryWeaponSwitch(2);
	}
	else
	{
		if (WeaponArray[2] && !IIWeapon::Execute_IsReloading(ActiveWeaponActor))
		{
			HandleWeaponSwitch(EWeaponSocketEnum::SPECIAL);
		}
	}
}

void ABaseUnit::HandleStationaryWeaponSwitch(int WeaponId) 
{
	if (GunnerWeaponChangedDelegate.IsBound() && StationaryRole == EALSStationaryRole::Gunner)
	{
		AActor* NewActiveWeapon = GunnerWeaponChangedDelegate.Execute(WeaponId);
		if (NewActiveWeapon)
		{
			ActiveWeaponActor = NewActiveWeapon;
			if (IIWeapon::Execute_GetWeaponSocket(ActiveWeaponActor) != EWeaponSocketEnum::SPECIAL) 
			{
				FVector CalibationVector = IIWeapon::Execute_GetCalibrationVector(ActiveWeaponActor, 500.00f);
				IVehicle::Execute_CalibrateADS(ControlledInputInterceptor, CalibationVector);
			}
		}
	}
}

//CUSTOMIZATION
//similar to ALS but dont have ForceMovementUpdate
void ABaseUnit::SetVisibleMeshCustom(USkeletalMesh* NewVisibleMesh)
{
	if (VisibleMesh != NewVisibleMesh)
	{
		const USkeletalMesh* Prev = VisibleMesh;
		VisibleMesh = NewVisibleMesh;
		GetMesh()->SetSkeletalMesh(VisibleMesh);

		// Reset materials to their new mesh defaults
		if (GetMesh() != nullptr)
		{
			for (int32 MaterialIndex = 0; MaterialIndex < GetMesh()->GetNumMaterials(); ++MaterialIndex)
			{
				GetMesh()->SetMaterial(MaterialIndex, nullptr);
			}
		}

		//if (GetLocalRole() != ROLE_Authority)
		//{
		//	Server_SetVisibleMeshCustom(NewVisibleMesh);
		//}
	}
}

void ABaseUnit::Server_SetVisibleMeshCustom_Implementation(USkeletalMesh* NewVisibleMesh)
{
	SetVisibleMeshCustom(NewVisibleMesh);
}

void ABaseUnit::SetupBodyMesh(const FMeshCreatorPrefabStruct& PrefabData)
{
	HandlePrefabDefinition(PrefabData.PrefabDefinition, PrefabData.AdditionalMeshes);
	SetVisibleMeshCustom(UMeshMergeFunctionLibrary::MergeMeshes(MeshMergeParams));
}
void ABaseUnit::SetupHeadMesh(const FMeshCreatorPrefabStruct& PrefabData)
{
	if (PrefabData.PrefabHeadDefinition.Num() == 1)
	{
		const FMeshCreatorDataInputStruct& MeshParams = PrefabData.PrefabHeadDefinition[0];
		const EBodyPart BodyPart = MeshParams.BodyPart;
		FMeshCreatorOutputStruct MeshOutput;
		ICustomizable::Execute_GetSkeletalMeshFromDefinition(this, BodyPart, MeshParams, MeshOutput);
		HeadComponent->SetSkeletalMeshAsset(MeshOutput.OutputMesh);
	}
	else {
		HandlePrefabDefinition(PrefabData.PrefabHeadDefinition, PrefabData.AdditionalHeadMeshes);
		HeadComponent->SetSkeletalMeshAsset(UMeshMergeFunctionLibrary::MergeMeshes(MeshMergeParams));
	}
}

void ABaseUnit::HandlePrefabDefinition(TArray<FMeshCreatorDataInputStruct> MeshDefinitions, TArray<TSoftObjectPtr<USkeletalMesh>> AdditionalMeshes)
{
	MeshMergeParams.MeshesToMerge.Empty();
	for (const FMeshCreatorDataInputStruct& MeshParams : MeshDefinitions)
	{
		const EBodyPart BodyPart = MeshParams.BodyPart;
		FMeshCreatorOutputStruct MeshOutput;
		ICustomizable::Execute_GetSkeletalMeshFromDefinition(this, BodyPart, MeshParams, MeshOutput);
		if (MeshOutput.ArmourParamValue != 1.0f)
		{
			if (BodyPart == EBodyPart::HELMET)
			{
				ArmourMap.Add(EBodyPart::HEAD, MeshOutput.ArmourParamValue);
			}
			else if (BodyPart == EBodyPart::VEST)
			{
				ArmourMap.Add(EBodyPart::TORSO, MeshOutput.ArmourParamValue);
			}
		}
		MeshMergeParams.MeshesToMerge.Add(MeshOutput.OutputMesh);
	}
	for (TSoftObjectPtr<USkeletalMesh> SoftMeshToAdd : AdditionalMeshes)
	{
		USkeletalMesh* MeshToAdd = SoftMeshToAdd.LoadSynchronous();
		MeshMergeParams.MeshesToMerge.Add(MeshToAdd);
	}
}

//END CUSTOMIZATION

//WEAPON HANDLING


void ABaseUnit::Multicast_WeaponArrayUpdate_Implementation(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum)
{
	WeaponArray[(uint8)SocketEnum] = Weapon;
	if (Weapon)
	{
		SocketWeapon(Weapon, SocketEnum);
	}
}

void ABaseUnit::SocketWeapon(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum)
{
	FName SelectedSocketName;
	switch (SocketEnum) 
	{
	case EWeaponSocketEnum::PRIMARY:
	{
		SelectedSocketName = PrimaryWeaponSocketName;
		break;
	}	
	case EWeaponSocketEnum::SECONDARY: 
	{
		SelectedSocketName = SecondaryWeaponSocketName;
		break;
	}
	case EWeaponSocketEnum::SPECIAL:
	{
		SelectedSocketName = SpecialWeaponSocketName;
		break;
	}
	}
	if (Weapon) 
	{
		Weapon->AttachToComponent(GetMesh(),
			FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, true),
			SelectedSocketName);
	}
}


bool ABaseUnit::GetHipFire()
{
	return HipFire;
}

void ABaseUnit::SetHipFire(bool Value)
{
	HipFire = Value;
}

//END WEAPON HANDLING

//DAMAGE
void ABaseUnit::OnAnyDamageReceived(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (HP > 0)
	{
		ServerUpdateHealth(Damage);

		if (GetController())
		{
			if (GetController()->IsPlayerController())
			{
				//reset camera effects
				IIPlayer::Execute_UpdateCameraDamageEffects(GetController(), GetHP() / GetMaxHP());
			}
			else
			{
				BackupAIController->OnDamageReceived(Damage, DamageCauser);
			}
		}

		if (HasAuthority())  // Server only
		{
			ResetHPRegenTimer(); // Restarts the 5s delay
		}
	}
}

void ABaseUnit::ResetHPRegenTimer()
{
	//UKismetSystemLibrary::PrintString(GetWorld(), "RESET HP REGEN TIMER", true, true);
	// Cancel any existing regen tick in case it's running
	GetWorldTimerManager().ClearTimer(RegenerationTickHandle);

	// Restart the 5-second regen start delay
	GetWorldTimerManager().ClearTimer(RegenerationTimerHandle);
	GetWorldTimerManager().SetTimer(RegenerationTimerHandle, this, &ABaseUnit::StartHPRegen, RegenDelay, false);
}

void ABaseUnit::StartHPRegen()
{
	// Start ticking every 1s or 0.5s (adjustable) for gradual HP regeneration
	//UKismetSystemLibrary::PrintString(GetWorld(), "START HP REGEN TIMER", true, true);
	GetWorldTimerManager().SetTimer(RegenerationTickHandle, this, &ABaseUnit::HandleHPRegen, 0.2f, true);
}

void ABaseUnit::StopHPRegen()
{
	GetWorldTimerManager().ClearTimer(RegenerationTickHandle);
	GetWorldTimerManager().ClearTimer(RegenerationTimerHandle);
	//UKismetSystemLibrary::PrintString(GetWorld(), "STOP HP REGEN TIMER", true, true);
}

void ABaseUnit::HandleHPRegen()
{
	if (!HasAuthority()) return; // Server-side only

	//const float MaxHP = GetMaxHP();
	float CurrentHP = GetHP();

	if (CurrentHP >= MaxHP)
	{
		StopHPRegen();
		return;
	}

	float HPPercent = CurrentHP / MaxHP;
	float RegenRatePerSecond = 0.0f;

	// Determine fixed regen rate based on HP range
	if (HPPercent < 0.33f)
	{
		RegenRatePerSecond = 0.03f; // 3%
	}
	else if (HPPercent < 0.75f)
	{
		RegenRatePerSecond = 0.02f; // 2%
	}
	else
	{
		RegenRatePerSecond = 0.01f; // 1%
	}

	// Heal amount per tick (0.2 seconds)
	float RegenAmountPerTick = RegenRatePerSecond * MaxHP * 0.2f;

	ServerUpdateHealth(RegenAmountPerTick);

	if (GetController() && GetController()->IsPlayerController())
	{
		IIPlayer::Execute_UpdateCameraDamageEffects(GetController(), CurrentHP / MaxHP);
	}

	if (CurrentHP >= MaxHP)
	{
		StopHPRegen();
	}
}

float ABaseUnit::GetArmourParamValueForBodyPart_Implementation(EBodyPart BodyPart)
{
	if (ArmourMap.Contains(BodyPart))
	{
		return ArmourMap[BodyPart];
	}
	else 
	{
		return 1.0f;
	}
}

void ABaseUnit::ServerUpdateHealth_Implementation(float Value)
{
	// Server-side: Update the Health value
	HP += Value;

	if (HP <= 0)
	{
		NotifyDeath();
	}

	if (HP > GetMaxHP()) 
	{
		HP = GetMaxHP();
	}
	//UKismetSystemLibrary::PrintString(GetWorld(), "CURRENT HP = " + FString::SanitizeFloat(HP), true, true);
}

bool ABaseUnit::ServerUpdateHealth_Validate(float Value)
{
	return true;
}

void ABaseUnit::NotifyDeath_Implementation()
{
	//enable ragdoll
	ReplicatedRagdollStart();
	//MulticastNotifyRagdoll(true);

	GetCharacterMovement()->DisableMovement();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	AimAction_Implementation(false);
	NotifyClearTargets();
	for (ABaseFormation* EnemyFormation : SeenByFormation)
	{
		EnemyFormation->EnemyDies(this);
	}

	//unregister from AIPerception
	AIPerception->UnregisterFromPerceptionSystem();

	//notify UnitAIController OnDeath
	BackupAIController->Deactivate();

	//RecoilAnimationComponent->Stop();
	//StopRecoil_Implementation();
	IIWeapon::Execute_Trigger(ActiveWeaponActor, false);
	OrdersManagerComponent->SetStopOrder();

	//RecoilComponent->Deactivate();

	if (GetController())
	{
		WasPlayerControlled = GetController()->IsPlayerController();
		if (WasPlayerControlled)
		{
			//reset camera effects
			IIPlayer::Execute_UpdateCameraDamageEffects(GetController(), 1.0f);
			IIPlayer::Execute_UnpossessCharacter(GetController());
		}
		else
		{
			IIPlayer::Execute_SetSelectionCircle(PlayerPtr, this, false);
		}
	}

	for (const auto& KeyValue : WeaponArray)
	{
		if (KeyValue)
		{
			IIWeapon::Execute_Trigger(KeyValue, false);
		}
	}

	//destroy weapons and actor
	GetWorldTimerManager().SetTimer(UnitDiedTimer, this,
	&ABaseUnit::HideActorOnDeath, 3.0f, false);
}

void ABaseUnit::MulticastNotifyRagdoll_Implementation(bool bRagdoll)
{
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName("RagDoll", false);
}

void ABaseUnit::NotifyClearTargets_Implementation()
{
	TArray<AActor*> TargetedBy = TargetingAtActorArray;
	for (AActor* Unit : TargetedBy)
	{
		IITargetable::Execute_ClearTarget(Unit);
	}
}

void ABaseUnit::ClearTarget_Implementation()
{
	BackupAIController->ClearTarget();
}

void ABaseUnit::ConfigureUnit_Implementation(const FMeshCreatorPrefabStruct& UnitPrefab)
{
	SetupBodyMesh(UnitPrefab);
	SetupHeadMesh(UnitPrefab);
}

void ABaseUnit::ConfigureWeapon_Implementation(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum)
{
	WeaponArray[(uint8)SocketEnum] = Weapon;
	if (Weapon)
	{
		SocketWeapon(Weapon, SocketEnum);
		UpdateHeldObject();
	}
}

void ABaseUnit::SetFormation_Implementation(ABaseFormation* Formation)
{
	OwningFormation = Formation;
}

ABaseFormation* ABaseUnit::GetFormation_Implementation()
{
	return OwningFormation;
}

void ABaseUnit::RespawnUnit_Implementation(FTransform RespawnTransform)
{
	HP = 5000.0f;
	//GetMesh()->SetCollisionProfileName("Pawn", false);
	ReplicatedRagdollEnd();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AIPerception->RegisterWithPerceptionSystem();
	if (GetController())
	{
		if (!WasPlayerControlled)
		{
			BackupAIController->Activate();
		}
	}
	
	SetActorTransform(RespawnTransform);
	GetWorldTimerManager().SetTimer(UnitDiedTimer, [this]() 
		{
			SetActorHiddenInGame(false);
			for (ABaseWeapon* Weapon : WeaponArray)
			{
				if (Weapon)
				{
					Weapon->SetActorHiddenInGame(false);
					Weapon->RefillAmmo();
				}
			}
			if (WasPlayerControlled)
			{
				IIPlayer::Execute_PossessCharacter(PlayerPtr, this);
			}
		}, 2.0f, false);
	
}

void ABaseUnit::HideActorOnDeath()
{
	SetActorHiddenInGame(true);
	for (ABaseWeapon* Weapon : WeaponArray)
	{
		if (Weapon)
		{
			Weapon->SetActorHiddenInGame(true);
		}
	}
	OwningFormation->UnitDied(this);
}

//END DAMAGE

// SELECTABLE INTERFACE
void ABaseUnit::SetSelectionCircle_Implementation(bool Visible) 
{
}

class UOrdersManager* ABaseUnit::GetOrdersManagerComponent_Implementation()
{
	return OrdersManagerComponent;
}

bool ABaseUnit::IsAlive_Implementation()
{
	return HP > 0;
}

void ABaseUnit::SetOwningPlayer_Implementation(AActor* InPlayer) 
{
	PlayerPtr = InPlayer;
}

AActor* ABaseUnit::GetOwningPlayer_Implementation()
{
	return PlayerPtr;
}

void ABaseUnit::SetTeamIndex_Implementation(int32 Index)
{
	TeamIndex = Index;
}

int32 ABaseUnit::GetTeamIndex_Implementation()
{
	return TeamIndex;
}

void ABaseUnit::SetInstanceIndex_Implementation(int32 Index) 
{
	SelectionCircleIndex = Index;
}

int32 ABaseUnit::GetInstanceIndex_Implementation()
{
	return SelectionCircleIndex;
}

FVector ABaseUnit::GetSelectionCircleScale_Implementation()
{
	return FVector::One();
}

FVector ABaseUnit::GetSelectionCircleLocation_Implementation()
{
	FVector TargetLocation = GetActorLocation();
	TargetLocation.Z = GetActorLocation().Z - 50.0f;
	return TargetLocation;
}

int ABaseUnit::GetUnitType_Implementation() 
{
	return 0;
}
//END SELECTABLE INTERFACE

//ORDERABLE INTERFACE
void ABaseUnit::Stop_Implementation(FVector TargetPosition) 
{
	if (AAIController* AIController = GetController<AAIController>())
	{
		AIController->StopMovement();
	}
}

void ABaseUnit::MoveOrder_Implementation(FVector TargetPosition)
 {
	GetController<AAIController>()->MoveToLocation(TargetPosition, 7.5f, true, true, true);
 }
void ABaseUnit::AttackTarget_Implementation(UObject* TargetObject) {
	// lightweight early checks
	if (!ActiveWeaponActor || !TargetObject) return;

	// Avoid calling SetupAim too frequently
	UWorld* World = GetWorld();
	if (!World) return;
	float CurrentTime = World->GetTimeSeconds();
	if (!LastAimTarget.IsValid() || LastAimTarget.Get() != TargetObject || (CurrentTime - LastAimSetupTime) > AimSetupCooldown)
	{
		LastAimSetupTime = CurrentTime;
		LastAimTarget = TargetObject;
		IIWeapon::Execute_SetupAim(ActiveWeaponActor, TargetObject);
	}

	// Fire only if not already firing to prevent redundant triggers
	if (!bTriggerActive)
	{
		WeaponTriggerAction();
	}
}

void ABaseUnit::WeaponTriggerAction() 
{
	if (!ActiveWeaponActor) return;

	bTriggerActive = true;
	IIWeapon::Execute_Trigger(ActiveWeaponActor, true);

	// reuse existing timer handle but ensure it's cleared first
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(AIAttackTimer);
		float Delay = FMath::FRandRange(0.1f, 0.2f);
		GetWorldTimerManager().SetTimer(AIAttackTimer, this, &ABaseUnit::StopTriggerTimer, Delay, false);
	}
}

void ABaseUnit::AttackLocation_Implementation(FVector TargetPosition) 
{
    if (!ActiveWeaponActor) return;
    if (!bTriggerActive)
    {
        bTriggerActive = true;
        IIWeapon::Execute_Trigger(ActiveWeaponActor, true);
        if (GetWorld())
        {
            GetWorldTimerManager().ClearTimer(AIAttackTimer);
            GetWorldTimerManager().SetTimer(AIAttackTimer, this, &ABaseUnit::StopTriggerTimer, FMath::FRandRange(0.2f,0.75f), false);
        }
    }
}

void ABaseUnit::StopTriggerTimer()
{
    if (!ActiveWeaponActor) return;

    IIWeapon::Execute_Trigger(ActiveWeaponActor, false);
    bTriggerActive = false;
}

void ABaseUnit::DoCrouch_Implementation() 
{
	if (!bIsCrouched) 
	{
		Crouch();
	}
}


 void ABaseUnit::DoCrawl_Implementation() 
 {
 
 }
 void ABaseUnit::DoStandUp_Implementation() 
 {
	 if (bIsCrouched)
	 {
		 UnCrouch();
	 }
 }

 TScriptInterface<IIWeapon> ABaseUnit::GetWeapon_Implementation()
 {
	 return ActiveWeaponActor;
 }

 void ABaseUnit::OnWeaponUpdated_Implementation(AActor* Weapon)
 {
	 if (GetController() != nullptr && IsPlayerControlled())
	 {
		 if (ActiveWeaponActor == Weapon)
		 {
			 FNotifyHUDData NotifyData;
			 NotifyData.NotifyType = ENotifyHudType::WeaponShot;
			 IIPlayer::Execute_NotifyCharacterHUD(GetController(), NotifyData);
		 }
	 }
 }


bool ABaseUnit::IsReloading_Implementation()
{
	return bIsReloading;
}

//END ORDERABLE INTERFACE

//ITARGETABBLE INTERAFACE
 FVector ABaseUnit::GetHeadLocation_Implementation() 
 {
	 return GetMesh()->GetBoneLocation(FName("head"), EBoneSpaces::WorldSpace);
 }

  FVector ABaseUnit::GetChestLocation_Implementation() 
  {
	  return GetMesh()->GetBoneLocation(FName("spine_03"), EBoneSpaces::WorldSpace);
  }

  FVector ABaseUnit::GetWeaponLocation_Implementation() 
  {
	  if (ActiveWeaponActor)
	  {
		  return ActiveWeaponActor->GetActorLocation();
	  }
	  else 
	  {
		  return FVector::ZeroVector;
	  }
  }

  TArray<AActor*> ABaseUnit::IsTargetedBy_Implementation() 
  {
	  return TargetingAtActorArray;
  }

  void ABaseUnit::SetIsTargetedBy_Implementation(AActor* Actor, bool IsTargeted) 
  {
	  if (IsTargeted) 
	  {
		  TargetingAtActorArray.AddUnique(Actor);
	  }
	  else
	  {
		  TargetingAtActorArray.Remove(Actor);
	  }
  }

  void ABaseUnit::SetIsSeenBy_Implementation(ABaseFormation* Formation, bool IsSeen)
  {
	  if (IsSeen)
	  {
		  SeenByFormation.AddUnique(Formation);
	  }
	  else
	  {
		  SeenByFormation.Remove(Formation);
	  }
  }
 //END ITARGETABBLE INTERAFCE