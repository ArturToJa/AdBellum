// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseUnit.h"
#include "Character/RecoilComponent.h"
#include "OrderSystem/OrdersManager.h"
#include "Math/UnrealMathUtility.h"
#include "Weapon/IWeapon.h"
#include "GameFramework/Actor.h"
#include "System/AdBellumGameMode.h"
#include "AIController.h"
#include "Weapon/BaseWeapon.h"
#include "UnitAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Character/ALSInteractionInterface.h"
#include "System/AdBellumGameState.h"
#include "Components/CapsuleComponent.h"
#include "Library/NetworkComponent.h"
#include "Vehicle/GunnerSeatComponent.h"
#include "EBBarrel.h"
#include "Formation/BaseFormation.h"
#include "Player/IPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"


class ABaseWeapon;

ABaseUnit::ABaseUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RecoilAnimationComponent = CreateDefaultSubobject<URecoilAnimationComponent>(TEXT("Recoil Animation Component"));
	NetworkComponent = CreateDefaultSubobject<UNetworkComponent>(TEXT("Network Component"));
	// Units are added to the same must-wait replication batches as their
	// owning ABaseFormation/ABaseWeapon (see AAdBellumGameMode::SpawnUnitsForPlayer),
	// which both already force bAlwaysRelevant = true for this reason: without
	// it, a unit spawned far from another player's camera falls back to
	// default distance-based net relevancy and may never replicate to that
	// player at all, so that player's ReplicationReporter can never reach the
	// batch's expected actor count and ReadyToStartMatch blocks forever.
	bAlwaysRelevant = true;
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
	DOREPLIFETIME(ABaseUnit, OccupiedSeat);
	DOREPLIFETIME(ABaseUnit, CurrentWeaponSocket);
	DOREPLIFETIME(ABaseUnit, TargetWeaponSocket);
	DOREPLIFETIME(ABaseUnit, PrimaryDrawMontage);
	DOREPLIFETIME(ABaseUnit, PrimaryHideMontage);
	DOREPLIFETIME(ABaseUnit, SecondaryDrawMontage);
	DOREPLIFETIME(ABaseUnit, SecondaryHideMontage);
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

	EnsureWeaponMontageDelegatesBound();
}

void ABaseUnit::EnsureWeaponMontageDelegatesBound()
{
	// The mesh/AnimInstance can be swapped after BeginPlay (customization load), so this is called again
	// right before a weapon montage plays rather than relying on the BeginPlay binding alone.
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] EnsureWeaponMontageDelegatesBound on %s Mesh=%s AnimInstance=%s"),
		*GetName(), *GetNameSafe(GetMesh()), *GetNameSafe(AnimInstance));
	if (AnimInstance)
	{
		AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &ABaseUnit::OnWeaponMontageNotifyBegin);
		AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &ABaseUnit::OnWeaponMontageEnded);
	}
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
		if (HasAuthority())
		{
			UE_LOG(LogTemp, Log, TEXT("ABaseUnit::PossessedBy - server: player possession, clearing AI timers and stopping weapon"));
			if (BackupAIController)
			{
				BackupAIController->OrdersManagerComponent->SetStopOrder();
				BackupAIController->ClearAttackTimer();
			}

			// Ensure AI stops shooting and weapon is safe
			if (ActiveWeaponActor)
			{
				IIWeapon::Execute_ResetAim(ActiveWeaponActor);
				IIWeapon::Execute_Trigger(ActiveWeaponActor, false);
				IIWeapon::Execute_NotifyAim(ActiveWeaponActor, false);
			}

			// Stop recoil visuals and animation
			Server_PlayRecoil(false);
			if (RecoilAnimationComponent->IsActive())
			{
				RecoilAnimationComponent->Stop();
				RecoilAnimationComponent->Deactivate();
			}

			// Reset unit local firing state (server)
			TriggerActive = false;
			HipFire = false;
			UsingADS = false;
		}

		// Client-side updates / events
		BP_OnPlayerPossessed(true);
		RecoilAnimationComponent->Activate();
	}
	else
	{
		HandleWeaponStatesOnPlayerDepossessed();
		BP_OnPlayerPossessed(false);
	}
	Super::PossessedBy(NewController);
}

void ABaseUnit::UnPossessed()
{
	if (!GetController()->IsPlayerController())
	{
		BackupAIController->Deactivate();
		BackupAIController->ClearAttackTimer();
		RecoilAnimationComponent->Deactivate();
	}
	Super::UnPossessed();
}

void ABaseUnit::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Checks the controller itself: IsPlayerControlled() depends on PlayerState, which on clients
	// may replicate later than the controller.
	const bool bIsLocallyPlayerControlled = IsLocallyControlled() && Cast<APlayerController>(GetController()) != nullptr;
	if (bWasLocallyPlayerControlled && !bIsLocallyPlayerControlled)
	{
		// Player left this unit (switched away or died). PossessedBy/UnPossessed only run on the server,
		// so the owning client has to clean up its own aim state here.
		ResetLocalAimState();
	}
	else if (!bWasLocallyPlayerControlled && bIsLocallyPlayerControlled)
	{
		// Server forces first person in PossessedBy, but ViewMode replicates with COND_SkipOwner,
		// so the owning client has to apply the same default itself.
		SetViewMode(EALSViewMode::FirstPerson);
	}
	bWasLocallyPlayerControlled = bIsLocallyPlayerControlled;

	// Weapons only poll their owner on a slow timer, tell them about the new controller right away.
	// WeaponArray is filled on clients, Children (actors owned by this unit) is what the server has.
	for (ABaseWeapon* Weapon : WeaponArray)
	{
		if (Weapon)
		{
			Weapon->RefreshOwnerDependentState();
		}
	}
	for (AActor* Child : Children)
	{
		ABaseWeapon* Weapon = Cast<ABaseWeapon>(Child);
		if (Weapon && !WeaponArray.Contains(Weapon))
		{
			Weapon->RefreshOwnerDependentState();
		}
	}
}

void ABaseUnit::ResetLocalAimState()
{
	GetWorldTimerManager().ClearTimer(CheckAimCollisionTimer);

	UsingADS = false;
	HipFire = false;
	TriggerActive = false;
	bReturnToThirdPersonAfterAim = false;
	CameraFOV = 90.0f;
	ClearAimLookRate();
	RecoilAnimationComponent->SetAimingStatus(false);

	for (ABaseWeapon* Weapon : WeaponArray)
	{
		if (Weapon)
		{
			IIWeapon::Execute_NotifyAim(Weapon, false);
			IIWeapon::Execute_SetSightMeshScale(Weapon, false);
		}
	}
	// In case no weapon was left to forward the call
	SetBodyHiddenForScope_Implementation(false);
}

void ABaseUnit::PossessByAIController()
{
	if (IsAlive_Implementation())
	{
		if (HasAuthority())
		{
			UE_LOG(LogTemp, Log, TEXT("ABaseUnit::PossessByAIController - server: AI taking possession"));
			BackupAIController->Possess(this);
			BackupAIController->Activate();
			// Ensure weapon trigger is off when AI takes control
			if (ActiveWeaponActor)
			{
				IIWeapon::Execute_Trigger(ActiveWeaponActor, false);
			}
		}
	}
}

