// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Poolable.generated.h"

class UPoolObject;

UINTERFACE(BlueprintType)
class LOGICRAFTCOREUTILS_API UPoolable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LOGICRAFTCOREUTILS_API IPoolable
{
	GENERATED_BODY()

	int Index{0};
	
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	int Internal_GetIndex() const;
	void Internal_SetIndex(const int NewIndex);
	

	UFUNCTION(BlueprintNativeEvent)
	void OnSpawn();

	UFUNCTION(BlueprintNativeEvent)
	void OnReturn();
};
