#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OwnershipInterface.h"
#include "HUDInformationInterface.generated.h"

class UOrdersManager;

UINTERFACE(MinimalAPI)
class UHUDInformationInterface : public UInterface
{
    GENERATED_BODY()
};

class ADBELLUM_API IHUDInformationInterface
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
    FString GetPrimaryInfo();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
    FString GetSecondaryInfo();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
    FString GetStatusInfo();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUDInformationInterface")
    UTexture2D* GetIcon();
};