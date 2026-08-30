#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "RTSUnitCardWidget.generated.h"

class ABaseUnit;


UCLASS()
class ADBELLUM_API URTSUnitCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Initialize with formations (caller owns the array)
    UFUNCTION(BlueprintCallable, Category = "RTS|API")
    void InitializeWidget(const TArray<ABaseUnit*>& Units);


protected:
    virtual void NativeConstruct() override;

};
