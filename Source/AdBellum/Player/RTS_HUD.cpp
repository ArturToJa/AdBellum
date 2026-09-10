// Fill out your copyright notice in the Description page of Project Settings.


#include "RTS_HUD.h"
#include "IPlayer.h"
#include "AdBellumPlayerController.h"
#include "System/AdBellumGameState.h"
#include "Interfaces/Selectable.h"
#include "Formation/BaseFormation.h"
#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "UI/RTSFormationUnitTableWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "OrderSystem/OrdersManager.h"
#include "Formation/FormationInterface.h"
#include "Interfaces/MarkerInterface.h"
#include "NavigationUI.h"
#include "Blueprint/SlateBlueprintLibrary.h"


void ARTS_HUD::HUDOpen(AActor* ControlledActor)
{
	OnHUDOpen(ControlledActor);
}

void ARTS_HUD::HUDClose()
{
	OnHUDClose();
}

void ARTS_HUD::DrawHUD()
{
	if (SelectionStarted)
	{
		float LocationX;
		float LocationY;
		GetOwningPlayerController()->GetMousePosition(LocationX, LocationY);
		TArray<APawn*> OutActors;
		TSet<FGuid> OutMarkers;
		DrawRect(SelectionColor, AnchorPoint.X, AnchorPoint.Y, LocationX - AnchorPoint.X, LocationY - AnchorPoint.Y);

		GetActorsInSelectionRectangle<APawn>(AnchorPoint, FVector2D(LocationX, LocationY), OutActors, false, false);
		CheckSelectedFormations(OutActors);


		CheckFormationMarkersInSelection(OutMarkers, FVector2D(LocationX, LocationY));
		CheckSelectedMarkersForFormation(OutMarkers);

	}
}


void ARTS_HUD::SelectionModeStart_Implementation()
{
	float LocationX;
	float LocationY;
	GetOwningPlayerController()->GetMousePosition(LocationX, LocationY);
	AnchorPoint.X = LocationX;
	AnchorPoint.Y = LocationY;
	SelectionStarted = true;
}

void ARTS_HUD::SelectionModeEnd_Implementation()
{
	SelectionStarted = false;
	SetFormationsInPlayerController(SelectedFormations);
	UpdateWidgetSelection(SelectedFormations);
	AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(GetOwningPlayerController());
	if (SelectedPawn)
	{
		PC->SetSelectedFormations(SelectedFormations);
		SetUnitInPlayerController(SelectedPawn);
		//add selected unit to variable
		// Set selection of formations in the URTSFormationUnitTableWidget
		//maybe add selected pawn to the widget as well
		UpdateWidgetSelection(SelectedFormations);
		InitOrderLineForFormations();
	}
	
	UpdateWidgetSelection(SelectedFormations);
}

