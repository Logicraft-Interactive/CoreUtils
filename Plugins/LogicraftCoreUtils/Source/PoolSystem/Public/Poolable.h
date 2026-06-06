// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Poolable.generated.h"

class UPoolObject;

/**
 * UPoolable
 * * Standard UInterface class required for Unreal Engine reflection.
 * Do not modify this class directly; implement functionality in IPoolable instead.
 */
UINTERFACE(BlueprintType)
class LOGICRAFTCOREUTILS_API UPoolable : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPoolable
 * * Interface for objects that can be managed by an Object Pool.
 * Implementing this interface allows an object to receive callbacks when it is
 * spawned from or returned to the pool, and allows the pool to manage its internal index.
 */
class LOGICRAFTCOREUTILS_API IPoolable
{
	GENERATED_BODY()

	/** * Internal index used by the pool manager to track this object's position within the pool container.
	 * This allows for O(1) removal or swapping if necessary.
	 */
	int Index{0};
	
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/**
	 * Gets the internal pool index of this object.
	 * * @return The current index assigned by the pool manager.
	 */
	int Internal_GetIndex() const;

	/**
	 * Sets the internal pool index of this object.
	 * This should typically only be called by the pool manager itself.
	 * * @param NewIndex - The new index to assign.
	 */
	void Internal_SetIndex(const int NewIndex);
	
	/**
	 * Called when the object is retrieved (spawned) from the pool.
	 * Use this to reset the object's state (e.g., health, ammo, position) 
	 * instead of using BeginPlay, which is only called once upon creation.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Pooling")
	void OnSpawn();

	/**
	 * Called when the object is returned (despawned) to the pool.
	 * Use this to clean up any active effects, timers, or references 
	 * before the object is deactivated.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Pooling")
	void OnReturn();
};