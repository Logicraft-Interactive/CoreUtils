// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataAsset.h"
#include "PoolSettings.generated.h"

class UPoolable;

/**
 * FPoolSettings
 * * Configuration struct defining the behavior of a specific object pool.
 * This struct controls how many objects are created initially, the class to spawn,
 * and how the pool manages memory (expansion and shrinking).
 */
USTRUCT(Blueprintable, BlueprintType)
struct LOGICRAFTCOREUTILS_API FPoolSettings 
{
	GENERATED_BODY()
	 
	/**
	 * The Actor class to spawn and manage in this pool.
	 * * Must implement the IPoolable interface.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite, meta = (MustImplement = "/Script/LogicraftCoreUtils.Poolable"))
	TSubclassOf<AActor> SpawnClass;
	 
	/**
	 * Cached pointer to the world context used for spawning actors.
	 * Typically set at runtime during initialization.
	 */
	TWeakObjectPtr<UWorld> WorldContext{nullptr};
	
	/**
	 * If true, the pool will automatically create new chunks of objects if it runs out of free ones.
	 * If false, SpawnFromPool will return nullptr when the pool is empty.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	bool bAllowResize{true};
	
	/**
	 * If true, the pool will periodically check for unused objects that exceed the MinPoolSize
	 * and destroy them to free up memory.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)
	bool bAutoShrink{true};
	
	/**
	 * The initial number of objects to spawn when the pool is initialized.
	 * If AutoShrink is enabled, the pool will never shrink below this count.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	int MinPoolSize{30};
	
	/**
	 * The interval (in seconds) at which the ShrinkRoutine checks for objects to remove.
	 * Only relevant if bAutoShrink is true.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	float AutoShrinkUpdateTime{15.f};
	
	/**
	 * The duration (in seconds) an object must remain unused (in the 'Free' state)
	 * before it is considered eligible for destruction during a shrink pass.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	float ShrinkObjectAfterReturnTime{30.f};
};
 
/**
 * UPoolSettingsDataAsset
 * * Data Asset wrapper for FPoolSettings.
 * Allows creating and storing pool configurations as assets in the Content Browser,
 * making it easy to reuse settings across different pool managers.
 */
UCLASS(Blueprintable, BlueprintType)
class LOGICRAFTCOREUTILS_API UPoolSettingsDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public: 	
	UPROPERTY(EditDefaultsOnly, Category = "Settings", BlueprintReadWrite)	
	FPoolSettings PoolSettings;
	
};