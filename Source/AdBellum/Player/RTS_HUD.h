// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Unit/BaseUnit.h"
#include <UI/RTSFormationUnitTableWidget.h>
#include "RTS_HUD.generated.h"

class AALSBaseCharacter;
class ABaseFormation;
class UNiagaraComponent;
class BaseOrder;

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

	APawn* SelectedPawn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	URTSFormationUnitTableWidget* FormationUnitTableWidget;

public:
	UFUNCTION(BlueprintImplementableEvent)
	void BP_InitializeWidget();

	void HUDOpen(AActor* ControlledActor);

	void HUDClose();

	UFUNCTION(BlueprintImplementableEvent)
	void OnHUDOpen(AActor* ControlledActor);

	UFUNCTION(BlueprintImplementableEvent)
	void OnHUDClose();

	UFUNCTION(BlueprintCallable)
	void InitializeWidget(TArray<ABaseFormation*>& Formations);

	void UpdateWidgetSelection(TArray<ABaseFormation*>& Formations);

	virtual void DrawHUD() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SelectionModeStart();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SelectionModeEnd();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetCurrentSelection(bool Visible);

	void ClearSelectedFormations();

	void CheckSelectedFormations(const TArray<APawn*>& SelectedFormationsArray);

	bool IsActorValidForSelection(APawn* Actor);

	void InitOrderLineForFormations();

	void ResetOrderLineForFormations();

	void DrawOrderLineForFormation(ABaseFormation* Formation);

	void DrawOrderLineForFormations(const TArray<ABaseFormation*>& Formations);

	void DrawOrderLineForUnit(APawn* Unit);

	void DrawOrderLine(APawn* Pawn, float MinSize);

	void ClearLines();
	TMap<BaseOrder*, UNiagaraComponent*> OrderLineMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UNiagaraSystem* LineVFX;

};
