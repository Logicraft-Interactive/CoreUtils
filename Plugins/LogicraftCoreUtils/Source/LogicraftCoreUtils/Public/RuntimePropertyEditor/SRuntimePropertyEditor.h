// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

/**
 * 
 */
class LOGICRAFTCOREUTILS_API SRuntimePropertyEditor : public SCompoundWidget
{
	using FListItemSource = TWeakObjectPtr<UObject>;
	using SEditableObjectListView = SListView<FListItemSource>;
	
public:
	SLATE_BEGIN_ARGS(SRuntimePropertyEditor) {}

		SLATE_ITEMS_SOURCE_ARGUMENT(FListItemSource, EditableObjectList)
		
	SLATE_END_ARGS()
		
	void Construct(const FArguments& InArgs);

	void AddEditableProperties(UObject* EditableProperties);

private:
	TSharedPtr<SVerticalBox> PropertyPanel;
	TSharedPtr<SEditableObjectListView> EditableObjectList;
};
