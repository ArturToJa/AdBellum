// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Unit/BaseUnit.h"
#include "RTS_HUD.generated.h"

class AALSBaseCharacter;
class ABaseFormation;

/**
 * 
 */
UCLASS()
class ADBELLUM_API ARTS_HUD : public AHUD
{
	GENERATED_BODY()
	

protected:
	bool SelectionStarted;
	FVector2D AnchorPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<ABaseFormation*> SelectedFormations;

public:
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SelectionModeStart();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SelectionModeEnd();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetCurrentSelection(bool Visible);

	void CheckSelectedFormations(const TArray<APawn*>& SelectedFormationsArray);
};
