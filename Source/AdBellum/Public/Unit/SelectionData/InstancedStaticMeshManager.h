// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "InstancedStaticMeshManager.generated.h"

UCLASS()
class ADBELLUM_API AInstancedStaticMeshManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInstancedStaticMeshManager();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UHierarchicalInstancedStaticMeshComponent* SelectionCircleInstanced;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void CreateSelectionInstance(AActor* Actor);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void RemoveSelectionInstance(int32 InstanceIndex);

private:
	TArray<AActor*> SelectionActors;

};
