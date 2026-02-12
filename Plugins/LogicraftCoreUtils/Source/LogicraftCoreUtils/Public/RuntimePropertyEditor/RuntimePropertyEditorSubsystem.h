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
	using FEditableObjectList = TWeakObjectPtr<UObject>;
	
	TSharedPtr<SWindow> RuntimePropertyEditorWindow;
	TSharedPtr<SRuntimePropertyEditor> RuntimePropertyEditor;

	TArray<FEditableObjectList> EditableObjects;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable)
	void OpenWindow();

	UFUNCTION(BlueprintCallable)
	void CloseWindow();

	UFUNCTION(BlueprintCallable)
	void RegisterEditableProperties(TScriptInterface<IRuntimeEditable> EditableProperties);
};
