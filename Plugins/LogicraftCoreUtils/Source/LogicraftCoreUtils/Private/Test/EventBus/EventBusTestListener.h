// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EventBusTestListener.generated.h"

/**
 * Minimal UObject used in EventBus automation tests.
 * Declared in a dedicated header so UHT can generate the required boilerplate.
 * Must not be used outside of test code.
 */
UCLASS()
class UEventBusTestListener : public UObject
{
	GENERATED_BODY()

public:
	int32 CallCount = 0;
	int32 LastValue  = 0;

	void OnEvent(int32 Value)
	{
		++CallCount;
		LastValue = Value;
	}
};

#endif