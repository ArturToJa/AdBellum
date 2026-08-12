// Reusable minimum-data card widget for formations and units
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "RTSMinimumDataCardWidget.generated.h"

class ABaseUnit;
class ABaseFormation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRTSCardClicked, UObject*, BoundObject);

UCLASS()
class ADBELLUM_API URTSMinimumDataCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URTSMinimumDataCardWidget(const FObjectInitializer& ObjInit);

    // Bind this card to a formation actor
    UFUNCTION(BlueprintCallable, Category = "RTS|Card")
    void BindToFormation(ABaseFormation* Formation);

    // Bind this card to a unit actor
    UFUNCTION(BlueprintCallable, Category = "RTS|Card")
    void BindToUnit(ABaseUnit* Unit);

    // Explicit setter for display data (optional)
    UFUNCTION(BlueprintCallable, Category = "RTS|Card")
    void SetData(UTexture2D* Icon, const FString& Primary, const FString& Secondary, const FString& Status);

    // Toggle visual selected state
    UFUNCTION(BlueprintCallable, Category = "RTS|Card")
    void SetSelected(bool bSelected);

    // Called when internal button is clicked
    UFUNCTION()
    void NotifyClicked();

    // Event parent can bind to
    UPROPERTY(BlueprintAssignable, Category = "RTS|Card")
    FOnRTSCardClicked OnCardClicked;

    // Accessor for the bound object (formation or unit)
    UObject* GetBoundObject() const { return BoundObject.Get(); }

protected:
    virtual void NativeConstruct() override;

    // Visual children created in code
    UPROPERTY()
    UButton* RootButton;

    UPROPERTY()
    UBorder* BackgroundBorder;

    UPROPERTY()
    UImage* IconImage;

    UPROPERTY()
    UTextBlock* PrimaryTextBlock;

    UPROPERTY()
    UTextBlock* SecondaryTextBlock;

    UPROPERTY()
    UTextBlock* StatusTextBlock;

    // Weak reference to the bound formation or unit
    TWeakObjectPtr<UObject> BoundObject;
};
