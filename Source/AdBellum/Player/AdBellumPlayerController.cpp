// Fill out your copyright notice in the Description page of Project Settings.


#include "AdBellumPlayerController.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Interfaces/Selectable.h"
#include "OrderSystem/OrdersManager.h"
#include "RTSPlayer.h"
#include "GameFramework/HUD.h"
#include "Character/ALSBaseCharacter.h"
#include "OrderSystem/Orders/GeneralOrders.h"
#include "IAreaOfInterest.h"
#include "Net/UnrealNetwork.h"
#include "Unit/BaseUnit.h"
#include "Weapon/BaseWeapon.h"
#include "System/AdBellumGameState.h"
#include "System/AdBellumGameMode.h"
#include "Vehicle/BaseVehicle.h"
#include "CharacterHUD.h"
#include "Formation/BaseFormation.h"
#include "Trainer/TrainingHelper.h"
#include "Player/ReplicationReporter.h"


AAdBellumPlayerController::AAdBellumPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void AAdBellumPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() < ENetMode::NM_Client) // Every NetMode lower than NM_Client is either Standalone, Dedicated Server or Listen Server
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = this;
		SpawnInfo.Instigator = GetInstigator();
		SpawnInfo.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map
	}
}

void AAdBellumPlayerController::SpawnRTSCamera_Implementation(FTransform SpawnTransform)
{
	// Moved RTS camera spawn to GameMode to centralize server-owned spawning and
	// batch replication. Forward request to GameMode implementation.
	if (HasAuthority())
	{
		AAdBellumGameMode* GM = Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->CreateRTSCameraForPlayer(this, SpawnTransform);
		}
	}
}

void AAdBellumPlayerController::NotifyActorReplicated_Implementation(AActor* ReplicatedActor)
{
	ServerNotifyActorReplicated(ReplicatedActor);
}

void AAdBellumPlayerController::SetSelectionCircle_Implementation(AActor* SelectedActor, bool IsVisible)
{
	Client_SetSelectionCircle(SelectedActor, IsVisible);
}

void AAdBellumPlayerController::Client_SetSelectionCircle_Implementation(AActor* SelectedActor, bool IsVisible)
{
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	if (GameState)
	{
		GameState->SetSelectionCircle(false, SelectedActor);
	}
}

void AAdBellumPlayerController::ServerNotifyActorReplicated_Implementation(AActor* ReplicatedActor)
{
	FUniqueNetIdPtr UserId = PlayerState->GetUniqueId().GetUniqueNetId();
	if (UserId.IsValid())
	{
		Cast<AAdBellumGameMode>(UGameplayStatics::GetGameMode(this))->NotifyActorReplicated(UserId->ToString(), ReplicatedActor);
	}
}

void AAdBellumPlayerController::Client_SetPlayerCameraManager_Implementation(FQuat NewRotation)
{
	PlayerCameraManager = Cast<AALSPlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0));
}

void AAdBellumPlayerController::OnRep_RTSPlayerSpawned()
{
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnRep_RTSPlayerSpawned: RTSCameraPawn=%s"), RTSCameraPawn ? TEXT("valid") : TEXT("NULL"));
	if (RTSCameraPawn)
	{
		RTSCameraPawn->OrderActionDelegate.BindUObject(this, &AAdBellumPlayerController::RightMouseButtonPressed);
		RTSCameraPawn->InteractionActionDelegate.BindUObject(this, &AAdBellumPlayerController::SelectTargetForOrder);
		SpawnRTSHud();
	}
}

void AAdBellumPlayerController::SpawnRTSHud()
{
	FActorSpawnParameters SpawnInfoRTSHUD;
	SpawnInfoRTSHUD.Owner = this;
	SpawnInfoRTSHUD.Instigator = GetInstigator();
	SpawnInfoRTSHUD.ObjectFlags |= RF_Transient;	// We never want to save HUDs into a map
	RTSHUD = GetWorld()->SpawnActor<ARTS_HUD>(RTSHUDClass, SpawnInfoRTSHUD);
	RTSCameraPawn->HUD = RTSHUD;
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SpawnRTSHud: RTSHUD=%s WidgetReadyRightAfterSpawn=%d"),
		RTSHUD ? TEXT("valid") : TEXT("NULL"), RTSHUD ? RTSHUD->IsFormationWidgetReady() : false);
}

