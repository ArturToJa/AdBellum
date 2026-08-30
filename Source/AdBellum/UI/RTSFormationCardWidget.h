#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "RTSFormationCardWidget.generated.h"

class UScrollBox;
class ABaseFormation;

UCLASS()
class ADBELLUM_API URTSFormationCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:


    // Initialize with formations (caller owns the array)
    UFUNCTION(BlueprintCallable, Category = "RTS|API")
    void InitializeWidget(const TArray<ABaseFormation*>& Formations);


protected:
    virtual void NativeConstruct() override;

};
