// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PoolSettings.h"
#include "Poolable.h"
#include "PoolSubsystem.generated.h"

class UPoolObject;
class IPoolable;

/**
 * UPoolSubsystem
 * * The central manager for creating and accessing Object Pools within a World.
 * As a WorldSubsystem, its lifecycle is tied to the UWorld; it is created when a level loads
 * and destroyed when the level unloads.
 * * Usage:
 * Get the subsystem via `GetWorld()->GetSubsystem<UPoolSubsystem>()` to create new pools.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	/**
	 * Called when the subsystem is created (World Init).
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * Called when the subsystem is destroyed (World Cleanup).
	 * Ensures all managed pools are properly invalidated.
	 */
	virtual void Deinitialize() override;

public:

	/**
	 * Creates a new Object Pool based on the provided settings struct.
	 * * @param PoolSettings - The configuration (class, size, resize policy) for the new pool.
	 * * @return A new UPoolObject instance meant to manage the specified actor class.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pool|Subsystem", meta = (ReturnDisplayName = "Pool Object"))
	UPoolObject* CreatePool(FPoolSettings PoolSettings);
	
	/**
	 * Creates a new Object Pool using a Data Asset for configuration.
	 * This is the preferred method for designer-friendly workflows.
	 * * @param PoolSettings - The Data Asset containing the pool configuration.
	 * * @return A new UPoolObject instance, or nullptr if the asset is invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pool|Subsystem", meta = (ReturnDisplayName = "Pool Object"))
	UPoolObject* CreatePoolFromDataAsset(UPoolSettingsDataAsset* PoolSettings);
};