void AAdBellumPlayerController::SpawnCharacterHud()
{
	FActorSpawnParameters SpawnInfoCharacterHUD;
	SpawnInfoCharacterHUD.Owner = this;
	SpawnInfoCharacterHUD.Instigator = GetInstigator();
	SpawnInfoCharacterHUD.ObjectFlags |= RF_Transient;	// We never want to save HUDs into a map
	CharacterHUD = GetWorld()->SpawnActor<ACharacterHUD>(CharacterHUDClass, SpawnInfoCharacterHUD);
}

void AAdBellumPlayerController::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAdBellumPlayerController, PlayerIndex);
	DOREPLIFETIME(AAdBellumPlayerController, TeamIndex);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, OwnedFormations, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, RTSCameraPawn, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, SelectingOrder, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, SelectionFormation, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, SelectionPawn, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, ReplicationReporterActor, COND_OwnerOnly);
}

void AAdBellumPlayerController::OnPossess(APawn* NewPawn)
{
	if (NewPawn == RTSCameraPawn)
	{
		RTSCameraPawn->TeleportAboveUnit(PossessedCharacter);
	}
	Super::OnPossess(NewPawn);
}

void AAdBellumPlayerController::AcknowledgePossession(APawn* NewPawn)
{
	Client_AcknowledgePossession(NewPawn);
	Super::AcknowledgePossession(NewPawn);
}

void AAdBellumPlayerController::Client_AcknowledgePossession_Implementation(APawn* NewPawn)
{
	if (RTSHUD == nullptr)
	{
		//SpawnRTSHud();
	}

	if (CharacterHUD == nullptr)
	{
		SpawnCharacterHud();
	}

	if (NewPawn == RTSCameraPawn)
	{
		MyHUD = RTSHUD;

		if (CharacterHUD)
		{
			CharacterHUD->HUDClose();
		}

		RTSHUD->HUDOpen(NewPawn);

		// RTS:
		// - Cursor visible
		// - UMG can receive mouse input
		// - World/PlayerController can still receive input
		//   when no interactive widget consumes it.
		SetShowMouseCursor(true);

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		SetInputMode(InputMode);

		IsSelectingOrder = false;
	}
	else if (ABaseUnit* ALSPawn = Cast<ABaseUnit>(NewPawn))
	{
		MyHUD = CharacterHUD;

		if (RTSHUD)
		{
			RTSHUD->HUDClose();
		}

		CharacterHUD->HUDOpen(NewPawn);

		// FPS:
		// - Hide cursor
		// - Capture game input
		SetShowMouseCursor(false);

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);

		SetIgnoreLookInput(false);
		SetIgnoreMoveInput(false);

		IsSelectingOrder = false;
	}
	else
	{
		// Unknown pawn
	}
}

void AAdBellumPlayerController::PossessAction(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		ServerPossessAction();
	}
}

void AAdBellumPlayerController::ServerPossessAction_Implementation()
{
	if (PossessedCharacter == RTSCameraPawn)
	{
		if (SelectionPawn)
		{
			ClientClearSelection();
			Possess(SelectionPawn);
		}
		else if (SelectionFormation)
		{
			if (APawn* UnitToPossess = SelectionFormation->GetUnitForPossesion())
			{
				ClientClearSelection();
				// what if vehicle is selected?
				Possess(UnitToPossess);
			}
		}
	}
	else
	{
		switch (ISelectable::Execute_GetUnitType(PossessedCharacter)) 
		{
		case 0:
			Cast<ABaseUnit>(PossessedCharacter)->PossessByAIController();
			Possess(RTSCameraPawn);
			break;
		case 1:
			Cast<ABaseVehicle>(PossessedCharacter)->PossessByAIController();
			Possess(RTSCameraPawn);
			break;
		}
	}
}

void AAdBellumPlayerController::ClientClearSelection_Implementation()
{
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	if (SelectionFormation)
	{
		IFormationInterface::Execute_SetSelection(SelectionFormation, false);
	}
}

