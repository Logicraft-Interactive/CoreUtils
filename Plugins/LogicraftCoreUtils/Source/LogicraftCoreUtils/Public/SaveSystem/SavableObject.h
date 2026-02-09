// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SavableObject.generated.h"

// This class does not need to be modified.
UINTERFACE()
class USavableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class LOGICRAFTCOREUTILS_API ISavableObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	
	virtual void OnPreLoad();
	virtual void OnPreSave();
	
	virtual void OnPostLoad();
	virtual void OnPostSave();
};
