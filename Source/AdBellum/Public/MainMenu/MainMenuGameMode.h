// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MainMenuGameMode.generated.h"




UCLASS()
class ADBELLUM_API AMainMenuGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bInLobby;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int MapIndex = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int GameModeIndex = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int PlayerNumberIndex = -1;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int TicketsIndex = -1;
	
	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable)
	void SetMap(int _MapIndex);
	UFUNCTION(BlueprintCallable)
	void SetGameMode(int _GameModeIndex);
	UFUNCTION(BlueprintCallable)
	void SetTickets(int _TicketsIndex);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<int> NumberOfPlayersPerTeam;

protected:
	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	virtual void Logout(AController* ExitingPlayer) override;
};