void AAdBellumPlayerController::CameraFreeRotateAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_CameraFreeRotateAction(PossessedCharacter, Value.Get<bool>());
	}
}

void AAdBellumPlayerController::TriggerAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		if (!IsSelectingOrder)
		{
			IALSInputInterface::Execute_TriggerAction(PossessedCharacter, Value.Get<bool>());
		}
		else
		{
			IsSelectingOrder = false;
			FVector MouseLocation = GetMousePositionInWorld();
			//Server_TriggerAction(MouseLocation);
		}
	}
}

void AAdBellumPlayerController::Server_TriggerAction_Implementation(FVector MouseLocation)
{
	switch (SelectingOrder)
	{
	case OrderEnum::Attack:
		FindSuitableOrder();
		break;
	case OrderEnum::HoldPosition:
		PerformOrder<OrderEnum::HoldPosition>(nullptr, FVector::ZeroVector);
		break;
	case OrderEnum::Stop:
		PerformOrder<OrderEnum::Stop>(nullptr, FVector::ZeroVector);
		break;
	case OrderEnum::Move:
	{
		AActor* FoundActor = GetActorFromMousePosition();
		MoveSuitableOrder(FoundActor, MouseLocation);
		break;
	}
	case OrderEnum::Patrol:
		PerformOrder<OrderEnum::Patrol>(nullptr, MouseLocation);
		break;
	}
}

void AAdBellumPlayerController::PrimarySelectionAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_PrimarySelectionAction(PossessedCharacter);
		CharacterHUD->NotifyWeapon();
	}
}

void AAdBellumPlayerController::SecondarySelectionAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_SecondarySelectionAction(PossessedCharacter);	
		CharacterHUD->NotifyWeapon();
	}
}

void AAdBellumPlayerController::ThirdSelectionAction(const FInputActionValue& Value)
{
	if (PossessedCharacter && Value.Get<bool>())
	{
		IALSInputInterface::Execute_ThirdSelectionAction(PossessedCharacter);
		CharacterHUD->NotifyWeapon();
	}
}

void AAdBellumPlayerController::OrderStopAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_OrderStopAction(PossessedCharacter, Value.Get<bool>());
	}
}

void AAdBellumPlayerController::OrderPatrolAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_OrderPatrolAction(PossessedCharacter, Value.Get<bool>());
	}
}

void AAdBellumPlayerController::OrderMoveAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_OrderMoveAction(PossessedCharacter, Value.Get<bool>());
	}
}

void AAdBellumPlayerController::OrderHoldPositionAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_OrderHoldPositionAction(PossessedCharacter, Value.Get<bool>());
	}
}

void AAdBellumPlayerController::OrderAttackAction(const FInputActionValue& Value)
{
	if (PossessedCharacter)
	{
		IALSInputInterface::Execute_OrderAttackAction(PossessedCharacter, Value.Get<bool>());
	}
}

int AAdBellumPlayerController::GetPlayerIndex_Implementation()
{
	return PlayerIndex;
}

void AAdBellumPlayerController::SetPlayerIndex_Implementation(int index)
{
	PlayerIndex = index;
}

int AAdBellumPlayerController::GetTeamIndex_Implementation()
{
	return TeamIndex;
}

void AAdBellumPlayerController::SetTeamIndex_Implementation(int index)
{
	TeamIndex = index;
}

void AAdBellumPlayerController::PossessCharacter_Implementation(APawn* Unit)
{
	Possess(Unit);
}

void AAdBellumPlayerController::UnpossessCharacter_Implementation()
{
	Possess(RTSCameraPawn);
}

void AAdBellumPlayerController::RemoveSelectedUnit_Implementation(APawn* Unit)
{
	//SelectedPawns.Remove(Unit);
}

void AAdBellumPlayerController::SetSelectedFormations_Implementation(const TArray<ABaseFormation*> & SelectedFormations)
{
	if (!SelectedFormations.IsEmpty())
	{
		SelectionFormation = SelectedFormations[0];
	}
	else 
	{
		SelectionFormation = nullptr;
	}
}

void AAdBellumPlayerController::SetSelectedPawn_Implementation(APawn* InSelectedPawn)
{
	SelectionPawn = InSelectedPawn;
}


