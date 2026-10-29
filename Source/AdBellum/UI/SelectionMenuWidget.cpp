// Fill out your copyright notice in the Description page of Project Settings.

#include "SelectionMenuWidget.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void USelectionMenuOptionWidget::SetOption(const FSelectionMenuOption& InOption)
{
	bOptionEnabled = InOption.bEnabled;
	if (Text)
	{
		Text->SetText(InOption.Label);
	}
	SetHighlighted(false);
}

void USelectionMenuOptionWidget::SetHighlighted(bool bHighlighted)
{
	if (Background)
	{
		Background->SetColorAndOpacity(bHighlighted ? SelectedBackgroundColor : UnselectedBackgroundColor);
	}
	if (Text)
	{
		Text->SetColorAndOpacity(bHighlighted ? SelectedTextColor : (bOptionEnabled ? UnselectedTextColor : DisabledTextColor));
	}
}

void USelectionMenuWidget::SetOptions(const TArray<FSelectionMenuOption>& InOptions, int32 InSelectedIndex)
{
	Options = InOptions;
	OptionWidgets.Reset();
	if (VerticalBox)
	{
		VerticalBox->ClearChildren();
	}

	if (OptionWidgetClass && VerticalBox)
	{
		for (const FSelectionMenuOption& Option : Options)
		{
			USelectionMenuOptionWidget* OptionWidget = CreateWidget<USelectionMenuOptionWidget>(this, OptionWidgetClass);
			OptionWidget->SetOption(Option);
			VerticalBox->AddChildToVerticalBox(OptionWidget);
			OptionWidgets.Add(OptionWidget);
		}
	}

	SelectedIndex = Options.IsValidIndex(InSelectedIndex) ? InSelectedIndex : INDEX_NONE;
	RefreshHighlight();
}

void USelectionMenuWidget::Cycle(bool bUp)
{
	const int32 Num = Options.Num();
	if (Num == 0)
	{
		return;
	}

	// Walk once around the list from the current entry to the next enabled one
	const int32 Step = bUp ? -1 : 1;
	int32 Index = SelectedIndex == INDEX_NONE ? (bUp ? 0 : Num - 1) : SelectedIndex;
	for (int32 Tries = 0; Tries < Num; ++Tries)
	{
		Index = (Index + Step + Num) % Num;
		if (Options[Index].bEnabled)
		{
			SelectedIndex = Index;
			break;
		}
	}
	RefreshHighlight();
}

void USelectionMenuWidget::RefreshHighlight()
{
	for (int32 Index = 0; Index < OptionWidgets.Num(); ++Index)
	{
		if (OptionWidgets[Index])
		{
			OptionWidgets[Index]->SetHighlighted(Index == SelectedIndex);
		}
	}
}