//INPUT INTERFACE
void ABaseUnit::SprintAction_Implementation(bool bValue)
{
	// Nothing to sprint in a vehicle seat, and the aiming branch below would overwrite the gunner's zoom
	if (StationaryRole != EALSStationaryRole::None)
	{
		return;
	}

	if (bValue)
	{
		SetDesiredGait(EALSGait::Sprinting);
		if (EALSRotationMode::Aiming == RotationMode)
		{
			if (ActiveWeaponActor && !IIWeapon::Execute_GetIsScoped(ActiveWeaponActor))
			{
				CameraFOV = IIWeapon::Execute_GetWeaponFOV(ActiveWeaponActor);
			}
		}
	}
	else
	{
		SetDesiredGait(EALSGait::Running);
		if (!ActiveWeaponActor || !IIWeapon::Execute_GetIsScoped(ActiveWeaponActor))
		{
			CameraFOV = 90.0f;
		}
		//CameraFOV = 90.0f;
	}
}

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
			RestoreViewModeAfterAim();
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

void ABaseUnit::RepairLocalActiveWeapon()
{
	// On a client, ActiveWeaponActor can be left empty after a weapon switch: the blueprint switch logic
	// clears it locally and only the server assigns the new weapon. When no switch is in progress,
	// take the weapon from the socket the unit is currently holding.
	if (ActiveWeaponActor || HasAuthority() || !IsLocallyControlled() || StationaryRole != EALSStationaryRole::None
		|| TargetWeaponSocket != EWeaponSocketEnum::NONE)
	{
		return;
	}
	const int32 SocketIndex = (int32)CurrentWeaponSocket;
	if (WeaponArray.IsValidIndex(SocketIndex) && WeaponArray[SocketIndex])
	{
		ActiveWeaponActor = WeaponArray[SocketIndex];
		UE_LOG(LogTemp, Warning, TEXT("ABaseUnit::RepairLocalActiveWeapon - %s: ActiveWeaponActor was empty on client, restored %s from socket %d"),
			*GetName(), *GetNameSafe(ActiveWeaponActor), SocketIndex);
	}
}

void ABaseUnit::HandleNonStationaryAimAction(bool Value)
{
	RepairLocalActiveWeapon();

	if (Value && TargetWeaponSocket == EWeaponSocketEnum::NONE && ActiveWeaponActor)
	{
		// Aiming always goes through first person ADS, SetViewMode also tells the server
		if (ViewMode == EALSViewMode::ThirdPerson && IsLocallyControlled() && IsPlayerControlled())
		{
			bReturnToThirdPersonAfterAim = true;
			SetViewMode(EALSViewMode::FirstPerson);
		}
		HandlePressedADS();
		SetRotationMode(EALSRotationMode::Aiming);
		if (IsPlayerControlled()) 
		{
			//set FOV
			if (IIWeapon::Execute_GetIsScoped(ActiveWeaponActor)) 
			{
				CameraFOV = IIWeapon::Execute_GetWeaponFOV(ActiveWeaponActor);
				IIWeapon::Execute_SetSightMeshScale(ActiveWeaponActor, true);
			}
		}
	}
	else
	{
		if (TriggerActive)
		{
			HipFire = true;
			if (IsPlayerControlled())
			{
				if (ActiveWeaponActor)
				{
					IIWeapon::Execute_SetSightMeshScale(ActiveWeaponActor, false);
				}
				CameraFOV = 90.0f;
			}
		}
		else if (ViewMode == EALSViewMode::ThirdPerson)
		{
			SetRotationMode(DesiredRotationMode);
		}
		else if (ViewMode == EALSViewMode::FirstPerson)
		{
			SetRotationMode(EALSRotationMode::LookingDirection);
			if (IsPlayerControlled())
			{
				if (ActiveWeaponActor)
				{
					IIWeapon::Execute_SetSightMeshScale(ActiveWeaponActor, false);
				}
				CameraFOV = 90.0f;
			}
		}
	}
}


void ABaseUnit::HandleStationaryAimAction(bool Value)
{
	// AimAction arrives every frame while the button is held, only a change does anything
	// (Server_AimAction is reliable, sending it every frame floods the connection)
	if (bStationaryAimHeld == Value)
	{
		return;
	}
	bStationaryAimHeld = Value;

	if (StationaryRole == EALSStationaryRole::Gunner_Standing)
	{
		if (Value)
		{
			SetRotationMode(EALSRotationMode::Aiming);
		}
		else SetRotationMode(EALSRotationMode::LookingDirection);


		Server_AimAction(Value);
	}

	if (StationaryRole == EALSStationaryRole::Gunner_Standing)
	{
		// Open mount: FOV follows the weapon's own sight, same as a handheld weapon aiming down sights
		const float WeaponFOV = (Value && ActiveWeaponActor) ? IIWeapon::Execute_GetWeaponFOV(ActiveWeaponActor) : 0.0f;
		CameraFOV = WeaponFOV > 0.0f ? WeaponFOV : 90.0f;
	}
	else if (StationaryRole == EALSStationaryRole::Gunner)
	{
		UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(OccupiedSeat);
		if (GunnerSeat && GunnerSeat->HasZoomLevels())
		{
			// Zoom table present: the gunner always looks through its own optic (no separate hip-fire FOV),
			// the aim button instead steps to the next zoom level, wrapping back to the first after the last.
			if (Value)
			{
				GunnerSeat->CycleZoom(1);
			}
			CameraFOV = GunnerSeat->GetCurrentZoomFOV();
		}
		else
		{
			// No zoom table on the seat: fall back to hold-to-aim using the weapon's own ADS FOV,
			// same as Gunner_Standing above
			const float WeaponFOV = (Value && ActiveWeaponActor) ? IIWeapon::Execute_GetWeaponFOV(ActiveWeaponActor) : 0.0f;
			CameraFOV = WeaponFOV > 0.0f ? WeaponFOV : 90.0f;
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
	// AimAction only reports the button while it is held, so a seated unit leaves its aim state here
	if (StationaryRole != EALSStationaryRole::None)
	{
		HandleStationaryAimAction(false);
	}
	// Also when the unit left its seat with the button still held
	bStationaryAimHeld = false;
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
		// Sight / weapon sensitivity replaces the configured look rates only while aiming
		SetAimLookRate(IIWeapon::Execute_GetCameraSensitivity(ActiveWeaponActor));
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
	ClearAimLookRate();
	if (EALSStationaryRole::None == StationaryRole)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundADSReleased, HeldObjectRoot->GetComponentLocation());
		RecoilAnimationComponent->SetAimingStatus(false);
	}
}

void ABaseUnit::TriggerAction_Implementation(bool Value)
{
	RepairLocalActiveWeapon();
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
				SetRotationMode(ViewMode == EALSViewMode::ThirdPerson ? DesiredRotationMode : EALSRotationMode::LookingDirection);
			}
		}
	}
}