void ARTS_HUD::CheckFormationMarkersInSelection(TSet<FGuid>& Markers, const FVector2D& CurrentMousePosition)
{
	Markers.Empty();

	const float MinX = FMath::Min(AnchorPoint.X, CurrentMousePosition.X);
	const float MaxX = FMath::Max(AnchorPoint.X, CurrentMousePosition.X);
	const float MinY = FMath::Min(AnchorPoint.Y, CurrentMousePosition.Y);
	const float MaxY = FMath::Max(AnchorPoint.Y, CurrentMousePosition.Y);

	UNavigationUI_Subsystem* SmartUISubSystem = GetGameInstance()->GetSubsystem<UNavigationUI_Subsystem>();

	if (!SmartUISubSystem)
	{
		return;
	}

	const TSet<FGuid> MarkerGuids = SmartUISubSystem->GetMarkers();

	AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(GetOwningPlayerController());

	if (!PC)
	{
		return;
	}

	for (const FGuid& MarkerGuid : MarkerGuids)
	{
		const FMarkerData MarkerData = SmartUISubSystem->GetMarkerData(MarkerGuid);

		if (!MarkerData.LinkedComponent)
		{
			continue;
		}

		UMarkerWidget* CurrentMarkerWidget =
			IMarkerInterface::Execute_GetWidget(MarkerData.LinkedComponent);

		if (!CurrentMarkerWidget)
		{
			continue;
		}

		// Get the actual widget geometry on the viewport.
		const FGeometry CurrentMarkerWidgetGeometry = CurrentMarkerWidget->GetCachedGeometry();

		FVector2D WidgetViewportPosition;
		FVector2D WidgetPixelPosition;

		USlateBlueprintLibrary::LocalToViewport(GetWorld(), CurrentMarkerWidgetGeometry, FVector2D::ZeroVector, WidgetPixelPosition, WidgetViewportPosition);

		const FVector2D WidgetSize =
			CurrentMarkerWidgetGeometry.GetAbsoluteSize();

		// Marker is circular, so use half of its largest dimension
		// as the selection tolerance/radius.
		const float MarkerRadius = FMath::Max(WidgetSize.X, WidgetSize.Y) * 0.5f;

		// WidgetPixelPosition represents the projected top-left
		// of the widget, so calculate its center.
		const FVector2D MarkerCenter = WidgetPixelPosition + (WidgetSize * 0.5f);

		// Expand the selection rectangle by the marker radius.
		const float ExpandedMinX = MinX - MarkerRadius;
		const float ExpandedMaxX = MaxX + MarkerRadius;
		const float ExpandedMinY = MinY - MarkerRadius;
		const float ExpandedMaxY = MaxY + MarkerRadius;

		if (MarkerCenter.X >= ExpandedMinX &&
			MarkerCenter.X <= ExpandedMaxX &&
			MarkerCenter.Y >= ExpandedMinY &&
			MarkerCenter.Y <= ExpandedMaxY)
		{
			Markers.Add(MarkerGuid);
		}
	}
}

void ARTS_HUD::InitOrderLineForFormations()
{
	for (ABaseFormation* Formation : SelectedFormations)
	{
		for (APawn* Pawn : Formation->GetUnitsInFormation_Implementation())
		{
			AController* Controller = Pawn->GetController();
			if (Controller && !Controller->IsPlayerController())
			{
				UOrdersManager* OrdersManagerComponent = IOrderable::Execute_GetOrdersManagerComponent(Controller);
				OrdersManagerComponent->OnHUDNotify.BindUObject(this, &ARTS_HUD::ResetOrderLineForFormations);
			}
		}
	}
	
	ResetOrderLineForFormations();
}

void ARTS_HUD::ResetOrderLineForFormations()
{
	DrawOrderLineForFormations(SelectedFormations);
}

void ARTS_HUD::DrawOrderLineForFormations(const TArray<ABaseFormation*>& Formations) 
{
	ClearLines();
	for (ABaseFormation* Formation : Formations) 
	{
		DrawOrderLineForFormation(Formation);
	}
}

void ARTS_HUD::DrawOrderLineForFormation(ABaseFormation* Formation) 
{
	DrawOrderLine(Formation, 1.0f);

	for (APawn* Pawn : Formation->GetUnitsInFormation_Implementation()) 
	{
		DrawOrderLineForUnit(Pawn);
	}
}

void ARTS_HUD::DrawOrderLineForUnit(APawn* Unit) 
{
	DrawOrderLine(Unit, 10.0f);
}

