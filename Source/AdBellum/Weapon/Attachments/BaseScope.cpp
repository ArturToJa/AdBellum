// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseScope.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include <Kismet/KismetSystemLibrary.h>

ABaseScope::ABaseScope(const FObjectInitializer& ObjectInitializer)
{
	ScreenCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("2D Screen Capture"));

	ScreenCaptureComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	ScreenCaptureComponent->SetAutoActivate(false);
	ScreenCaptureComponent->bCaptureEveryFrame = false;
	ScreenCaptureComponent->bCaptureOnMovement = false;
	ScreenCaptureComponent->ShowFlags.SetDynamicShadows(true);
	ScopeReticle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Scope reticle"));
	ScopeReticle->AttachToComponent(SightView, FAttachmentTransformRules::KeepRelativeTransform);
	ScopeReticle->SetCollisionProfileName("NoCollision");
	bReplicates = false;
	bAlwaysRelevant = true;
}

void ABaseScope::BeginPlay()
{
	Super::BeginPlay();
	UTextureRenderTarget2D* ScreenCapture = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), ScopeRenderPixelSize, ScopeRenderPixelSize, RTF_RGBA8_SRGB);
	ScreenCaptureComponent->TextureTarget = ScreenCapture;
	UMaterialInstanceDynamic* ScopeMaterial = SightView->CreateDynamicMaterialInstance(0, SourceMaterial);
	ScopeMaterial->SetTextureParameterValue("RenderTarget", ScreenCapture);
	ScreenCaptureComponent->SetActive(false);
}

void ABaseScope::CalibrateSight(FVector TargetLocation)
{
	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(ScreenCaptureComponent->GetRelativeLocation(), TargetLocation);
	ScreenCaptureComponent->AddRelativeRotation(FRotator(TargetRotation.Pitch, 0.0f, 0.0f), true, nullptr, ETeleportType::TeleportPhysics);
	//UKismetSystemLibrary::PrintString(GetWorld(), "SCOPE CALIBRATED", true, true);
}

void ABaseScope::NotifyAim(bool bIsAiming)
{
	ScreenCaptureComponent->SetActive(bIsAiming);
	ScreenCaptureComponent->bCaptureEveryFrame = bIsAiming;
	ScreenCaptureComponent->bCaptureOnMovement = bIsAiming;
	SightView->SetVisibility(bIsAiming);
	ScopeReticle->SetVisibility(bIsAiming);
}