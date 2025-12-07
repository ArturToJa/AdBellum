// Designed by Hitman's Store, 2023

#pragma once

#include "CoreMinimal.h"
#include "RecoilAnimationComponent.h"
#include "Blueprint/UserWidget.h"
#include "HelperWidget.generated.h"

UCLASS()
class RECOILANIMATION_API UHelperWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetComponent(URecoilAnimationComponent* RecAnimComp);

	UFUNCTION(BlueprintImplementableEvent, Category = "Update UI Values")
	void UpdateValues();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Base")
	URecoilAnimationComponent* AnimationComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Display")
	bool bAdvancedDisplay = true;
};