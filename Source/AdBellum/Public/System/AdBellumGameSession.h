// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "AdBellumGameSession.generated.h"

/**
 * 
 */
UCLASS()
class ADBELLUM_API AAdBellumGameSession : public AGameSession
{
	GENERATED_BODY()
	
public:
	virtual bool HandleStartMatchRequest() override;
	
	
};
