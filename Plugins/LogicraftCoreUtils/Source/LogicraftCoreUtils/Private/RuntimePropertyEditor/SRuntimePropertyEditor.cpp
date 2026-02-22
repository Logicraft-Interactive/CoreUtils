// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/SRuntimePropertyEditor.h"

void SRuntimePropertyEditor::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SSplitter)
			.Orientation(Orient_Horizontal)
			.MinimumSlotHeight(50.f)

		+ SSplitter::Slot()
			.Value(50.f)
			.MinSize(200.f)
			[
				SAssignNew(EditableObjectList, SListView<FListItemSource>)
					.Orientation(Orient_Vertical)
					//.bEnableShadowBoxStyle(true)
					.EnableAnimatedScrolling(true)
					.ListItemsSource(InArgs._EditableObjectList)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(InArgs._OnEditableObjectAdded)
					.OnSelectionChanged(InArgs._OnEditableObjectSelectionChanged)
			]

		+ SSplitter::Slot()
			.Value(50.f)
			.MinSize(200.f)
			[
				SAssignNew(EditablePropertiesPanel, SVerticalBox)
			]
	];
}

TSharedRef<SScrollBox> SRuntimePropertyEditor::MakeEditablePropertiesScrollBox(const TScriptInterface<IRuntimeEditable>& EditableProperties)
{
	TSharedPtr<SScrollBox> PropertiesContainer;
	SAssignNew(PropertiesContainer, SScrollBox)
		.AnimateWheelScrolling(true)
		.ScrollBarAlwaysVisible(true)
		.EnableTouchScrolling(true)
		.Orientation(Orient_Vertical);

	FRuntimePropertyBuilder RuntimePropertyBuilder{ PropertiesContainer.ToSharedRef() };
	EditableProperties->OnPropertiesDisplay(RuntimePropertyBuilder);
	
	return PropertiesContainer.ToSharedRef();
}

void SRuntimePropertyEditor::DisplayPropertiesContainer(const TSharedPtr<SScrollBox>& PropertiesContainer)
{
	EditablePropertiesPanel->ClearChildren();
	
	if (!PropertiesContainer.IsValid())
	{
		return;
	}

	EditablePropertiesPanel->AddSlot()
		.Padding(5.f)
		.FillHeight(1.f)
		[
			PropertiesContainer.ToSharedRef()
		];
}
