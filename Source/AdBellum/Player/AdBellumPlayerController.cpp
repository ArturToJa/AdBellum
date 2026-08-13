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
		//SelectionFormation = GetWorld()->SpawnActor<ABaseFormation>(SpawnInfo);
	}
}

void AAdBellumPlayerController::SpawnRTSCamera_Implementation(FTransform SpawnTransform)
{
	FActorSpawnParameters SpawnInfoRTSCamera;
	SpawnInfoRTSCamera.Owner = this;
	SpawnInfoRTSCamera.Instigator = GetInstigator();
	SpawnInfoRTSCamera.ObjectFlags |= RF_Transient;	// We never want to save default player pawns into a map

	FTransform RTSCameraTransform = SpawnTransform;

	RTSCameraPawn = GetWorld()->SpawnActor<ARTSPlayer>(RTSCameraClass, RTSCameraTransform, SpawnInfoRTSCamera);
	if (HasAuthority())
	{
		OnRep_RTSPlayerSpawned();
	}
	Possess(RTSCameraPawn);
	GetWorld()->GetTimerManager().SetTimer(RTSSpawnTimerHandle, [this, SpawnTransform]()
		{
			Client_SetPlayerCameraManager(SpawnTransform.GetRotation());
		}, 1.0f, false);
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
	if (RTSCameraPawn)
	{
		RTSCameraPawn->OrderActionDelegate.BindUObject(this, &AAdBellumPlayerController::RightMouseButtonPressed);
		RTSCameraPawn->InteractionActionDelegate.BindUObject(this, &AAdBellumPlayerController::SelectTargetForOrder);
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
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, RTSCameraPawn, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, SelectingOrder, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AAdBellumPlayerController, SelectionFormation, COND_OwnerOnly);
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
		SpawnRTSHud();
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
		SetShowMouseCursor(true);
	}
	else if (ABaseUnit* ALSPawn = Cast<ABaseUnit>(NewPawn))
	{
		MyHUD = CharacterHUD;
		if(RTSHUD)
		{
			RTSHUD->HUDClose();
		}
		CharacterHUD->HUDOpen(NewPawn);
		SetShowMouseCursor(false);
		IsSelectingOrder = false;
	}
	else
	{
		// error, unknown Pawn being possessed
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
		//if (!SelectedPawns.IsEmpty())
		if (SelectionFormation)
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
	//SelectedPawns = InPawns;
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
	for (int i = 0; i < Units.Num(); ++i)
	{
		ICustomizable::Execute_ConfigureUnit(Units[i], Prefabs[i]);
	}
}

void AAdBellumPlayerController::Client_OnWeaponCreated_Implementation(const TArray<AActor*>& Weapons, const TArray<FUnitWeaponDataStruct>& WeaponPrefabs)
{
	for (int i = 0; i < Weapons.Num(); ++i)
	{
		ICustomizable::Execute_ConfigureWeapon(WeaponPrefabs[i].OwningUnit, Cast<ABaseWeapon>(Weapons[i]), WeaponPrefabs[i].Weapon.WeaponSocketType);
		IIWeapon::Execute_ConfigureWeapon(Weapons[i], WeaponPrefabs[i].Weapon.WeaponCustomizationData);
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
