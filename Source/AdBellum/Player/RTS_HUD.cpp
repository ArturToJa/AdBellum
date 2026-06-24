// Fill out your copyright notice in the Description page of Project Settings.


#include "RTS_HUD.h"
#include "IPlayer.h"
#include "AdBellumPlayerController.h"
#include "System/AdBellumGameState.h"
#include "Interfaces/Selectable.h"
#include "Formation/BaseFormation.h"


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
		GetActorsInSelectionRectangle<APawn>(AnchorPoint, FVector2D(LocationX, LocationY), OutActors, false, false);
		CheckSelectedFormations(OutActors);
		DrawRect(FLinearColor(0.261f, 1.0f, 0.235f, 0.4f), AnchorPoint.X, AnchorPoint.Y, LocationX - AnchorPoint.X, LocationY - AnchorPoint.Y);
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
	Cast<AAdBellumPlayerController>(GetOwningPlayerController())->SetSelectedFormations(SelectedFormations);
}

void ARTS_HUD::SetCurrentSelection_Implementation(bool Visible)
{
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	for (ABaseFormation* Actor : SelectedFormations)
	{
		IFormationInterface::Execute_SetSelection(Actor, Visible);
	}
}

void ARTS_HUD::CheckSelectedFormations(const TArray<APawn*>& SelectedFormationsArray)
{
	SetCurrentSelection(false);
	SelectedFormations.Empty();
	int PlayerTeam = IIPlayer::Execute_GetTeamIndex(GetOwner());
	AAdBellumGameState* GameState = GetWorld()->GetGameState<AAdBellumGameState>();
	for (APawn* Actor : SelectedFormationsArray)
	{
		if (Actor->GetClass()->ImplementsInterface(USelectable::StaticClass()))
		{
			if (IOwnershipInterface::Execute_GetOwningPlayer(Actor) == GetOwner()) // zmienić warunek na !IsEnemyUnit z AdBellumGameInstance
			{
				if (ITargetable::Execute_IsAlive(Actor))
				{
					SelectedFormations.AddUnique(IFormable::Execute_GetFormation(Actor));
					GameState->SetSelectionCircle(true, Actor);
				}
			}
		}
	}
}
