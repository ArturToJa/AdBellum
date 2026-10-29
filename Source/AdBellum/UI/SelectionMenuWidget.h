// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SelectionMenuProvider.h"
#include "SelectionMenuWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UImage;

// One row of the selection menu. The blueprint needs a TextBlock named Text and an Image named Background.
UCLASS(Abstract)
class ADBELLUM_API USelectionMenuOptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOption(const FSelectionMenuOption& InOption);
	void SetHighlighted(bool bHighlighted);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	FLinearColor SelectedBackgroundColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	FLinearColor UnselectedBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.25f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	FSlateColor SelectedTextColor = FSlateColor(FLinearColor(0.052f, 0.052f, 0.052f, 1.0f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	FSlateColor UnselectedTextColor = FSlateColor(FLinearColor::White);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	FSlateColor DisabledTextColor = FSlateColor(FLinearColor(0.35f, 0.35f, 0.35f, 1.0f));

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Background;

private:
	bool bOptionEnabled = true;
};

// Generic list menu: one option widget per entry inside a VerticalBox, one highlighted entry,
// cycling wraps around and skips disabled entries. The blueprint needs a VerticalBox named VerticalBox.
UCLASS(Abstract)
class ADBELLUM_API USelectionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOptions(const TArray<FSelectionMenuOption>& InOptions, int32 InSelectedIndex);

	// Up - previous entry, down - next entry
	void Cycle(bool bUp);

	int32 GetSelectedIndex() const { return SelectedIndex; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection Menu")
	TSubclassOf<USelectionMenuOptionWidget> OptionWidgetClass;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox;

private:
	void RefreshHighlight();

	TArray<FSelectionMenuOption> Options;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USelectionMenuOptionWidget>> OptionWidgets;

	int32 SelectedIndex = INDEX_NONE;
};
