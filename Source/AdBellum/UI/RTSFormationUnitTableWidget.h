#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "RTSFormationUnitTableWidget.generated.h"

class UUniformGridPanel;
class UScrollBox;
class URTSUnitCardWidget;
class URTSFormationCardWidget;
class ABaseFormation;
class ABaseUnit;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSFormationClicked, ABaseFormation*, Formation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRTSUnitClicked, ABaseUnit*, Unit);

UCLASS()
class ADBELLUM_API URTSFormationUnitTableWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Layout")
    int32 MaxVisibleColumns = 8;

    // Card widget class to instantiate
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Layout")
    TSubclassOf<URTSUnitCardWidget> UnitCardWidgetClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS|Layout")
    TSubclassOf<URTSFormationCardWidget> FormationCardWidgetClass;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "RTS|Events")
    FRTSFormationClicked OnFormationClicked;

    UPROPERTY(BlueprintAssignable, Category = "RTS|Events")
    FRTSUnitClicked OnUnitClicked;

    // Initialize with formations (caller owns the array)
    UFUNCTION(BlueprintCallable, Category = "RTS|API")
    void InitializeWidget(const TArray<ABaseFormation*>& Formations);


protected:
    virtual void NativeConstruct() override;

    void OnFormationClicked(ABaseFormation* Formation, int Index);
	void OnUnitClicked(ABaseUnit* Unit, int Index);

    // Bind these widget names in the UMG Blueprint
    UPROPERTY(meta = (BindWidget))
    UScrollBox* FormationScrollBox;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* UnitScrollBox;


};
