// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SelectionMenuProvider.generated.h"

class APlayerController;

// One entry of the generic selection menu (USelectionMenuComponent / USelectionMenuWidget)
USTRUCT(BlueprintType)
struct ADBELLUM_API FSelectionMenuOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	FText Label;

	// Disabled entries are shown greyed out and skipped when cycling
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	bool bEnabled = true;
};

UINTERFACE(MinimalAPI, Blueprintable)
class USelectionMenuProvider : public UInterface
{
	GENERATED_BODY()
};

// Anything that offers a list for the selection menu (seats of a vehicle, later other features).
// The menu asks for the options on the player's machine and sends the choice to the server, so both
// functions must agree on the order of the options.
class ADBELLUM_API ISelectionMenuProvider
{
	GENERATED_BODY()

public:
	// Player's machine: the entries to show and the one highlighted when the menu opens
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Selection Menu")
	void GetSelectionMenuOptions(APlayerController* Player, TArray<FSelectionMenuOption>& Options, int32& CurrentIndex);

	// Server: Player picked Index. Check again here, the client's list can be out of date.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Selection Menu")
	void HandleSelectionMenuChoice(APlayerController* Player, int32 Index);
};
