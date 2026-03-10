// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/RuntimePropertyEditor.h"
#include "Widgets/Layout/SScrollBox.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

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

TSharedRef<SScrollBox> SRuntimePropertyEditor::MakeEditablePropertiesWidget(const TScriptInterface<IRuntimeEditable>& EditableProperties)
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

void SRuntimePropertyEditor::RefreshEditableObjectList()
{
	if (EditableObjectList.IsValid())
	{
		EditableObjectList->RequestListRefresh();
	}
}

void SRuntimePropertyEditor::DisplayObjectProperties(const TSharedPtr<SScrollBox>& PropertiesContainer)
{
	EditablePropertiesPanel->ClearChildren();
	
	if (!PropertiesContainer.IsValid())
	{
		SelectedObjectScrollBox.Reset();
		return;
	}
	
	SelectedObjectScrollBox = PropertiesContainer;
	
	EditablePropertiesPanel->AddSlot()
		.Padding(5.f)
		.FillHeight(1.f)
		[
			PropertiesContainer.ToSharedRef()
		];
}

bool SRuntimePropertyEditor::IsSelected(const TSharedPtr<SScrollBox>& SelectedScrollBox) const
{
	return SelectedObjectScrollBox == SelectedScrollBox;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION