// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Poolable.h"
#include "PoolSettings.h"
#include "UObject/Object.h"
#include "TimerHolder.h"
#include "PoolObject.generated.h"

class IPoolable;

/**
 * UPoolObject
 * * Manages a pool of specific actors that implement the IPoolable interface.
 * Handles spawning, retrieving, returning, and dynamically resizing the pool based on settings.
 */
UCLASS(Blueprintable, BlueprintType)
class LOGICRAFTCOREUTILS_API UPoolObject : public UObject
{
	GENERATED_BODY()

	/**
	 * Internal struct to track the state and metadata of a pooled object.
	 */
	struct FPoolableInfo
	{
		IPoolable* Poolable{nullptr};
		bool bIsFree{true};
		float ReturnTime{0.f}; // Timestamp or duration tracking for shrinking logic.
	};

	enum class ESwitchState
	{
		Activate,
		Deactivate
	};
	
	/** Configuration settings for this specific pool (Size, Class, Resizing policy). */
	FPoolSettings PoolSettings;

	/** Main container holding references to all pooled objects. */
	TArray<FPoolableInfo> PoolArray;

	/** Optional tracker for the next available linear index, if not using the Queue. */
	TOptional<int> NextIndex{NullOpt};

	/** Timer handle for the routine that periodically removes unused objects to save memory. */
	FTimerHolder ShrinkRoutineTimer;

	/**
	 * Queue storing indices of objects that have been returned to the pool.
	 * Allows for O(1) retrieval of free objects.
	 */
	TQueue<int> IndexQueue;

	/** Helper to retrieve the AActor pointer from an IPoolable interface. */
	static AActor* GetActor(const IPoolable* Poolable);
	
	/** Instantiates a new batch of actors (Chunk) defined by the PoolSettings. */
	void AddNewChunk();

	/** Searches for the next available index if the Queue is empty. */
	bool FindNextIndex();

	/** Internal logic to retrieve and wake up an actor. */
	IPoolable* SpawnActor(const FTransform& SpawnTransform);

	/** Internal logic to put an actor back to sleep. */
	void ReturnActor(int Index);

	/**
	 * Toggles the actor's active state (Collision, Visibility, Tick).
	 * * @param Actor - The actor to modify.
	 * * @param State - Whether to Activate or Deactivate.
	 */
	static void SwitchActorState(AActor* Actor, ESwitchState State);

	/** Periodically checks if the pool has too many unused objects and destroys them. */
	void ShrinkRoutine();

	virtual void BeginDestroy() override;
	
public:	
	/**
	 * Initializes the pool with the provided settings.
	 * Pre-warms the pool by spawning the initial number of actors.
	 * * @param InPoolSettings - Configuration struct containing class, size, etc.
	 */
	void SetupPoolObject(const FPoolSettings& InPoolSettings);

	/**
	 * Retrieves an actor from the pool and places it at the specified transform.
	 * If the pool is empty and expandable, it will create new objects.
	 * * @param SpawnTransform - The location/rotation to spawn at.
	 * * @return The spawned AActor, or nullptr if the pool is full and fixed-size.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pool|Object")
	AActor* SpawnFromPool(const FTransform& SpawnTransform);

	/**
	 * Templated convenience wrapper for SpawnFromPool.
	 * Automatically casts the result to the specific Actor type.
	 */
	template<typename T>
	T* SpawnFromPool(const FTransform& SpawnTransform)
	{
		return Cast<T>(SpawnFromPool(SpawnTransform));
	}

	/**
	 * Checks if it is possible to spawn an object.
	 * Returns true if there are free objects or if the pool is allowed to grow.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pool|Object", BlueprintPure)
	bool CanSpawn() const;

	/**
	 * Returns an active actor back to the pool.
	 * This deactivates the actor and makes it available for future SpawnFromPool calls.
	 * * @param PoolableActor - The actor to return. Must implement IPoolable.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pool|Object")
	void ReturnToPool(AActor* PoolableActor);
	
};