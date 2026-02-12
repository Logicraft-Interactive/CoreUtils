// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/SRuntimePropertyEditor.h"
#include "RuntimePropertyEditor/RuntimeEditable.h"

void SRuntimePropertyEditor::Construct(const FArguments& InArgs)
{
	const TArray<FListItemSource>* Editable { InArgs.GetEditableObjectList().ArrayPointer };
	ChildSlot
	[
		SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			.MinimumSlotHeight(50.f)

		+ SSplitter::Slot()
			.Value(50.f)
			.MinSize(200.f)
			.SizeRule(SSplitter::SizeToContent)
			[
				SNew(SListView<FListItemSource>)
					.Orientation(Orient_Vertical)
					.bEnableShadowBoxStyle(true)
					.EnableAnimatedScrolling(true)
					.ListItemsSource(Editable)
			]

		+ SSplitter::Slot()
			.Value(50.f)
			.MinSize(200.f)
			.SizeRule(SSplitter::SizeToContent)
			[
				SNew(STextBlock).Text(FText::FromString("Editable Properties"))
			]
	];
}

void SRuntimePropertyEditor::AddEditableProperties(UObject* EditableProperties)
{
	if (!PropertyPanel.IsValid())
	{
		//TODO : Put an ensure.
		return;
	}

	FString PropertyName;
	IRuntimeEditable::Execute_OnPropertiesDisplay(EditableProperties, PropertyName);

	PropertyPanel->AddSlot()
		.AutoHeight()
		.Padding(50.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(PropertyName))
			];
}
