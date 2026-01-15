// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EventBusObjectTest.generated.h"

	
/**
 * 
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UEventBusObjectTest : public UObject
{
	GENERATED_BODY()

public:
	void EventCallback();
	void EventCallback_Int(int Arg);
	void EventCallback_Float(float Arg);
};
