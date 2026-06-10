// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EventBusTestObject.generated.h"

/**
 * Minimal UObject used in EventBus automation tests.
 * Declared in a dedicated header so UHT can generate the required boilerplate.
 * Must not be used outside of test code.
 */
UCLASS()
class UEventBusTestObject : public UObject
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