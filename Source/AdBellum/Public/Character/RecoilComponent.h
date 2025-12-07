// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecoilComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADBELLUM_API URecoilComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	//URecoilComponent();
	URecoilComponent(const FObjectInitializer& ObjectInitializer);
	URecoilComponent(const FObjectInitializer& ObjectInitializer, APlayerController* OwnerController ,float InitVerticalMultiplier, float InitHorizontalMultiplier);

protected:
	// Called when the game starts
	/*virtual void BeginPlay() override;*/

public:	

	void AddRecoil(float VerticalValue, float HorizontalValue, TSubclassOf<class UCameraShakeBase> ShakeClass);

	void PlayCameraShake(TSubclassOf<class UCameraShakeBase> ShakeClass, float Scale);

	//Recoil system for player
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float VerticalRecoilMultiplier = 0.075f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float MaxVerticalMultiplier = 0.75f;

	float VerticalRecoilRiseRate = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float HorizontalRecoilMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float MaxHorizontalMultiplier = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	bool PrintDebugInfo = false;

	float HorizontalRecoilRiseRate = 1.25f;

	void IncreaseRecoilMultiplier();

	//cooldown 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recoil")
	float RecoilCoolDownSpeed = 1.0f;

	FTimerHandle RecoilCooldownTimer;

	void SetOwningController(APlayerController* NewController);
	
private:
	APlayerController* OwnerController;
};
