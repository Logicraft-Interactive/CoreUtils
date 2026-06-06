// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyBuilder.h"
#include "SlateFontStyleHelper.h"
#include "Widgets/Layout/SScrollBox.h"

FRuntimePropertyBuilder::FRuntimePropertyBuilder(const TSharedRef<SScrollBox>& InPropertiesContainer)
	: PropertiesContainer(InPropertiesContainer)
{
}

FRuntimePropertyBuilder& FRuntimePropertyBuilder::AddCategory(FStringView CategoryName)
{
	return
		AddRowProperty(SNew(STextBlock)
			.Justification(ETextJustify::Center)
			.Font(SlateFontStyleHelper::GetSlateFontStyle(TEXT("Roboto-Bold.ttf"), 16))
			.Text(FText::FromStringView(CategoryName)));
}

FRuntimePropertyBuilder& FRuntimePropertyBuilder::AddSeparator(const FSlateColor& ColorAndOpacity, float Thickness)
{
	return
		AddRowProperty(
			SNew(SSeparator)
			.Orientation(Orient_Horizontal)
			.ColorAndOpacity(ColorAndOpacity)
			.Thickness(Thickness));
}

FRuntimePropertyBuilder& FRuntimePropertyBuilder::AddRowProperty(const TSharedRef<SWidget>& PropertyWidget)
{
	PropertiesContainer->AddSlot()
		.AutoSize()
		.Padding(5.f)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.FillContentWidth(1.f)
							[
								PropertyWidget
							]
			];
	
	return *this;
}

FRuntimePropertyBuilder& FRuntimePropertyBuilder::AddRowProperty(FStringView PropertyName,
																const TSharedRef<SWidget>& PropertyWidget)
{
	PropertiesContainer->AddSlot()
		.AutoSize()
		.Padding(5.f)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 2.5f, 10.f, 2.5f)
						[
							SNew(STextBlock)
								.Text(FText::FromStringView(PropertyName))
								.MinDesiredWidth(25.f)
						]
			
				+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
						[
							PropertyWidget
						]
		];
	
	return *this;
}