void ABaseUnit::RestoreViewModeAfterAim()
{
	if (bReturnToThirdPersonAfterAim)
	{
		bReturnToThirdPersonAfterAim = false;
		// OnViewModeChanged puts the rotation mode back to DesiredRotationMode, unless still hip firing
		SetViewMode(EALSViewMode::ThirdPerson);
	}
}

void ABaseUnit::CameraHeldAction_Implementation()
{
	// View mode is driven by aiming while the aim button is held
	if (UsingADS || bReturnToThirdPersonAfterAim)
	{
		return;
	}
	Super::CameraHeldAction_Implementation();
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
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] PrimarySelectionAction_Implementation on %s MovementState=%d WeaponArray[0]=%s IsReloading=%d"),
		*GetName(), (int32)MovementState, *GetNameSafe(WeaponArray.IsValidIndex(0) ? WeaponArray[0] : nullptr), IsReloading_Implementation());
	if (MovementState == EALSMovementState::Stationary)
	{
		HandleStationaryWeaponSwitch(0);
	}
	else
	{
		if (WeaponArray[0] && !IsReloading_Implementation())
		{
			HandleWeaponSwitch(EWeaponSocketEnum::PRIMARY);
		}

	}
}
void ABaseUnit::SecondarySelectionAction_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SecondarySelectionAction_Implementation on %s MovementState=%d WeaponArray[1]=%s IsReloading=%d"),
		*GetName(), (int32)MovementState, *GetNameSafe(WeaponArray.IsValidIndex(1) ? WeaponArray[1] : nullptr), IsReloading_Implementation());
	if (MovementState == EALSMovementState::Stationary)
	{
		HandleStationaryWeaponSwitch(1);
	}
	else
	{
		if (WeaponArray[1] && !IsReloading_Implementation())
		{
			HandleWeaponSwitch(EWeaponSocketEnum::SECONDARY);
		}
	}
}
void ABaseUnit::ReloadAction_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] ReloadAction_Implementation on %s StationaryRole=%d"), *GetName(), (int32)StationaryRole);
	if (StationaryRole == EALSStationaryRole::None)
	{
		HandleReloadAction();
	}
}

void ABaseUnit::ThirdSelectionAction_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] ThirdSelectionAction_Implementation on %s MovementState=%d WeaponArray[2]=%s IsReloading=%d"),
		*GetName(), (int32)MovementState, *GetNameSafe(WeaponArray.IsValidIndex(2) ? WeaponArray[2] : nullptr), IsReloading_Implementation());
	if (MovementState == EALSMovementState::Stationary)
	{
		HandleStationaryWeaponSwitch(2);
	}
	else
	{
		if (WeaponArray[2] && !IsReloading_Implementation())
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
			// The weapon brings its own ADS point and zero, nothing to recalibrate on the vehicle
			ActiveWeaponActor = NewActiveWeapon;
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

void ABaseUnit::Multicast_ClearWeaponSlot_Implementation(EWeaponSocketEnum SocketEnum)
{
	const int32 SlotIndex = (int32)SocketEnum;
	if (!WeaponArray.IsValidIndex(SlotIndex))
	{
		return;
	}

	ABaseWeapon* Weapon = WeaponArray[SlotIndex];
	if (Weapon && ActiveWeaponActor == Weapon)
	{
		// Still valid here, so the player's aim state (ADS, FOV, hidden meshes) can be reset through it
		if (IsLocallyControlled())
		{
			ResetLocalAimState();
		}
		ActiveWeaponActor = nullptr;
	}
	WeaponArray[SlotIndex] = nullptr;
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

ABaseWeapon* ABaseUnit::GetWeaponBySocket(EWeaponSocketEnum weaponSocketType) const
{
	const int32 Index = static_cast<int32>(weaponSocketType);
	return WeaponArray.IsValidIndex(Index) ? WeaponArray[Index] : nullptr;
}

ABaseWeapon* ABaseUnit::GetActiveWeapon() const
{
	return GetWeaponBySocket(CurrentWeaponSocket);
}

void ABaseUnit::SelectOverlayState(EWeaponSocketEnum DesiredSocket)
{
	switch (DesiredSocket)
	{
	case EWeaponSocketEnum::PRIMARY:
		Server_SetOverlayState(EALSOverlayState::Rifle, false);
		break;
	case EWeaponSocketEnum::SECONDARY:
		Server_SetOverlayState(EALSOverlayState::PistolTwoHanded, false);
		break;
	case EWeaponSocketEnum::SPECIAL:
		Server_SetOverlayState(EALSOverlayState::RPG, false);
		break;
	case EWeaponSocketEnum::NONE:
	default:
		Server_SetOverlayState(EALSOverlayState::Default, false);
		break;
	}
}

void ABaseUnit::HandleReloadAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HandleReloadAction called on %s (HasAuthority=%d, LocallyControlled=%d, CurrentWeaponSocket=%d, TargetWeaponSocket=%d)"),
		*GetName(), HasAuthority(), IsLocallyControlled(), (int32)CurrentWeaponSocket, (int32)TargetWeaponSocket);

	ABaseWeapon* Weapon = GetActiveWeapon();
	if (!Weapon || !Weapon->EBarrel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HandleReloadAction aborted: %s"), !Weapon ? TEXT("GetActiveWeapon() returned null") : TEXT("Weapon->EBarrel is null"));
		return;
	}

	const bool bHasSpareAmmo = Weapon->TotalAmmo > 0;
	const bool bMagazineNotFull = Weapon->EBarrel->CycleAmmoCount != Weapon->MagazineSize;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HandleReloadAction weapon=%s TotalAmmo=%d MagazineSize=%d CycleAmmoCount=%d bHasSpareAmmo=%d bMagazineNotFull=%d"),
		*Weapon->GetName(), Weapon->TotalAmmo, Weapon->MagazineSize, Weapon->EBarrel->CycleAmmoCount, bHasSpareAmmo, bMagazineNotFull);
	if (!bHasSpareAmmo || !bMagazineNotFull)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HandleReloadAction aborted: nothing to reload"));
		return;
	}

	UAnimMontage* MontageToPlay = AALSBaseCharacter::GetStance() == EALSStance::Standing ? Weapon->ReloadStandMontage : Weapon->ReloadCrouchMontage;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HandleReloadAction stance=%d MontageToPlay=%s"), (int32)AALSBaseCharacter::GetStance(), *GetNameSafe(MontageToPlay));
	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HandleReloadAction aborted: no reload montage assigned on weapon"));
		return;
	}

	PlayMontageReplicated(MontageToPlay, 1.0f);
	Server_StartReload(Weapon);
}

void ABaseUnit::Server_StartReload_Implementation(ABaseWeapon* Weapon)
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] Server_StartReload on %s Weapon=%s"), *GetName(), *GetNameSafe(Weapon));
	if (Weapon)
	{
		IIWeapon::Execute_Reload(Weapon);
	}
}

