// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseSight.h"
#include "Components/SceneCaptureComponent2D.h"
#include "UObject/UObjectGlobals.h"
#include "GameFramework/Actor.h"
#include "BaseScope.generated.h"

/**
 * 
 */
UCLASS()
class ADBELLUM_API ABaseScope : public ABaseSight
{
	GENERATED_BODY()
	
public:
	ABaseScope(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneCaptureComponent2D> ScreenCaptureComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* SourceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	TObjectPtr<UStaticMeshComponent> ScopeReticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	int ScopeRenderPixelSize = 320;

	//for custom scopes save mesh mat id and apply render target?

	//UFUNCTION(BlueprintCallable)
	virtual void CalibrateSight(FVector TargetLocation) override;
	virtual void NotifyAim(bool bIsAiming) override;

protected:
	virtual void BeginPlay() override;
};
