#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "RTSFormationCardWidget.generated.h"

class UScrollBox;
class ABaseFormation;

DECLARE_DELEGATE_TwoParams(FOnFormationCardClicked, ABaseFormation* /*Formation*/, int /*Index*/);

UCLASS()
class ADBELLUM_API URTSFormationCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWidget(ABaseFormation* InFormation);

    void Clear();

    UFUNCTION()
    void OnCardButtonClicked();

	FOnFormationCardClicked OnFormationCardClicked;


protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UOverlay* Overlay;

    UPROPERTY(meta = (BindWidget))
    UButton* CardButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SquadNameText;

    ABaseFormation* Formation = nullptr;

    int Index = -1;
};
