// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "RuntimePropertyEditor/RuntimePropertyHelper.h"

#include "Chain.h"
#include "RuntimePropertyEditor/RuntimePropertyEditorSubsystem.h"

void URuntimePropertyHelper::OpenWindow(const UObject* WorldContext)
{
	Chain::StartChain(
	URuntimePropertyEditorSubsystem::Get(WorldContext)).Execute(
		[](URuntimePropertyEditorSubsystem* RuntimePropertyEditor)
		{
			RuntimePropertyEditor->OpenWindow();
		});
}

void URuntimePropertyHelper::CloseWindow(const UObject* WorldContext)
{
	Chain::StartChain(
	URuntimePropertyEditorSubsystem::Get(WorldContext)).Execute(
		[](URuntimePropertyEditorSubsystem* RuntimePropertyEditor)
		{
			RuntimePropertyEditor->CloseWindow();
		});
}

void URuntimePropertyHelper::RegisterEditableObject(const UObject* WorldContext,
	const TScriptInterface<IRuntimeEditable>& RuntimeEditable)
{
	Chain::StartChain(
	URuntimePropertyEditorSubsystem::Get(WorldContext)).Execute(
		[&RuntimeEditable](URuntimePropertyEditorSubsystem* RuntimePropertyEditor)
		{
			RuntimePropertyEditor->RegisterEditableObject(RuntimeEditable);
		});	
}

void URuntimePropertyHelper::UnRegisterEditableObject(const UObject* WorldContext,
	const TScriptInterface<IRuntimeEditable>& RuntimeEditable)
{
	Chain::StartChain(
	URuntimePropertyEditorSubsystem::Get(WorldContext)).Execute(
		[&RuntimeEditable](URuntimePropertyEditorSubsystem* RuntimePropertyEditor)
		{
			RuntimePropertyEditor->UnRegisterEditableObject(RuntimeEditable);
		});
}
