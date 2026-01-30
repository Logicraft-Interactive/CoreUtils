// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Poolable.h"
#include "PoolSettings.h"
#include "UObject/Object.h"
#include "TimerHolder.h"
#include "PoolObject.generated.h"

class IPoolable;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class LOGICRAFTCOREUTILS_API UPoolObject : public UObject
{
	GENERATED_BODY()

	struct FPoolableInfo
	{
		IPoolable* Poolable{nullptr};
		bool bIsFree{true};
		float ReturnTime{0.f}; 
	};

	enum class ESwitchState
	{
		Activate,
		Deactivate
	};
	
	FPoolSettings PoolSettings;
	TArray<FPoolableInfo> PoolArray;
	TOptional<int> NextIndex{NullOpt};
	FTimerHolder ShrinkRoutineTimer;

	static AActor* GetActor(const IPoolable* Poolable);
	
	void AddNewChunk();

	bool FindNextIndex();

	IPoolable* SpawnActor(const FTransform& SpawnTransform);
	void ReturnActor(int Index);

	static void SwitchActorState(AActor* Actor,ESwitchState State);

	void ShrinkRoutine();

	TQueue<int> IndexQueue;

	virtual void BeginDestroy() override;
	
public:	
	void SetupPoolObject(const FPoolSettings& InPoolSettings);

	UFUNCTION(BlueprintCallable, Category = "Pool|Object")
	AActor* SpawnFromPool(const FTransform& SpawnTransform);

	template<typename T>
	T* SpawnFromPool(const FTransform& SpawnTransform)
	{
		return Cast<T>(SpawnFromPool(SpawnTransform));
	}

	UFUNCTION(BlueprintCallable, Category = "Pool|Object", BlueprintPure)
	bool CanSpawn() const;

	UFUNCTION(BlueprintCallable, Category = "Pool|Object")
	void ReturnToPool(AActor* PoolableActor);
	
};