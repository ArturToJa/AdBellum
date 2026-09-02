#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "RTSUnitCardWidget.generated.h"

class ABaseUnit;

DECLARE_DELEGATE_TwoParams(FOnUnitCardClicked, ABaseUnit* /*Unit*/, int /*Index*/);

UCLASS()
class ADBELLUM_API URTSUnitCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Initialize with formations (caller owns the array)
    UFUNCTION(BlueprintCallable, Category = "RTS|API")
    void InitializeWidget(const TArray<ABaseUnit*>& Units);

	FOnUnitCardClicked OnUnitCardClicked;


protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* CardButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* UnitNameText;

};
