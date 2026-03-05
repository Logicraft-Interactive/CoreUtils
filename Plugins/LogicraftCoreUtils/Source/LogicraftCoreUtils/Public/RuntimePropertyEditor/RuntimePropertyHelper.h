// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeEditable.h"

#include "RuntimePropertyHelper.generated.h"

/**
 * 
 */
UCLASS()
class LOGICRAFTCOREUTILS_API URuntimePropertyHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void OpenWindow(const UObject* WorldContext);

	UFUNCTION(BlueprintCallable)
	static void CloseWindow(const UObject* WorldContext);

	static void RegisterEditableObject(const UObject* WorldContext, const TScriptInterface<IRuntimeEditable>& RuntimeEditable);
};
