// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PoolSettings.h"
#include "PoolSubsystem.generated.h"


class UPoolObject;
class IPoolable;

 
/**
 * 
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()


	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:

	UFUNCTION(Blueprintable)
	UPoolObject* CreatePool(FPoolSettings PoolSettings);
};
