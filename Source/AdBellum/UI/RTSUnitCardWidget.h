#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "RTSUnitCardWidget.generated.h"

class ABaseUnit;

DECLARE_DELEGATE_TwoParams(FOnUnitCardClicked, ABaseUnit* /*Unit*/, int /*Index*/);

UCLASS()
class ADBELLUM_API URTSUnitCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWidget(APawn* InUnit);

    void Clear();

	FOnUnitCardClicked OnUnitCardClicked;


protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UOverlay* Overlay;

    UPROPERTY(meta = (BindWidget))
    UButton* CardButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* UnitNameText;

    APawn* Unit = nullptr;

    int Index = -1;

};
