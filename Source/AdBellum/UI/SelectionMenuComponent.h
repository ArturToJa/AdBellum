// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SelectionMenuComponent.generated.h"

class USelectionMenuWidget;

// Generic hold-to-open selection menu on the player controller (debug overlay menu input: hold to open,
// scroll to cycle, release to apply). The options come from an ISelectionMenuProvider: the possessed pawn
// when it is one, otherwise the actor the possessed unit is sitting in (ControlledInputInterceptor).
UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API USelectionMenuComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USelectionMenuComponent();

	// Widget shown while the menu is open (WBP_SelectionMenu)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	TSubclassOf<USelectionMenuWidget> MenuWidgetClass;

	// Local. True opens the menu, false closes it and applies the highlighted entry if it changed.
	// Returns false when there is nothing to show (no provider), so the caller can fall back to other behaviour.
	bool Open(bool bOpen);

	// Local. Moves the highlight, does nothing while closed.
	void Cycle(bool bUp);

	bool IsOpen() const { return MenuWidget != nullptr; }

	// The provider the menu would use right now, or null
	UObject* FindProvider() const;

	UFUNCTION(Server, Reliable)
	void Server_ChooseOption(UObject* Provider, int32 Index);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void CloseWidget();

	UPROPERTY(Transient)
	TObjectPtr<USelectionMenuWidget> MenuWidget;

	TWeakObjectPtr<UObject> OpenProvider;

	int32 OpenedAtIndex = INDEX_NONE;
};
