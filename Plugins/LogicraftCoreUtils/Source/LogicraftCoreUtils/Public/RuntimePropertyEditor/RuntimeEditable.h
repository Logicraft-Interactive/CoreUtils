// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FRuntimePropertyBuilder.h"
#include "UObject/Interface.h"
#include "RuntimeEditable.generated.h"

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

public:
	virtual void OnPropertiesDisplay(FRuntimePropertyBuilder& PropertiesBuilder)
		PURE_VIRTUAL(IRuntimeEditable::OnPropertiesDisplay, )
};
