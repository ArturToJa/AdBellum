// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include <Library/ALSCharacterEnumLibrary.h>
#include "GunnerCrosshairWidget.generated.h"

class UImage;

// Gunner's crosshair. Built entirely in C++ (RebuildWidget constructs its own widget tree) so it does not
// depend on the old BP_Gunner_Widget tree - shown/hidden natively by ACharacterHUD::UpdateGunnerCrosshair.
UCLASS()
class ADBELLUM_API UGunnerCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGunnerCrosshairWidget(const FObjectInitializer& ObjectInitializer);

	// Default crosshair texture, used while the active weapon reports none of its own (GetWeaponHUDReticle)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Crosshair")
	TObjectPtr<UTexture2D> CrosshairTexture;

	// On screen size (pixels) of the crosshair image
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Crosshair")
	FVector2D CrosshairSize = FVector2D(192.0f, 96.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor CrosshairColor = FLinearColor(0.0117f, 1.0f, 0.0f, 0.5f);

	// Fallback pixel offset from screen center, used only while no seated GunnerSeat is found (e.g. previewing
	// this widget outside a vehicle). The reticle glyph inside CrosshairTexture is not centered in the texture
	// canvas, so this nudges the image box until the glyph lines up with screen center - but the correct nudge
	// also depends on the vehicle's own MCT socket placement, which differs per vehicle even for a shared
	// weapon, so the seat's own UGunnerSeatComponent::CrosshairOffset is what actually applies in game.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Crosshair")
	FVector2D CrosshairOffset = FVector2D(0.0f, 40.0f);

	// Only shown for an enclosed Gunner (Use Main Camera Transform): a Gunner_Standing aims through its
	// weapon's own sight directly, which already draws its own reticle - see SetStationaryRole.
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetStationaryRole(EALSStationaryRole Role);

	// Turret-relative-to-hull rotation indicator (vehicle body + armor pips + rotating gun line), ported from
	// the old BP_Gunner_Widget's TurretRotation canvas (Body_icon, Armor_left, Armor_right, Main_gun_line)

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret Rotation")
	TObjectPtr<UTexture2D> BodyTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret Rotation")
	TObjectPtr<UTexture2D> ArmorLeftTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret Rotation")
	TObjectPtr<UTexture2D> ArmorRightTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret Rotation")
	TObjectPtr<UTexture2D> GunLineTexture;

	// Size (pixels) of each icon making up the indicator
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret Rotation")
	FVector2D TurretRotationIconSize = FVector2D(64.0f, 64.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Turret Rotation")
	FLinearColor TurretRotationColor = FLinearColor(0.0117f, 1.0f, 0.0f, 0.5f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;

private:
	EALSStationaryRole StationaryRole = EALSStationaryRole::None;

	UPROPERTY()
	TObjectPtr<UImage> CrosshairImage;

	UPROPERTY()
	TObjectPtr<class UCanvasPanelSlot> CrosshairImageSlot;

	UPROPERTY()
	TObjectPtr<UImage> BodyImage;

	UPROPERTY()
	TObjectPtr<UImage> ArmorLeftImage;

	UPROPERTY()
	TObjectPtr<UImage> ArmorRightImage;

	UPROPERTY()
	TObjectPtr<UImage> GunLineImage;

	// Adds one icon of the turret rotation indicator to Root, anchored at the indicator group's center plus
	// LocalOffset (pixels, same relative layout as the old BP_Gunner_Widget's TurretRotation canvas)
	UImage* AddTurretIcon(class UCanvasPanel* Root, FName Name, UTexture2D* Texture, FVector2D LocalOffset) const;

	const class UGunnerSeatComponent* GetGunnerSeat() const;

	void UpdateCrosshairTexture();
	void UpdateCrosshairOffset();
	void UpdateTurretRotationIndicator();
};
