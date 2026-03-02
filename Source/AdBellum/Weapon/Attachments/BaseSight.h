// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h" 
#include "BaseSight.generated.h"

UCLASS()
class ADBELLUM_API ABaseSight : public AActor 
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseSight();

	UPROPERTY(VisibleDefaultsOnly, Category = "Components")
	class USceneComponent* RootSceneComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SightMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SightView;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> ADS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	FName SocketName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* SightArrow;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	float BaseMouseSensitivity = 0.15f;
	
	//UFUNCTION(BlueprintCallable)
	virtual void CalibrateSight(FVector TargetLocation);
	virtual void NotifyAim(bool bIsAiming);
	float GetBaseSensitivity();
protected:
	// Called when the game starts or when spawned
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ADBELLUM_API USightChildActorComponent : public UChildActorComponent
{
	GENERATED_BODY()

public:
	//USightChildActorComponent();
	USightChildActorComponent(const FObjectInitializer& Initializer);
	//virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	ABaseSight* GetSightActor() const;
	void CalibrateSightActor(FVector TargetLocation);
};