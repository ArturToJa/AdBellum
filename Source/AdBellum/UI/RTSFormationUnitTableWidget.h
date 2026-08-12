#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "RTSFormationUnitTableWidget.generated.h"

class UUniformGridPanel;
class UScrollBox;
class URTSMinimumDataCardWidget;
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
    TSubclassOf<URTSMinimumDataCardWidget> CardWidgetClass;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "RTS|Events")
    FRTSFormationClicked OnFormationClicked;

    UPROPERTY(BlueprintAssignable, Category = "RTS|Events")
    FRTSUnitClicked OnUnitClicked;

    // Initialize with formations (caller owns the array)
    UFUNCTION(BlueprintCallable, Category = "RTS|API")
    void InitializeWidget(const TArray<ABaseFormation*>& Formations);

    UFUNCTION(BlueprintCallable, Category = "RTS|API")
    void Refresh();

    UFUNCTION(BlueprintPure, Category = "RTS|API")
    ABaseFormation* GetSelectedFormation() const { return SelectedFormation.Get(); }

    UFUNCTION(BlueprintPure, Category = "RTS|API")
    ABaseUnit* GetSelectedUnit() const { return SelectedUnit.Get(); }

protected:
    virtual void NativeConstruct() override;

    // Bind these widget names in the UMG Blueprint
    UPROPERTY(meta = (BindWidget))
    UScrollBox* FormationScrollBox;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* UnitScrollBox;

private:
    // References to formations provided in Initialize
    TArray<TWeakObjectPtr<ABaseFormation>> FormationsArray;

    // Currently selected objects
    TWeakObjectPtr<ABaseFormation> SelectedFormation;
    TWeakObjectPtr<ABaseUnit> SelectedUnit;

    // Mapping from bound actor to created card widget for quick refresh
    TMap<TWeakObjectPtr<UObject>, URTSMinimumDataCardWidget*> CardMap;

    // rebuild helpers
    void RebuildFormationCards();
    void RebuildUnitCards();
    void RefreshFormationCard(ABaseFormation* Formation);
    void RefreshUnitCard(ABaseUnit* Unit);

    // callbacks
    UFUNCTION()
    void OnCardClicked(UObject* BoundObject);
};
