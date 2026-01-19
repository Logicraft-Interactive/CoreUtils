// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PoolSettings.generated.h"

class UPoolable;
/**
 * 
 */
USTRUCT()
struct LOGICRAFTCOREUTILS_API FPoolSettings 
{
	GENERATED_BODY()

	TSubclassOf<AActor> SpawnClass;
	TWeakObjectPtr<UWorld> WorldContext = nullptr;
	bool bAutoShrink = true;
	int MinPoolSize = 30;
	float AutoShrinkUpdateTime = 15.f;
	float ShrinkObjectAfterReturnTime = 30.f;
};
