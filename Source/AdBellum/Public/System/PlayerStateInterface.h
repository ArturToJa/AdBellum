// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "System/GameModes/GameModeBase.h"

#include "PlayerStateInterface.generated.h"

DECLARE_DELEGATE_OneParam(PlayerDiedDelegate, int32);

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UPlayerStateInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ADBELLUM_API IPlayerStateInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PlayerStateInterface")
	int GetAvailableTickets();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PlayerStateInterface")
	void SetInitialTickets(int InitialTickets);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PlayerStateInterface")
	bool TryConsumeTickets(int NumberOfTickets);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PlayerStateInterface")
	void RespawnFormation(AActor* Formation);

	PlayerDiedDelegate PlayerDiedDelegate;
};