void ABaseUnit::HandleWeaponSwitch(EWeaponSocketEnum DesiredWeaponSocket)
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HandleWeaponSwitch called on %s DesiredWeaponSocket=%d (HasAuthority=%d, LocallyControlled=%d, ControlledInputInterceptor=%s)"),
		*GetName(), (int32)DesiredWeaponSocket, HasAuthority(), IsLocallyControlled(), *GetNameSafe(ControlledInputInterceptor));

	Server_SetRotationMode(EALSRotationMode::LookingDirection, false);

	if (IsValid(ControlledInputInterceptor))
	{
		// Something (a gunner seat, a turret, ...) is intercepting this unit's input: forward the
		// selection to it instead of switching this unit's own weapon.
		switch (DesiredWeaponSocket)
		{
		case EWeaponSocketEnum::PRIMARY:
			IALSInputInterface::Execute_PrimarySelectionAction(ControlledInputInterceptor);
			break;
		case EWeaponSocketEnum::SECONDARY:
			IALSInputInterface::Execute_SecondarySelectionAction(ControlledInputInterceptor);
			break;
		case EWeaponSocketEnum::SPECIAL:
			IALSInputInterface::Execute_ThirdSelectionAction(ControlledInputInterceptor);
			break;
		default:
			break;
		}
	}
	else
	{
		switch (DesiredWeaponSocket)
		{
		case EWeaponSocketEnum::PRIMARY:
			SelectPrimaryWeapon();
			break;
		case EWeaponSocketEnum::SECONDARY:
			SelectSecondaryWeapon();
			break;
		case EWeaponSocketEnum::SPECIAL:
			SelectSpecialWeapon();
			break;
		default:
			break;
		}
	}

	if (ActiveWeaponActor)
	{
		IIWeapon::Execute_ResetAim(ActiveWeaponActor);
	}
}

void ABaseUnit::SetTargetWeaponSocketReplicated(EWeaponSocketEnum DesiredSocket)
{
	TargetWeaponSocket = DesiredSocket;
	if (!HasAuthority())
	{
		Server_SetTargetWeaponSocket(DesiredSocket);
	}
}

void ABaseUnit::Server_SetTargetWeaponSocket_Implementation(EWeaponSocketEnum DesiredSocket)
{
	TargetWeaponSocket = DesiredSocket;
}

void ABaseUnit::SelectPrimaryWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectPrimaryWeapon called on %s CurrentWeaponSocket=%d HasAuthority=%d LocallyControlled=%d"), *GetName(), (int32)CurrentWeaponSocket, HasAuthority(), IsLocallyControlled());
	if (CurrentWeaponSocket == EWeaponSocketEnum::PRIMARY)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectPrimaryWeapon: already Primary, cancelling TargetWeaponSocket"));
		SetTargetWeaponSocketReplicated(EWeaponSocketEnum::NONE);
		return;
	}

	SetTargetWeaponSocketReplicated(EWeaponSocketEnum::PRIMARY);
	switch (CurrentWeaponSocket)
	{
	case EWeaponSocketEnum::SECONDARY:
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectPrimaryWeapon: holstering Secondary via SecondaryHideMontage=%s"), *GetNameSafe(SecondaryHideMontage));
		PlayMontageReplicated(SecondaryHideMontage, 1.0f);
		break;
	case EWeaponSocketEnum::SPECIAL:
		// Special is back-mounted like Primary, so holstering it reuses Primary's hide montage.
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectPrimaryWeapon: holstering Special via PrimaryHideMontage=%s"), *GetNameSafe(PrimaryHideMontage));
		PlayMontageReplicated(PrimaryHideMontage, 1.0f);
		break;
	case EWeaponSocketEnum::NONE:
	default:
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectPrimaryWeapon: drawing directly via PrimaryDrawMontage=%s"), *GetNameSafe(PrimaryDrawMontage));
		PlayMontageReplicated(PrimaryDrawMontage, 1.0f);
		break;
	}
}

void ABaseUnit::SelectSecondaryWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSecondaryWeapon called on %s CurrentWeaponSocket=%d HasAuthority=%d LocallyControlled=%d"), *GetName(), (int32)CurrentWeaponSocket, HasAuthority(), IsLocallyControlled());
	if (CurrentWeaponSocket == EWeaponSocketEnum::SECONDARY)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSecondaryWeapon: already Secondary, cancelling TargetWeaponSocket"));
		SetTargetWeaponSocketReplicated(EWeaponSocketEnum::NONE);
		return;
	}

	SetTargetWeaponSocketReplicated(EWeaponSocketEnum::SECONDARY);
	switch (CurrentWeaponSocket)
	{
	case EWeaponSocketEnum::PRIMARY:
	case EWeaponSocketEnum::SPECIAL:
		// Both Primary and Special are back-mounted, so holstering either reuses Primary's hide montage.
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSecondaryWeapon: holstering via PrimaryHideMontage=%s"), *GetNameSafe(PrimaryHideMontage));
		PlayMontageReplicated(PrimaryHideMontage, 1.0f);
		break;
	case EWeaponSocketEnum::NONE:
	default:
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSecondaryWeapon: drawing directly via SecondaryDrawMontage=%s"), *GetNameSafe(SecondaryDrawMontage));
		PlayMontageReplicated(SecondaryDrawMontage, 1.0f);
		break;
	}
}

void ABaseUnit::SelectSpecialWeapon()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSpecialWeapon called on %s CurrentWeaponSocket=%d HasAuthority=%d LocallyControlled=%d"), *GetName(), (int32)CurrentWeaponSocket, HasAuthority(), IsLocallyControlled());
	if (CurrentWeaponSocket == EWeaponSocketEnum::SPECIAL)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSpecialWeapon: already Special, cancelling TargetWeaponSocket"));
		SetTargetWeaponSocketReplicated(EWeaponSocketEnum::NONE);
		return;
	}

	SetTargetWeaponSocketReplicated(EWeaponSocketEnum::SPECIAL);
	switch (CurrentWeaponSocket)
	{
	case EWeaponSocketEnum::PRIMARY:
		// Primary<->Special fast-swap: the hand is already behind the back, so this skips the holster
		// montage entirely (see HandleWeaponDrawingNotify's CheckFastSwap case).
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSpecialWeapon: fast-swap from Primary, no montage"));
		SelectOverlayState(EWeaponSocketEnum::PRIMARY);
		break;
	case EWeaponSocketEnum::SECONDARY:
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSpecialWeapon: holstering Secondary via SecondaryHideMontage=%s"), *GetNameSafe(SecondaryHideMontage));
		PlayMontageReplicated(SecondaryHideMontage, 1.0f);
		break;
	case EWeaponSocketEnum::NONE:
	default:
		// No dedicated Special draw montage: reuses Primary's, per the back-mounted weapon design.
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] SelectSpecialWeapon: drawing directly via PrimaryDrawMontage=%s"), *GetNameSafe(PrimaryDrawMontage));
		PlayMontageReplicated(PrimaryDrawMontage, 1.0f);
		break;
	}
}

void ABaseUnit::AttachToHandOverriden_Implementation(UStaticMesh* NewStaticMesh, USkeletalMesh* NewSkeletalMesh, TSubclassOf<UAnimInstance> NewAnimClass, bool bLeftHand, FVector Offset, AActor* AttachActor)
{
	// Implemented in the Blueprint (mesh/anim class swap on the held-object component, hand-IK socket
	// attach, Hand IK setup); this default only runs if no BP override exists.
}

