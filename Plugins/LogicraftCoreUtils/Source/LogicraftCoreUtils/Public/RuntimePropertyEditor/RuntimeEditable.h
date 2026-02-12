// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RuntimeEditable.generated.h"

// This class does not need to be modified.
UINTERFACE(Category = "RuntimePropertyEditor")
class URuntimeEditable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LOGICRAFTCOREUTILS_API IRuntimeEditable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnPropertiesDisplay(FString& PropertyName);
};
