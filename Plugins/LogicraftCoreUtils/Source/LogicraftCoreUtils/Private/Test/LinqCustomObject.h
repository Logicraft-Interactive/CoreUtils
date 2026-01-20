// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PoolSystem/Poolable.h"
#include "UObject/Object.h"
#include "LinqCustomObject.generated.h"

/**
 * 
 */
UCLASS()
class LOGICRAFTCOREUTILS_API ULinqCustomObject : public UObject
{
	GENERATED_BODY()
};

UCLASS()
class APoolableTestActor : public AActor, public IPoolable
{
	GENERATED_BODY()
public:
	// Implementation of IPoolable interface methods (adjust based on your actual interface)
	virtual void OnSpawn_Implementation() override {}
	virtual void OnReturn_Implementation() override {}
};