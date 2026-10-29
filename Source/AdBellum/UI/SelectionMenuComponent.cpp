// Fill out your copyright notice in the Description page of Project Settings.

#include "SelectionMenuComponent.h"
#include "SelectionMenuProvider.h"
#include "SelectionMenuWidget.h"
#include "Character/ALSBaseCharacter.h"
#include "GameFramework/PlayerController.h"

USelectionMenuComponent::USelectionMenuComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Server_ChooseOption goes through this component of the player's own controller
	SetIsReplicatedByDefault(true);
}

UObject* USelectionMenuComponent::FindProvider() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return nullptr;
	}
	if (Pawn->GetClass()->ImplementsInterface(USelectionMenuProvider::StaticClass()))
	{
		return Pawn;
	}
	const AALSBaseCharacter* Character = Cast<AALSBaseCharacter>(Pawn);
	AActor* Interceptor = Character ? Character->ControlledInputInterceptor.Get() : nullptr;
	if (Interceptor && Interceptor->GetClass()->ImplementsInterface(USelectionMenuProvider::StaticClass()))
	{
		return Interceptor;
	}
	return nullptr;
}

bool USelectionMenuComponent::Open(bool bOpen)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return false;
	}

	if (!bOpen)
	{
		if (!IsOpen())
		{
			return false;
		}
		const int32 Chosen = MenuWidget->GetSelectedIndex();
		UObject* Provider = OpenProvider.Get();
		if (Provider && Chosen != INDEX_NONE && Chosen != OpenedAtIndex)
		{
			Server_ChooseOption(Provider, Chosen);
		}
		CloseWidget();
		return true;
	}

	if (IsOpen() || !MenuWidgetClass)
	{
		return IsOpen();
	}
	UObject* Provider = FindProvider();
	if (!Provider)
	{
		return false;
	}

	TArray<FSelectionMenuOption> Options;
	int32 CurrentIndex = INDEX_NONE;
	ISelectionMenuProvider::Execute_GetSelectionMenuOptions(Provider, PlayerController, Options, CurrentIndex);
	if (Options.Num() == 0)
	{
		return false;
	}

	MenuWidget = CreateWidget<USelectionMenuWidget>(PlayerController, MenuWidgetClass);
	if (!MenuWidget)
	{
		return false;
	}
	MenuWidget->SetOptions(Options, CurrentIndex);
	MenuWidget->AddToViewport();
	OpenProvider = Provider;
	OpenedAtIndex = CurrentIndex;
	return true;
}

void USelectionMenuComponent::Cycle(bool bUp)
{
	if (MenuWidget)
	{
		MenuWidget->Cycle(bUp);
	}
}

void USelectionMenuComponent::Server_ChooseOption_Implementation(UObject* Provider, int32 Index)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (PlayerController && Provider && Provider->GetClass()->ImplementsInterface(USelectionMenuProvider::StaticClass()))
	{
		ISelectionMenuProvider::Execute_HandleSelectionMenuChoice(Provider, PlayerController, Index);
	}
}

void USelectionMenuComponent::CloseWidget()
{
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
	}
	MenuWidget = nullptr;
	OpenProvider.Reset();
	OpenedAtIndex = INDEX_NONE;
}

void USelectionMenuComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CloseWidget();
	Super::EndPlay(EndPlayReason);
}