void ARTS_HUD::DrawOrderLine(APawn* Pawn, float MinSize) 
{
	if (!LineVFX || !Pawn) return;


	UOrdersManager* TempOrdersManager = nullptr;
	if (Pawn->GetClass()->ImplementsInterface(UFormationInterface::StaticClass()))
	{
		TArray<APawn*> Units = IFormationInterface::Execute_GetUnitsInFormation(Pawn);
		for (APawn* CurrentUnit : Units) 
		{
			AController* Controller = CurrentUnit->GetController();
			if (Controller && !Controller->IsPlayerController())
			{
				//TempOrdersManager = IOrderable::Execute_GetOrdersManagerComponent(Controller);
				break;
			}
		}
	}
	else
	{
		AController* Controller = Pawn->GetController();
		UE_LOG(LogTemp, Warning, TEXT("[DEBUG] DrawOrderLine: Pawn=%s Controller=%s IsPlayerController=%d HasAuthority=%d"),
			*Pawn->GetName(), Controller ? *Controller->GetName() : TEXT("NULL"),
			Controller ? Controller->IsPlayerController() : -1, Pawn->HasAuthority());
		if (Controller &&!Controller->IsPlayerController())
		{
			TempOrdersManager = IOrderable::Execute_GetOrdersManagerComponent(Controller);
		}
	}

	if (!TempOrdersManager)
	{
		return; 
	}
	
	BaseOrder* FormationOrder = TempOrdersManager->GetOrder();
	if (!FormationOrder) return;

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
		LineVFX,
		Pawn->GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false,
		true,
		ENCPoolMethod::AutoRelease
	);

	if (!NiagaraComp)
	{
		return;
	}

	OrderLineMap.Add(FormationOrder, NiagaraComp);
	//FormationOrder->OnHUDNotify.BindUObject(this, &ARTS_HUD::ResetOrderLineForFormations);

	NiagaraComp->SetVariableObject("TargetObject", FormationOrder->GetTargetUnit());
	NiagaraComp->SetVariableVec3("End", FormationOrder->GetTargetPosition());
	NiagaraComp->SetFloatParameter("MinSize", 7.0f);

	switch (FormationOrder->GetOrderType())
	{

	case OrderEnum::Move:
	case OrderEnum::Patrol:
	case OrderEnum::Follow:
		NiagaraComp->SetColorParameter("Color", FLinearColor::Green);
		break;
	case OrderEnum::Interact:
		NiagaraComp->SetColorParameter("Color", FLinearColor::Yellow);
		break;
	case OrderEnum::Attack:
	case OrderEnum::OccupyAOI:
	case OrderEnum::Training:
		NiagaraComp->SetColorParameter("Color", FLinearColor::Red);
		break;
	default:
		break;
	}

}

void ARTS_HUD::ClearLines() 
{
	
	for (auto& KVP : OrderLineMap)
	{
		UNiagaraComponent* CurrentComponent = KVP.Value;

		if (CurrentComponent && CurrentComponent->IsActive())
		{
			CurrentComponent->DeactivateImmediate();
		}
	}
	OrderLineMap.Empty();
}

void ARTS_HUD::InitializeWidget(TArray<ABaseFormation*>& Formations)
{
    if (FormationUnitTableWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEBUG] ARTS_HUD::InitializeWidget: widget valid, Formations.Num()=%d"), Formations.Num());
        FormationUnitTableWidget->InitializeWidget(Formations);
		FormationUnitTableWidget->OnFormationClickedDelegate.AddDynamic(this, &ARTS_HUD::SetFormationsInPlayerController);
		FormationUnitTableWidget->OnUnitClickedDelegate.AddDynamic(this, &ARTS_HUD::SetUnitInPlayerController);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEBUG] ARTS_HUD::InitializeWidget: FormationUnitTableWidget is NULL - skipping!"));
    }
}

void ARTS_HUD::InitializeFormationUnitTableWidget(URTSFormationUnitTableWidget* InWidget)
{
	FormationUnitTableWidget = InWidget;
}

void ARTS_HUD::UpdateWidgetSelection(TArray<ABaseFormation*>& Formations)
{
    if (FormationUnitTableWidget)
    {
        // Reinitialize widget with updated selection list
		//FormationUnitTableWidget->Initialize(Formations);
    }
}

void ARTS_HUD::SetCurrentSelection_Implementation(bool Visible)
{
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	for (ABaseFormation* Actor : SelectedFormations)
	{
		IFormationInterface::Execute_SetSelection(Actor, Visible);
	}
}


