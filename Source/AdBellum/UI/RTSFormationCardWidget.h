#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "RTSFormationCardWidget.generated.h"

class UScrollBox;
class ABaseFormation;

DECLARE_DELEGATE_TwoParams(FOnFormationCardClicked, ABaseFormation* /*Formation*/, int /*Index*/);

UCLASS()
class ADBELLUM_API URTSFormationCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:


    // Initialize with formations (caller owns the array)
    UFUNCTION(BlueprintCallable, Category = "RTS|API")
    void InitializeWidget(ABaseFormation* InFormation);

    UFUNCTION()
    void OnCardButtonClicked();

	FOnFormationCardClicked OnFormationCardClicked;


protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    UButton* CardButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* SquadNameText;

    ABaseFormation* Formation = nullptr;

    int Index = -1;
};