void AAdBellumPlayerController::NotifyHUDRole_Implementation(EALSStationaryRole StationaryRole, AActor* ControlledActor)
{
	Client_NotifyHUDRole(StationaryRole, ControlledActor);
}

AActor* AAdBellumPlayerController::GetPlayerStateActor_Implementation()
{
	return PlayerState;
}

void AAdBellumPlayerController::GameEnded_Implementation(MatchResult Result)
{
	Client_GameEnded(Result);
}

void AAdBellumPlayerController::NotifyCharacterHUD_Implementation(FNotifyHUDData NotifyData)
{
	Client_OnHUDNotify(NotifyData);
}

void AAdBellumPlayerController::UpdateCameraDamageEffects_Implementation(float HPRatio)
{
	Client_UpdateCameraDamageEffects(HPRatio);
}

void AAdBellumPlayerController::Client_UpdateCameraDamageEffects_Implementation(float HPRatio)
{
	PlayerCameraManager->UpdateDamageEffect(HPRatio);
}

void AAdBellumPlayerController::InitializeRTSHUD_Implementation(ABaseFormation* Formation)
{
	TArray<ABaseFormation*> Formations;
	Formations.Add(Formation);
	RTSHUD->InitializeWidget(Formations);
}

void AAdBellumPlayerController::OnRep_OwnedFormations()
{
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnRep_OwnedFormations: OwnedFormations.Num()=%d RTSHUD=%s WidgetReady=%d"),
		OwnedFormations.Num(), RTSHUD ? TEXT("valid") : TEXT("NULL"), RTSHUD ? RTSHUD->IsFormationWidgetReady() : false);
	// When the OwnedFormations array finishes replicating to the client,
	// ensure the RTS HUD exists AND its widget has been created (via HUDOpen)
	// before initializing it with the replicated array.
	// Start a short repeating timer to retry until both are ready.
	if (RTSHUD == nullptr || !RTSHUD->IsFormationWidgetReady())
	{
		if (!GetWorldTimerManager().IsTimerActive(RTSSpawnTimerHandle))
		{
			UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnRep_OwnedFormations: starting retry timer"));
			GetWorldTimerManager().SetTimer(RTSSpawnTimerHandle, this, &AAdBellumPlayerController::TryInitializeRTSHUD, 0.1f, true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DEBUG] OnRep_OwnedFormations: calling InitializeWidget directly"));
		RTSHUD->InitializeWidget(OwnedFormations);
	}
}

void AAdBellumPlayerController::TryInitializeRTSHUD()
{
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] TryInitializeRTSHUD tick: RTSHUD=%s WidgetReady=%d"),
		RTSHUD ? TEXT("valid") : TEXT("NULL"), RTSHUD ? RTSHUD->IsFormationWidgetReady() : false);
	if (RTSHUD && RTSHUD->IsFormationWidgetReady())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DEBUG] TryInitializeRTSHUD: calling InitializeWidget, clearing timer"));
		RTSHUD->InitializeWidget(OwnedFormations);
		GetWorldTimerManager().ClearTimer(RTSSpawnTimerHandle);
	}
}

TArray<ABaseFormation*> AAdBellumPlayerController::GetOwnedFormations_Implementation()
{
	return OwnedFormations;
}

void AAdBellumPlayerController::SetOwnedFormations_Implementation(TArray<ABaseFormation*>& Formations)
{
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetOwnedFormations_Implementation: Formations.Num()=%d IsLocalController=%d"),
		Formations.Num(), IsLocalController());
	OwnedFormations = Formations;

	// OnRep_OwnedFormations (which initializes the RTS HUD with the owned
	// formations) is only ever invoked by replication landing on a remote
	// proxy - it never fires for a locally-controlled owner, since there is
	// no separate proxy to replicate to. Without this, the host's RTS HUD
	// would never get initialized at all, even though the client (a genuine
	// remote proxy) works correctly.
	if (IsLocalController())
	{
		OnRep_OwnedFormations();
	}
}