void ABaseUnit::AttachWeaponBySocket(EWeaponSocketEnum weaponSocketType)
{
	WeaponToEquip = GetWeaponBySocket(weaponSocketType);
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] AttachWeaponBySocket on %s weaponSocketType=%d WeaponToEquip=%s"),
		*GetName(), (int32)weaponSocketType, *GetNameSafe(WeaponToEquip));
	if (!WeaponToEquip)
	{
		return;
	}

	GunOffset = WeaponToEquip->WeaponHeldRelativeLocation;
	AttachToHandOverriden(nullptr, nullptr, nullptr, false, GunOffset, WeaponToEquip);
	CurrentWeaponSocket = weaponSocketType;

	if (URecoilData* WeaponRecoilData = WeaponToEquip->RecoilData)
	{
		EFireMode_PRAS PRASFireMode = EFireMode_PRAS::Auto;
		if (WeaponToEquip->EBarrel)
		{
			switch (WeaponToEquip->EBarrel->FireMode)
			{
			case EFireMode::FM_Semiauto:
			case EFireMode::FM_Manual:
			case EFireMode::FM_Slamfire:
				PRASFireMode = EFireMode_PRAS::Semi;
				break;
			case EFireMode::FM_Burst:
			case EFireMode::FM_InterBurst:
				PRASFireMode = EFireMode_PRAS::Burst;
				break;
			case EFireMode::FM_Auto:
			case EFireMode::FM_Gatling:
			default:
				PRASFireMode = EFireMode_PRAS::Auto;
				break;
			}
		}
		RecoilAnimationComponent->SetFireMode(PRASFireMode);
		const float FireRateMax = WeaponToEquip->EBarrel ? WeaponToEquip->EBarrel->FireRateMax : 0.0f;
		RecoilAnimationComponent->Init(WeaponRecoilData, FireRateMax * 60.0f);

		if (WeaponToEquip->WeaponMeshComponent)
		{
			PivotPointCS = WeaponToEquip->WeaponMeshComponent->GetSocketTransform(FName("Slide"), RTS_Component);
		}
	}
}

void ABaseUnit::PlayMontageReplicated(UAnimMontage* Montage, float PlayRate)
{
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] PlayMontageReplicated called on %s with a null Montage, aborting"), *GetName());
		return;
	}
	EnsureWeaponMontageDelegatesBound();
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] PlayMontageReplicated on %s Montage=%s PlayRate=%f AnimInstance=%s"),
		*GetName(), *Montage->GetName(), PlayRate, *GetNameSafe(AnimInstance));
	if (AnimInstance)
	{
		const float Length = AnimInstance->Montage_Play(Montage, PlayRate);
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] PlayMontageReplicated local Montage_Play returned length=%f"), Length);
	}
	Server_PlayMontage(Montage, PlayRate);
}

void ABaseUnit::Server_FinishMontage_Implementation()
{
	ABaseWeapon* Weapon = GetActiveWeapon();
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] Server_FinishMontage on %s Weapon=%s bIsReloading=%d"),
		*GetName(), *GetNameSafe(Weapon), Weapon ? Weapon->bIsReloading : false);
	if (Weapon && Weapon->bIsReloading)
	{
		IIWeapon::Execute_ReloadComplete(Weapon);
	}
}

bool ABaseUnit::IsWeaponMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return false;
	}
	if (Montage == PrimaryDrawMontage || Montage == PrimaryHideMontage ||
		Montage == SecondaryDrawMontage || Montage == SecondaryHideMontage)
	{
		return true;
	}
	const ABaseWeapon* Weapon = GetActiveWeapon();
	return Weapon && (Montage == Weapon->ReloadStandMontage || Montage == Weapon->ReloadCrouchMontage);
}

void ABaseUnit::OnWeaponMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	UAnimMontage* NotifyMontage = Cast<UAnimMontage>(BranchingPointPayload.SequenceAsset);
	const bool bIsWeapon = IsWeaponMontage(NotifyMontage);
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] OnWeaponMontageNotifyBegin on %s NotifyName=%s Montage=%s IsWeaponMontage=%d"),
		*GetName(), *NotifyName.ToString(), *GetNameSafe(NotifyMontage), bIsWeapon);
	if (bIsWeapon)
	{
		MontageNotifiesLogic(NotifyName);
	}
}

void ABaseUnit::OnWeaponMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	const bool bIsWeapon = IsWeaponMontage(Montage);
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] OnWeaponMontageEnded on %s Montage=%s bInterrupted=%d IsWeaponMontage=%d"),
		*GetName(), *GetNameSafe(Montage), bInterrupted, bIsWeapon);
	if (!bInterrupted && bIsWeapon)
	{
		Server_FinishMontage();
	}
}

void ABaseUnit::MontageNotifiesLogic(FName NotifyName)
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] MontageNotifiesLogic on %s NotifyName=%s"), *GetName(), *NotifyName.ToString());
	if (NotifyName == FName("OldMagGrab") || NotifyName == FName("GrabNewMag") || NotifyName == FName("AttachMag"))
	{
		HandleReloadMontageNotify(NotifyName);
		return;
	}

	if (NotifyName == FName("DrawWeapon") || NotifyName == FName("HideWeapon") ||
		NotifyName == FName("CheckTargetSocket") || NotifyName == FName("CheckFastSwap"))
	{
		HandleWeaponDrawingNotify(NotifyName);
		return;
	}

	// Any other notify on a weapon montage: nothing to do (completion is handled by OnWeaponMontageEnded).
}

void ABaseUnit::HandleReloadMontageNotify(FName NotifyName)
{
	ABaseWeapon* Weapon = GetActiveWeapon();
	if (!Weapon || !Weapon->Magazine)
	{
		return;
	}

	if (NotifyName == FName("OldMagGrab"))
	{
		Weapon->Magazine->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, true));
		Weapon->Magazine->SetSimulatePhysics(true);
	}
	else if (NotifyName == FName("AttachMag"))
	{
		Weapon->Magazine->SetSimulatePhysics(false);
		Weapon->Magazine->SetVisibility(true, false);
		Weapon->Magazine->AttachToComponent(Weapon->WeaponMeshComponent,
			FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, true),
			FName("Mag"));
	}
	// GrabNewMag: no visual/state change defined yet.
}

