// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeEditable.h"
#include "SRuntimePropertyEditor.h"
#include "Subsystems/WorldSubsystem.h"
#include "RuntimePropertyEditorSubsystem.generated.h"

/**
 * 
 */
UCLASS(Category = "RuntimePropertyEditor")
class LOGICRAFTCOREUTILS_API URuntimePropertyEditorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
private:
	using FEditableObjectType = TWeakObjectPtr<UObject>;
	
	TSharedPtr<SWindow> RuntimePropertyEditorWindow;
	TSharedPtr<SRuntimePropertyEditor> RuntimePropertyEditor;

	TArray<FEditableObjectType> EditableObjects;
	TMap<FEditableObjectType, TSharedRef<SScrollBox>> EditableObjectsUIProperties;
	
public:
	static ThisClass* Get(const UObject* WorldContext);
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable)
	void OpenWindow();

	UFUNCTION(BlueprintCallable)
	void CloseWindow();
	
	void RegisterEditableProperties(const TScriptInterface<IRuntimeEditable>& RuntimeEditable);

private:
	TSharedRef<ITableRow> OnEditableObjectAdded(TWeakObjectPtr<> EditableObject, const TSharedRef<STableViewBase>& Owner);

	void OnEditableObjectSelectionChanged(TWeakObjectPtr<> SelectedItem, ESelectInfo::Type SelectInfo);
};