void AAdBellumPlayerController::Client_OnHUDNotify_Implementation(FNotifyHUDData NotifyData)
{
	if (CharacterHUD == MyHUD)
	{
		switch (NotifyData.NotifyType)
		{
		case ENotifyHudType::WeaponShot:
			CharacterHUD->NotifyWeaponShot();
			break;
		}
	}
}

void AAdBellumPlayerController::Client_GameEnded_Implementation(MatchResult Result)
{
	BP_GameEnded(Result);
}

void AAdBellumPlayerController::Client_NotifyHUDRole_Implementation(EALSStationaryRole StationaryRole, AActor* ControlledActor)
 {
	if (CharacterHUD == MyHUD) 
	{
		if (GetPawn() == ControlledActor)
		{
			CharacterHUD->NotifyRole(StationaryRole);
		}
	}
 }

void AAdBellumPlayerController::OnRep_ReplicationReporter()
{
	if (ReplicationReporterActor)
	{
		ReplicationReporterActor->InitializeReporter(this);
	}
}

template<OrderEnum Type>
void AAdBellumPlayerController::PerformOrder(AActor* TargetUnit, FVector TargetPosition)
{
	if (SelectionFormation)
	{
		SelectionFormation->PerformOrder<Type>(TargetUnit, TargetPosition);
	}
}

FVector AAdBellumPlayerController::GetMousePositionInWorld()
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
	FHitResult HitResult;
	GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult);
	return HitResult.Location;
}

AActor* AAdBellumPlayerController::GetActorFromMousePosition()
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Vehicle));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_GameTraceChannel6)); //AreaOfIntrestChannel
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	FHitResult HitResult;
	GetHitResultUnderCursorForObjects(ObjectTypes, true, HitResult);
	return HitResult.GetActor();
}

void AAdBellumPlayerController::RightMouseButtonPressed(bool bEnabled)
{
	if (bEnabled) 
	{
		if (!IsSelectingOrder)
		{
			FindSuitableOrder();
		}
		else
		{
			IsSelectingOrder = false;
		}
	}
}

void AAdBellumPlayerController::FindSuitableOrder()
{
	AActor* FoundActor = GetActorFromMousePosition();
	FVector MouseLocation = GetMousePositionInWorld();
	Server_FindSuitableOrder(FoundActor, MouseLocation);
}

void AAdBellumPlayerController::Server_FindSuitableOrder_Implementation(AActor* FoundActor, FVector MouseLocation)
{
	if (FoundActor && FoundActor->GetClass()->ImplementsInterface(USelectable::StaticClass()))
	{
		PerformSmartOrder(FoundActor);
	}
	else
	{
		PerformOrder<OrderEnum::Move>(nullptr, MouseLocation);
	}
}

void AAdBellumPlayerController::MoveSuitableOrder(AActor* FoundActor, FVector MouseLocation)
{
	if (FoundActor && FoundActor->GetClass()->ImplementsInterface(USelectable::StaticClass()))
	{
		PerformOrder<OrderEnum::Follow>(FoundActor, FVector::ZeroVector);
	}
	else
	{
		PerformOrder<OrderEnum::Move>(nullptr, MouseLocation);
	}
}

void AAdBellumPlayerController::SelectTargetForOrder(OrderEnum OrderType)
{
	if (OrderType == OrderEnum::Interact)
	{

	}
	else if(!IsSelectingOrder)
	{
		IsSelectingOrder = true;
		SelectingOrder = OrderType;
	}
}

void AAdBellumPlayerController::Client_SetUnitPrefab_Implementation(const TArray<AActor*>& Units, const TArray<FMeshCreatorPrefabStruct>& Prefabs)
{
	ConfigureUnitPrefabsWithRetry(Units, Prefabs);
}

