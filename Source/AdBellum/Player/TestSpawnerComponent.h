// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TestSpawnerComponent.generated.h"

class AALSBaseCharacter;

USTRUCT(BlueprintType)
struct ADBELLUM_API FTestSpawn
{
public:
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<APawn> UnitClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int32 NumberOfActors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float SpawnRadius;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTransform TargetTransform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FName PrefabName;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADBELLUM_API UTestSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTestSpawnerComponent();

	TArray<AActor*> SpawnTestUnits();

	UPROPERTY(EditDefaultsOnly)
	TArray<FTestSpawn> TestSpawnData;
};