void ARTS_HUD::CheckSelectedMarkersForFormation(const TSet<FGuid>& SelectedMarkersArray)
{
	SelectedMarkerGuids.Empty();
	if (SelectedMarkersArray.Num() == 0) return;

	AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(GetOwningPlayerController());

	if (!PC) return;

	UNavigationUI_Subsystem* SmartUISubSystem = GetGameInstance()->GetSubsystem<UNavigationUI_Subsystem>();

	if (!SmartUISubSystem) return;

	TArray<ABaseFormation*> SelectedFormationsArray;

	for (const FGuid& MarkerGuid : SelectedMarkersArray)
	{
		const FMarkerData MarkerData = SmartUISubSystem->GetMarkerData(MarkerGuid);
		if (MarkerData.LinkedComponent)
		{
			
			SelectedMarkerGuids.Add(MarkerGuid);
			AActor* MarkerFormation = MarkerData.LinkedComponent->GetOwner();
			if((IOwnershipInterface::Execute_GetOwningPlayer(MarkerFormation) == GetOwner()))
			{
				SelectedFormationsArray.Add(Cast<ABaseFormation>(MarkerFormation));
			}
			// You can also perform additional logic here, such as highlighting the selected markers in the UI.
		}
	}
	SetFormationsInPlayerController(SelectedFormationsArray);
}

void ARTS_HUD::CheckSelectedFormations(const TArray<APawn*>& SelectedFormationsArray)
{
	SetCurrentSelection(false);
	SelectedFormations.Empty();
	if (SelectedFormationsArray.IsEmpty()) return;
	//int PlayerTeam = IIPlayer::Execute_GetTeamIndex(GetOwner());
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();

	if (SelectedFormationsArray.Num() == 1)
	{
		if (IsActorValidForSelection(SelectedFormationsArray[0]))
		{
			SelectedPawn = SelectedFormationsArray[0];
			SelectedFormations.AddUnique(IFormable::Execute_GetFormation(SelectedPawn));
			GameState->SetSelectionCircle(true, SelectedPawn);
		}
	}
	else
	{
		SelectedPawn = nullptr;
		for (APawn* Actor : SelectedFormationsArray)
		{
			if (IsActorValidForSelection(Actor))
			{
				if (SelectedPawn == nullptr)
				{
					SelectedPawn = Actor;
				}
				SelectedFormations.AddUnique(IFormable::Execute_GetFormation(Actor));
				//add selected actor
				GameState->SetSelectionCircle(true, Actor);
			}
		}
	}
}

bool ARTS_HUD::IsActorValidForSelection(APawn* Actor)
{
	if (Actor->GetClass()->ImplementsInterface(USelectable::StaticClass()))
	{
		if (IOwnershipInterface::Execute_GetOwningPlayer(Actor) == GetOwner()) // zmienić warunek na !IsEnemyUnit z AdBellumGameInstance
		{
			if (ITargetable::Execute_IsAlive(Actor))
			{
				return true;
			}
		}
	}
	return false;
}

void ARTS_HUD::ClearSelectedFormations()
{
	for (ABaseFormation* Formation : SelectedFormations)
	{
		for (APawn* Pawn : Formation->GetUnitsInFormation_Implementation())
		{
			AController* Controller = Pawn->GetController();
			if (Controller && !Controller->IsPlayerController())
			{
				UOrdersManager* OrdersManagerComponent = IOrderable::Execute_GetOrdersManagerComponent(Controller);
				OrdersManagerComponent->OnHUDNotify.Unbind();
			}
		}
	}

	SetCurrentSelection(false);
	SelectedFormations.Empty();
}

void ARTS_HUD::SetFormationsInPlayerController(TArray<ABaseFormation*> Formations)
{
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetFormationsInPlayerController: Formations.Num()=%d"), Formations.Num());
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(GetOwningPlayerController());
	ClearSelectedFormations();
	if (PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetFormationsInPlayerController: PC valid, calling SetSelectedFormations"));
		PC->SetSelectedFormations(Formations);
		SelectedFormations = Formations;
		for (AActor* Actor : Formations)
		{
			if (Actor)
			{
				UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetFormationsInPlayerController: Setting selection circle for Actor=%s"), *Actor->GetName());
				GameState->SetSelectionCircle(true, Actor);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetFormationsInPlayerController: Actor is NULL"));
			}
		}
		InitOrderLineForFormations();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetFormationsInPlayerController: PC is NULL"));
	}
}

void ARTS_HUD::SetUnitInPlayerController(APawn* SelectedUnit)
{
	AAdBellumPlayerController* PC = Cast<AAdBellumPlayerController>(GetOwningPlayerController());
	if (PC)
	{
		PC->SetSelectedPawn(SelectedUnit);
	}
}