void ABaseUnit::HandleWeaponDrawingNotify(FName NotifyName)
{
	if (NotifyName == FName("HideWeapon"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] HideWeapon on %s TargetWeaponSocket=%d CurrentWeaponSocket=%d ActiveWeaponActor(before)=%s HasAuthority=%d LocallyControlled=%d"),
			*GetName(), (int32)TargetWeaponSocket, (int32)CurrentWeaponSocket, *GetNameSafe(ActiveWeaponActor), HasAuthority(), IsLocallyControlled());
		// Re-holsters whichever weapon is currently equipped.
		SocketWeapon(GetWeaponBySocket(CurrentWeaponSocket), CurrentWeaponSocket);
		Server_SetOverlayState(EALSOverlayState::Default, false);
		// Cleared until DrawWeapon assigns the new weapon; RepairLocalActiveWeapon restores it from
		// CurrentWeaponSocket if a client is left without one (e.g. the switch never completes).
		ActiveWeaponActor = nullptr;
		return;
	}

	if (NotifyName == FName("DrawWeapon"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] DrawWeapon on %s TargetWeaponSocket=%d CurrentWeaponSocket(before)=%d HasAuthority=%d LocallyControlled=%d"),
			*GetName(), (int32)TargetWeaponSocket, (int32)CurrentWeaponSocket, HasAuthority(), IsLocallyControlled());
		// Reads TargetWeaponSocket before it gets cleared below.
		SelectOverlayState(TargetWeaponSocket);

		USoundBase* RaiseSound = nullptr;
		switch (TargetWeaponSocket)
		{
		case EWeaponSocketEnum::PRIMARY:   RaiseSound = PrimaryDrawSound; break;
		case EWeaponSocketEnum::SECONDARY: RaiseSound = SecondaryDrawSound; break;
		case EWeaponSocketEnum::SPECIAL:   RaiseSound = SpecialDrawSound; break;
		default: break;
		}
		if (RaiseSound && GetMesh())
		{
			UGameplayStatics::PlaySoundAtLocation(this, RaiseSound, GetMesh()->GetComponentLocation());
		}

		ActiveWeaponActor = GetWeaponBySocket(TargetWeaponSocket);
		OnWeaponUpdated_Implementation(ActiveWeaponActor);
		CurrentWeaponSocket = TargetWeaponSocket;
		TargetWeaponSocket = EWeaponSocketEnum::NONE;
		return;
	}

	if (NotifyName == FName("CheckTargetSocket"))
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] CheckTargetSocket on %s TargetWeaponSocket=%d CurrentWeaponSocket=%d HasAuthority=%d LocallyControlled=%d"),
			*GetName(), (int32)TargetWeaponSocket, (int32)CurrentWeaponSocket, HasAuthority(), IsLocallyControlled());
		switch (TargetWeaponSocket)
		{
		case EWeaponSocketEnum::PRIMARY:
			// Coming from the hip-holstered Secondary, the hand isn't already near the back socket, so
			// Primary needs its own draw montage. Primary<->Special swaps skip this (see CheckFastSwap).
			if (CurrentWeaponSocket == EWeaponSocketEnum::SECONDARY)
			{
				UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] CheckTargetSocket: TargetWeaponSocket=PRIMARY and CurrentWeaponSocket=SECONDARY, playing PrimaryDrawMontage"));
				PlayMontageReplicated(PrimaryDrawMontage, 1.0f);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] CheckTargetSocket: TargetWeaponSocket=PRIMARY but CurrentWeaponSocket=%d (not SECONDARY), skipping draw montage"), (int32)CurrentWeaponSocket);
			}
			break;
		case EWeaponSocketEnum::SECONDARY:
			PlayMontageReplicated(SecondaryDrawMontage, 1.0f);
			break;
		case EWeaponSocketEnum::SPECIAL:
			// Special is back-mounted like Primary, so by design it reuses the Primary draw montage.
			PlayMontageReplicated(PrimaryDrawMontage, 1.0f);
			break;
		default:
			break;
		}
		return;
	}

	if (NotifyName == FName("CheckFastSwap"))
	{
		// Gate only: holstering Primary/Special already moves the hand behind the back, so swapping
		// between the two can fast-grab from there instead of playing a full draw montage.
		return;
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
	if (IsAlive_Implementation())
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

	if (CurrentHP >= MaxHP || !IsAlive_Implementation())
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

	// Heal amount per tick (0.2 seconds) -> -1 because we are "dealing" negative damage
	float RegenAmountPerTick = -1 * RegenRatePerSecond * MaxHP * 0.2f;

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
	HP -= Value;

	if (!IsAlive_Implementation())
	{
		HP = 0;
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

void ABaseUnit::NotifyDeath()
{
	//enable ragdoll
	ReplicatedRagdollStart();
	//MulticastNotifyRagdoll(true);

	GetCharacterMovement()->DisableMovement();

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//AimAction_Implementation(false);
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
	if (ActiveWeaponActor)
	{
		IIWeapon::Execute_Trigger(ActiveWeaponActor, false);
	}
	else
	{
		// Diagnostic: a unit should always hold a weapon here (HandleWeaponStatesOnPlayerDepossessed
		// below still turns every weapon's trigger off)
		UE_LOG(LogTemp, Warning, TEXT("ABaseUnit::NotifyDeath - %s died without ActiveWeaponActor (CurrentWeaponSocket=%d, TargetWeaponSocket=%d, Controller=%s)"),
			*GetName(), (int32)CurrentWeaponSocket, (int32)TargetWeaponSocket,
			GetController() ? (GetController()->IsPlayerController() ? TEXT("Player") : TEXT("AI")) : TEXT("None"));
	}
	BackupAIController->OrdersManagerComponent->SetStopOrder();

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

	HandleWeaponStatesOnPlayerDepossessed();

	BP_NotifyDeath();
	GetWorldTimerManager().SetTimer(UnitDiedTimer, this,
		&ABaseUnit::HideActorOnDeath, 3.0f, false);
}


void ABaseUnit::HandleWeaponStatesOnPlayerDepossessed()
{
	RecoilAnimationComponent->Deactivate();
	HipFire = false;
	TriggerActive = false;
	AimAction_Implementation(false);
	AimActionCompleted_Implementation();
	for (const auto& KeyValue : WeaponArray)
	{
		if (KeyValue)
		{
			IIWeapon::Execute_Trigger(KeyValue, false);
			IIWeapon::Execute_SetSightMeshScale(KeyValue, false);
		}
	}
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
	UnitCombatData = UnitPrefab.UnitCombatData;
}

void ABaseUnit::ConfigureWeapon_Implementation(ABaseWeapon* Weapon, EWeaponSocketEnum SocketEnum)
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponDebug] ConfigureWeapon_Implementation on %s Weapon=%s SocketEnum=%d ActiveWeaponActor(before)=%s CurrentWeaponSocket(before)=%d"),
		*GetName(), *GetNameSafe(Weapon), (int32)SocketEnum, *GetNameSafe(ActiveWeaponActor), (int32)CurrentWeaponSocket);
	WeaponArray[(uint8)SocketEnum] = Weapon;
	if (ActiveWeaponActor == nullptr)
	{
		ActiveWeaponActor = Weapon;
		CurrentWeaponSocket = SocketEnum;
		EALSOverlayState NewState = EALSOverlayState::Default;
		switch (SocketEnum)
		{
		case EWeaponSocketEnum::PRIMARY:
			NewState = EALSOverlayState::Rifle;
			break;
		case EWeaponSocketEnum::SECONDARY:
			NewState = EALSOverlayState::PistolTwoHanded;
			break;
		case EWeaponSocketEnum::SPECIAL:
			NewState = EALSOverlayState::RPG;
			break;
		}
		Server_SetOverlayState(NewState, false);
	}
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
	HP = MaxHP;
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

	// A unit that died during a weapon switch can be left without an active weapon on the server
	// (the switch clears it and assigns the new weapon only when the switch finishes).
	// Give it back the weapon of the socket it holds, or the first weapon it has.
	if (HasAuthority() && !ActiveWeaponActor)
	{
		const int32 SocketIndex = (int32)CurrentWeaponSocket;
		if (WeaponArray.IsValidIndex(SocketIndex) && WeaponArray[SocketIndex])
		{
			ActiveWeaponActor = WeaponArray[SocketIndex];
		}
		else
		{
			for (ABaseWeapon* Weapon : WeaponArray)
			{
				if (Weapon)
				{
					ActiveWeaponActor = Weapon;
					break;
				}
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("ABaseUnit::RespawnUnit - %s had no ActiveWeaponActor, restored %s"),
			*GetName(), *GetNameSafe(ActiveWeaponActor));
	}

	SetActorTransform(RespawnTransform);
	GetWorldTimerManager().SetTimer(UnitDiedTimer, [this]()
		{
			SetActorHiddenInGame(false);
			HeadComponent->CastShadow = true;
			BP_RespawnUnit();
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

UAISense_Sight::EVisibilityResult ABaseUnit::CanBeSeenFrom(const FCanBeSeenFromContext& Context,
	FVector& OutSeenLocation, int32& OutNumberOfLoSChecksPerformed, int32& OutNumberOfAsyncLosCheckRequested,
	float& OutSightStrength, int32* UserData, const FOnPendingVisibilityQueryProcessedDelegate* Delegate)
{
	uint8 Flags6bit = 0;

	FVector ChestLoc = GetChestLocation_Implementation();
	FVector HeadLoc = GetHeadLocation_Implementation();
	FVector LeftArmLoc = GetLeftArmLocation_Implementation();
	FVector RightArmLoc = GetRightArmLocation_Implementation();
	FVector LeftLegLoc = GetLeftLegLocation_Implementation();
	FVector RightLegLoc = GetRightLegLocation_Implementation();

	TArray<ETargetBodyPart> VisibleBodyParts;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	FCollisionResponseParams ResponseParams;
	QueryParams.AddIgnoredActor(Context.IgnoreActor);
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Context.ObserverLocation, ChestLoc,
		ECollisionChannel::ECC_Visibility, QueryParams);
	if (bHit && HitResult.GetActor() == this)
	{
		VisibleBodyParts.Add(ETargetBodyPart::CHEST);
		Flags6bit |= 1 << 0;
	}
	bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Context.ObserverLocation, HeadLoc,
		ECollisionChannel::ECC_Visibility, QueryParams);
	if (bHit && HitResult.GetActor() == this)
	{
		VisibleBodyParts.Add(ETargetBodyPart::HEAD);
		Flags6bit |= 1 << 1;
	}
	bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Context.ObserverLocation, LeftArmLoc,
		ECollisionChannel::ECC_Visibility, QueryParams);
	if (bHit && HitResult.GetActor() == this)
	{
		VisibleBodyParts.Add(ETargetBodyPart::LEFT_ARM);
		Flags6bit |= 1 << 2;
	}
	bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Context.ObserverLocation, RightArmLoc,
		ECollisionChannel::ECC_Visibility, QueryParams);
	if (bHit && HitResult.GetActor() == this)
	{
		VisibleBodyParts.Add(ETargetBodyPart::RIGHT_ARM);
		Flags6bit |= 1 << 3;
	}
	bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Context.ObserverLocation, LeftLegLoc,
		ECollisionChannel::ECC_Visibility, QueryParams);
	if (bHit && HitResult.GetActor() == this)
	{
		VisibleBodyParts.Add(ETargetBodyPart::LEFT_LEG);
		Flags6bit |= 1 << 4;
	}
	bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Context.ObserverLocation, RightLegLoc,
		ECollisionChannel::ECC_Visibility, QueryParams);
	if (bHit && HitResult.GetActor() == this)
	{
		VisibleBodyParts.Add(ETargetBodyPart::RIGHT_LEG);
		Flags6bit |= 1 << 5;
	}

	if (VisibleBodyParts.Num() > 0)
	{
		float Strength01 = VisibleBodyParts.Num() / 6.0f;

		Flags6bit &= 0b00111111; // 6 bitów

		// Quantize strength to 10 bits
		uint32 StrengthQuantized = FMath::RoundToInt(Strength01 * 1023.0f);

		// Pack bits
		uint32 Packed =
			(StrengthQuantized << 6) |
			Flags6bit;

		// Reinterpret as float
		float Result;
		FMemory::Memcpy(&Result, &Packed, sizeof(float));

		OutSeenLocation = ChestLoc;
		OutNumberOfLoSChecksPerformed += 6;
		OutSightStrength = Result;
		return UAISense_Sight::EVisibilityResult::Visible;
	}

	return UAISense_Sight::EVisibilityResult::NotVisible;
}

