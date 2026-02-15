// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "RuntimePropertyEditor/FRuntimePropertyBuilder.h"

#include "Widgets/Input/SSpinBox.h"

FRuntimePropertyBuilder::FRuntimePropertyBuilder(const TSharedRef<SScrollBox>& InPropertiesContainer)
	: PropertiesContainer(InPropertiesContainer)
{
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
						[
							SNew(STextBlock)
								.Text(FText::FromString(PropertyName.GetData()))
						]

				+ SHorizontalBox::Slot()
					.AutoWidth()
						[
							PropertyWidget
						]
		];
	
	return *this;
}

TSharedRef<SScrollBox> FRuntimePropertyBuilder::GetPropertiesContainer() const
{
	return PropertiesContainer.ToSharedRef();
}
