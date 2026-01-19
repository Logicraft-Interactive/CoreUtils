// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PoolSettings.h"
#include "UObject/Object.h"
#include "TimerHolder.h"
#include "PoolObject.generated.h"

class IPoolable;
/**
 * 
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UPoolObject : public UObject
{
	GENERATED_BODY()

	struct FPoolableInfo
	{
		IPoolable* Poolable = nullptr;
		bool bIsFree = true;
		float ReturnTime = 0.f;
	};

	enum class ESwitchState
	{
		Activate,
		Deactivate
	};
	
	FPoolSettings PoolSettings;
	TArray<FPoolableInfo> PoolArray;
	TOptional<int> NextIndex = NullOpt;
	FTimerHolder ShrinkRoutineTimer;

	void AddNewChunk();

	bool FindNextIndex();

	IPoolable* SpawnActor(const FTransform& SpawnTransform);
	void ReturnActor(int Index);

	static void SwitchActorState(AActor* Actor,ESwitchState State);

	void ShrinkRoutine();
	
public:	
	void SetupPoolObject(const FPoolSettings& InPoolSettings);

	UFUNCTION(BlueprintCallable)
	TScriptInterface<IPoolable> Spawn(const FTransform& SpawnTransform);

	template<typename T>
	T* Spawn(const FTransform& SpawnTransform)
	{
		return Cast<T>(Spawn(SpawnTransform).GetObject());
	}


	UFUNCTION(BlueprintCallable)
	void Return(TScriptInterface<IPoolable> Poolable);
};