void AAdBellumPlayerController::ConfigureUnitPrefabsWithRetry(TArray<AActor*> Units, TArray<FMeshCreatorPrefabStruct> Prefabs, int32 Attempts)
{
	TArray<AActor*> PendingUnits;
	TArray<FMeshCreatorPrefabStruct> PendingPrefabs;

	for (int i = 0; i < Units.Num(); ++i)
	{
		if (!Units[i])
		{
			PendingUnits.Add(Units[i]);
			PendingPrefabs.Add(Prefabs[i]);
			continue;
		}
		ICustomizable::Execute_ConfigureUnit(Units[i], Prefabs[i]);
	}

	static constexpr int32 MaxAttempts = 50; // ~5 seconds at 0.1s per retry
	if (!PendingUnits.IsEmpty())
	{
		if (Attempts >= MaxAttempts)
		{
			return;
		}
		TWeakObjectPtr<AAdBellumPlayerController> WeakThis(this);
		FTimerDelegate Delegate = FTimerDelegate::CreateLambda([WeakThis, PendingUnits, PendingPrefabs, Attempts]()
		{
			if (AAdBellumPlayerController* StrongThis = WeakThis.Get())
			{
				StrongThis->ConfigureUnitPrefabsWithRetry(PendingUnits, PendingPrefabs, Attempts + 1);
			}
		});
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, Delegate, 0.1f, false);
	}
}

void AAdBellumPlayerController::Client_OnWeaponCreated_Implementation(const TArray<AActor*>& Weapons, const TArray<FUnitWeaponDataStruct>& WeaponPrefabs)
{
	ConfigureWeaponsWithRetry(Weapons, WeaponPrefabs);
}

void AAdBellumPlayerController::ConfigureWeaponsWithRetry(TArray<AActor*> Weapons, TArray<FUnitWeaponDataStruct> WeaponPrefabs, int32 Attempts)
{
	TArray<AActor*> PendingWeapons;
	TArray<FUnitWeaponDataStruct> PendingPrefabs;

	for (int i = 0; i < Weapons.Num(); ++i)
	{
		if (!Weapons[i] || !WeaponPrefabs[i].OwningUnit)
		{
			PendingWeapons.Add(Weapons[i]);
			PendingPrefabs.Add(WeaponPrefabs[i]);
			continue;
		}
		ICustomizable::Execute_ConfigureWeapon(WeaponPrefabs[i].OwningUnit, Cast<ABaseWeapon>(Weapons[i]), WeaponPrefabs[i].Weapon.WeaponSocketType);
		IIWeapon::Execute_ConfigureWeapon(Weapons[i], WeaponPrefabs[i].Weapon.WeaponCustomizationData);
	}

	static constexpr int32 MaxAttempts = 50; // ~5 seconds at 0.1s per retry
	if (!PendingWeapons.IsEmpty())
	{
		if (Attempts >= MaxAttempts)
		{
			return;
		}
		TWeakObjectPtr<AAdBellumPlayerController> WeakThis(this);
		FTimerDelegate Delegate = FTimerDelegate::CreateLambda([WeakThis, PendingWeapons, PendingPrefabs, Attempts]()
		{
			if (AAdBellumPlayerController* StrongThis = WeakThis.Get())
			{
				StrongThis->ConfigureWeaponsWithRetry(PendingWeapons, PendingPrefabs, Attempts + 1);
			}
		});
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, Delegate, 0.1f, false);
	}
}

void AAdBellumPlayerController::PerformSmartOrder(AActor* TargetUnit)
{
	if (IsEnemyUnit(TargetUnit))
	{
		PerformOrder<OrderEnum::Attack>(TargetUnit, FVector::ZeroVector);
	}
	else
	{
		if (TargetUnit->GetClass()->ImplementsInterface(UEnterable::StaticClass()))
		{
			PerformOrder<OrderEnum::Enter>(TargetUnit, FVector::ZeroVector);
		}
		else if (TargetUnit->GetClass()->ImplementsInterface(UIAreaOfInterest::StaticClass()))
		{
			PerformOrder<OrderEnum::OccupyAOI>(TargetUnit, FVector::ZeroVector);
		}
		else if (TargetUnit->GetClass()->ImplementsInterface(UTrainingHelper::StaticClass()))
		{
			PerformOrder<OrderEnum::Training>(TargetUnit, FVector::ZeroVector);
		}
		else
		{
			PerformOrder<OrderEnum::Follow>(TargetUnit, FVector::ZeroVector);
		}
	}
}

bool AAdBellumPlayerController::IsEnemyUnit(AActor* TargetUnit)
{
	return TeamIndex != IOwnershipInterface::Execute_GetTeamIndex(TargetUnit);
}
