// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimePropertyEditor/RuntimeEditable.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<ITableRow>, FOnEditableObjectAdded, TWeakObjectPtr<>, const TSharedRef<STableViewBase>&);
DECLARE_DELEGATE_TwoParams(FOnEditableObjectSelectionChanged, TWeakObjectPtr<> SelectedItem, ESelectInfo::Type SelectInfo);

/**
 * 
 */
class LOGICRAFTCOREUTILS_API SRuntimePropertyEditor : public SCompoundWidget
{
	using FListItemSource = TWeakObjectPtr<UObject>;
	using SEditableObjectListView = SListView<FListItemSource>;
	
public:
	SLATE_BEGIN_ARGS(SRuntimePropertyEditor) {}

		SLATE_EVENT(FOnEditableObjectAdded, OnEditableObjectAdded)
		SLATE_EVENT(FOnEditableObjectSelectionChanged, OnEditableObjectSelectionChanged)
		
		SLATE_ITEMS_SOURCE_ARGUMENT(FListItemSource, EditableObjectList)
		
	SLATE_END_ARGS()
		
	void Construct(const FArguments& InArgs);

	TSharedRef<SScrollBox> MakeEditablePropertiesScrollBox(const TScriptInterface<IRuntimeEditable>& EditableProperties);

	void DisplayPropertiesContainer(const TSharedPtr<SScrollBox>& PropertiesContainer);

private:
	TSharedPtr<SEditableObjectListView> EditableObjectList;
	TSharedPtr<SVerticalBox> EditablePropertiesPanel;
};