void ABaseUnit::HideActorOnDeath()
{
	SetActorHiddenInGame(true);
	HeadComponent->CastShadow = false;
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

//ARMED UNIT INTERFACE
AActor* ABaseUnit::GetWeapon_Implementation()
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
	return ActiveWeaponActor && IIWeapon::Execute_IsReloading(ActiveWeaponActor);
}
void ABaseUnit::GetUnitCombatDataStruct_Implementation(FUnitCombatDataStruct& CombatData)
{
	CombatData = UnitCombatData;
}

//END ARMED UNIT INTERFACE

//ITARGETABBLE INTERAFACE
FVector ABaseUnit::GetHeadLocation_Implementation()
{
	return GetMesh()->GetBoneLocation(FName("head"), EBoneSpaces::WorldSpace);
}

FVector ABaseUnit::GetChestLocation_Implementation()
{
	return GetMesh()->GetBoneLocation(FName("spine_03"), EBoneSpaces::WorldSpace);
}

FVector ABaseUnit::GetLeftArmLocation_Implementation()
{
	return GetMesh()->GetBoneLocation(FName("spine_03"), EBoneSpaces::WorldSpace);
}

FVector ABaseUnit::GetRightArmLocation_Implementation()
{
	return GetMesh()->GetBoneLocation(FName("spine_03"), EBoneSpaces::WorldSpace);
}

FVector ABaseUnit::GetLeftLegLocation_Implementation()
{
	return GetMesh()->GetBoneLocation(FName("spine_03"), EBoneSpaces::WorldSpace);
}

FVector ABaseUnit::GetRightLegLocation_Implementation()
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
void ABaseUnit::ScopeZeroUpAction_Implementation()
{
	// On foot zero is only adjusted while the sight is not being aimed through.
	// A seated gunner always looks through the weapon's ADS, so there it works any time.
	const bool bSeatedGunner = StationaryRole == EALSStationaryRole::Gunner || StationaryRole == EALSStationaryRole::Gunner_Standing;
	if (ActiveWeaponActor && (bSeatedGunner || (StationaryRole == EALSStationaryRole::None && !UsingADS)))
	{
		IIWeapon::Execute_ChangeSightZero(ActiveWeaponActor, 1);
	}
}

void ABaseUnit::ScopeZeroDownAction_Implementation()
{
	const bool bSeatedGunner = StationaryRole == EALSStationaryRole::Gunner || StationaryRole == EALSStationaryRole::Gunner_Standing;
	if (ActiveWeaponActor && (bSeatedGunner || (StationaryRole == EALSStationaryRole::None && !UsingADS)))
	{
		IIWeapon::Execute_ChangeSightZero(ActiveWeaponActor, -1);
	}
}

//VEHICLE SEAT
bool ABaseUnit::AddGunnerAimInput(float YawInput, float PitchInput)
{
	UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(OccupiedSeat);
	if (!GunnerSeat)
	{
		return false;
	}

	GunnerSeat->AddAimInput(YawInput, PitchInput);

	// The server already has the new aim when the gunner plays on it. Other machines send it at a fixed
	// rate instead of once per input event, the timer also delivers the last value after the mouse stops.
	if (!HasAuthority() && !GetWorldTimerManager().IsTimerActive(GunnerAimSendTimer))
	{
		constexpr float GunnerAimSendInterval = 1.0f / 30.0f;
		GetWorldTimerManager().SetTimer(GunnerAimSendTimer, this, &ABaseUnit::SendGunnerAim, GunnerAimSendInterval, false);
	}
	return true;
}

void ABaseUnit::SendGunnerAim()
{
	if (const UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(OccupiedSeat))
	{
		Server_SetGunnerAim(GunnerSeat->GetAim());
	}
}

void ABaseUnit::Server_SetGunnerAim_Implementation(FVector2D Aim)
{
	if (UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(OccupiedSeat))
	{
		GunnerSeat->SetAimFromClient(Aim);
	}
}

void ABaseUnit::GunnerCameraUpInput(float Value)
{
	if (!AddGunnerAimInput(0.0f, Value))
	{
		Super::GunnerCameraUpInput(Value);
	}
}

void ABaseUnit::GunnerCameraRightInput(float Value)
{
	if (!AddGunnerAimInput(Value, 0.0f))
	{
		Super::GunnerCameraRightInput(Value);
	}
}


void ABaseUnit::ZoomAction_Implementation()
{
	// Zoom now lives entirely in ScrollAction_Implementation (see CycleZoomAction) - left empty rather than
	// deleted since IALSInputInterface::ZoomAction is a BlueprintNativeEvent every unit must implement.
}

void ABaseUnit::ScrollAction_Implementation(bool Value)
{
	// Scroll drives zoom (also for a handheld weapon's own BaseSight, not just a seated gunner), instead of
	// the base ScrollAction's continuous CameraFOV stepping (Gunner) or debug overlay cycling (everyone else)
	// - the FOV stepping fought over the same CameraFOV value.
	// Value is scroll-down (see AALSBaseCharacter::ScrollAction_Implementation's own comment).
	const bool bHasZoomableWeapon = StationaryRole == EALSStationaryRole::None && ActiveWeaponActor
		&& ActiveWeaponActor->GetClass()->ImplementsInterface(UIWeapon::StaticClass());
	if (StationaryRole == EALSStationaryRole::Gunner || StationaryRole == EALSStationaryRole::Gunner_Standing || bHasZoomableWeapon)
	{
		// The wheel can report several deltas for what felt like one notch to the player - without this a
		// single scroll could skip multiple zoom levels (or jump straight to the last one) instead of one step.
		constexpr float ScrollZoomCooldown = 0.15f;
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastScrollZoomTime < ScrollZoomCooldown)
		{
			return;
		}
		LastScrollZoomTime = Now;
		// Flipped from the naive !Value=up mapping: that had scroll up zooming out, backwards from what felt
		// right. Scroll clamps at the ends (bWrap false) instead of revolving back around the array.
		CycleZoomAction(Value ? 1 : -1, false);
		return;
	}
	Super::ScrollAction_Implementation(Value);
}

void ABaseUnit::CycleZoomAction(int32 Direction, bool bWrap)
{
	if (StationaryRole == EALSStationaryRole::Gunner)
	{
		// Enclosed gunner: zoom is the seat's own optic (periscope/sight glass), not the weapon's
		if (UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(OccupiedSeat))
		{
			if (GunnerSeat->CycleZoom(Direction, bWrap))
			{
				CameraFOV = GunnerSeat->GetCurrentZoomFOV();
			}
		}
		return;
	}
	if (!ActiveWeaponActor || (StationaryRole != EALSStationaryRole::None && StationaryRole != EALSStationaryRole::Gunner_Standing))
	{
		return;
	}
	// The weapon reports every step of the zoom transition through RefreshSightZoom
	IIWeapon::Execute_ChangeSightZoom(ActiveWeaponActor, Direction, bWrap);
}

void ABaseUnit::JumpAction_Implementation(bool bValue)
{
	if (StationaryRole == EALSStationaryRole::Gunner || StationaryRole == EALSStationaryRole::Gunner_Standing)
	{
		if (const UGunnerSeatComponent* GunnerSeat = Cast<UGunnerSeatComponent>(OccupiedSeat))
		{
			if (AActor* Coax = GunnerSeat->GetCoaxWeapon())
			{
				IIWeapon::Execute_Trigger(Coax, bValue);
				return;
			}
		}
	}
	Super::JumpAction_Implementation(bValue);
}

void ABaseUnit::RefreshSightZoom_Implementation(AActor* Weapon)
{
	// Only while looking through that scope, otherwise the zoom is applied on the next aim
	if (!Weapon || Weapon != ActiveWeaponActor || !UsingADS || !IsLocallyControlled() || !IIWeapon::Execute_GetIsScoped(Weapon))
	{
		return;
	}
	CameraFOV = IIWeapon::Execute_GetWeaponFOV(Weapon);
	IIWeapon::Execute_SetSightMeshScale(Weapon, true);

	SetAimLookRate(IIWeapon::Execute_GetCameraSensitivity(Weapon));
}

void ABaseUnit::SetBodyHiddenForScope_Implementation(bool bHideBody)
{
	if (bHideBody == bBodyHiddenForScope)
	{
		return;
	}

	if (bHideBody)
	{
		// Only the player looking through the scope should lose the body
		if (!IsLocallyControlled() || !IsPlayerControlled())
		{
			return;
		}
		bBodyHiddenForScope = true;

		// Weapon is attached to the hand, so bones have to be refreshed even when the mesh is not rendered
		ScopeSavedAnimTickOption = GetMesh()->VisibilityBasedAnimTickOption;
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

		TInlineComponentArray<UMeshComponent*> MeshComponents(this);
		for (UMeshComponent* MeshComponent : MeshComponents)
		{
			if (MeshComponent && !MeshComponent->bOwnerNoSee)
			{
				MeshComponent->SetOwnerNoSee(true);
				ScopeHiddenComponents.Add(MeshComponent);
			}
		}
	}
	else
	{
		bBodyHiddenForScope = false;
		GetMesh()->VisibilityBasedAnimTickOption = ScopeSavedAnimTickOption;

		for (const TWeakObjectPtr<UPrimitiveComponent>& Component : ScopeHiddenComponents)
		{
			if (Component.IsValid())
			{
				Component->SetOwnerNoSee(false);
			}
		}
		ScopeHiddenComponents.Reset();
	}
}
