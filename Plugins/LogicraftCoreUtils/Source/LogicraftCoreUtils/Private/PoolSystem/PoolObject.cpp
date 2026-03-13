// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "PoolSystem/PoolObject.h"

#include "PoolSystem/Poolable.h"
#include "Engine/World.h"

AActor* UPoolObject::GetActor(const IPoolable* Poolable)
{
	return static_cast<AActor*>(Poolable->_getUObject());
}


void UPoolObject::AddNewChunk()
{
	PoolArray.Reserve(PoolArray.Num() + PoolSettings.MinPoolSize);
	int InitialSize = PoolArray.Num();
	for (int i = PoolArray.Num(); i < InitialSize + PoolSettings.MinPoolSize; ++i)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PoolArray.Add(FPoolableInfo{.Poolable = PoolSettings.WorldContext.Get()->SpawnActor<IPoolable>(PoolSettings.SpawnClass, SpawnParameters)});
		PoolArray[i].Poolable->Internal_SetIndex(i);
		SwitchActorState(GetActor(PoolArray[i].Poolable), ESwitchState::Deactivate);
		IndexQueue.Enqueue(i);
	}
}

bool UPoolObject::FindNextIndex()
{
	if (IndexQueue.IsEmpty())
	{
		NextIndex = NullOpt;
	}
	else
	{
		NextIndex = *IndexQueue.Peek();
		IndexQueue.Pop();
	}
	
	return NextIndex.IsSet();
}

IPoolable* UPoolObject::SpawnActor(const FTransform& SpawnTransform)
{
	auto& [Poolable, bIsFree, ReturnTime] = PoolArray[*NextIndex];
	bIsFree = false;
	if (!ensureMsgf(Poolable && IsValid(Poolable->_getUObject()), TEXT("Selected pool actor is null!")))
	{
		return nullptr;
	}
	
	AActor* PoolableActor = GetActor(Poolable);
	PoolableActor->SetActorTransform(SpawnTransform);
	SwitchActorState(PoolableActor, ESwitchState::Activate);

	IPoolable::Execute_OnSpawn(PoolableActor);
	
	FindNextIndex();
	
	return Poolable;
}

void UPoolObject::ReturnActor(int Index)
{
	auto& [Poolable, bIsFree, ReturnTime] = PoolArray[Index];
	if (bIsFree)
	{
		return;
	}
	
	bIsFree = true;
	ReturnTime = 0.f; 
	
	if (!ensureMsgf(Poolable && IsValid(Poolable->_getUObject()), TEXT("Selected pool actor is null!")))
	{
		return;
	}
	
	AActor* PoolableActor = GetActor(Poolable);
	SwitchActorState(PoolableActor, ESwitchState::Deactivate);

	IndexQueue.Enqueue(Index);
	
	IPoolable::Execute_OnReturn(PoolableActor);
}

void UPoolObject::SwitchActorState(AActor* Actor, const ESwitchState State)
{
	switch (State) {
	case ESwitchState::Activate:
		Actor->SetActorHiddenInGame(false);
		Actor->SetActorTickEnabled(true);
		Actor->SetActorEnableCollision(true);
		break;
	case ESwitchState::Deactivate:
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorTickEnabled(false);
		Actor->SetActorEnableCollision(false);
		break;
	}
}

void UPoolObject::ShrinkRoutine()
{
	IndexQueue.Empty();
	NextIndex = NullOpt;
	
	for (int i = 0; i < PoolArray.Num(); ++i)
	{
		FPoolableInfo& PoolableInfo = PoolArray[i];
		PoolableInfo.ReturnTime += PoolSettings.AutoShrinkUpdateTime;

		const bool bCanShrinkSize = PoolArray.Num() > PoolSettings.MinPoolSize;
		const bool bIsTooOld = PoolableInfo.ReturnTime >= PoolSettings.ShrinkObjectAfterReturnTime && PoolableInfo.bIsFree;
		
		if (bCanShrinkSize && bIsTooOld)
		{
			AActor* ActorToDestroy = GetActor(PoolableInfo.Poolable);
			if (IsValid(ActorToDestroy))
			{
				ActorToDestroy->Destroy();
			}

			PoolArray.RemoveAtSwap(i);
			i--;
		}
		else
		{
			IndexQueue.Enqueue(i);
			PoolArray[i].Poolable->Internal_SetIndex(i);
		}
	}
	FindNextIndex();
}

void UPoolObject::BeginDestroy()
{
	for (auto Element : PoolArray)
	{
		GetActor(Element.Poolable)->Destroy();
	}
	Super::BeginDestroy();
}

void UPoolObject::SetupPoolObject(const FPoolSettings& InPoolSettings)
{
	PoolSettings = InPoolSettings;
	if (!ensureMsgf(PoolSettings.WorldContext.IsValid(), TEXT("World Context of the pool is null!")))
	{
		return;
	}
	AddNewChunk();

	if (InPoolSettings.bAutoShrink)
	{
		ShrinkRoutineTimer.Schedule(this, &ThisClass::ShrinkRoutine,FTimerParameters{
		.bIsLooping = true,
		.Rate = InPoolSettings.AutoShrinkUpdateTime,
		.FirstDelay = InPoolSettings.AutoShrinkUpdateTime});
	}	
}

AActor* UPoolObject::SpawnFromPool(const FTransform& SpawnTransform)
{
	if (!ensureMsgf(PoolSettings.WorldContext.IsValid(), TEXT("World Context of the pool is null!")))
	{
		return nullptr;
	}
	
	if (!NextIndex && !FindNextIndex())
	{
		if (!PoolSettings.bAllowResize)
		{
			checkf(false, TEXT("Pool capacity exceeded and resize is disabled for this pool. Returning nullptr."));
			return nullptr;
		}
		else
		{
			AddNewChunk();
			FindNextIndex();
		}
	}
	

	if (!ensureMsgf(NextIndex, TEXT("Couldn't find or allocate next actor!")))
	{
		return nullptr;
	}
	
	IPoolable* SpawnedPoolable = SpawnActor(SpawnTransform);
	if (!ensureMsgf(SpawnedPoolable, TEXT("Failed to spawn actor from pool!")))
	{
		return nullptr;
	}
	return GetActor(SpawnedPoolable);
}

bool UPoolObject::CanSpawn() const
{
	if (!PoolSettings.WorldContext.IsValid())
	{
		return false;
	}
	if (PoolSettings.bAllowResize)
	{
		return true;
	}
	if (NextIndex)
	{
		return true;
	}
	if (!IndexQueue.IsEmpty())
	{
		return true;
	}
	return false;
}

void UPoolObject::ReturnToPool(AActor* PoolableActor)
{
	if (!PoolableActor || !PoolableActor->Implements<UPoolable>())
	{
		return;
	}
	
	IPoolable* Poolable = Cast<IPoolable>(PoolableActor);

	const int32 Index = Poolable ? Poolable->Internal_GetIndex() : INDEX_NONE;
	if (!ensureMsgf(Poolable && PoolArray.IsValidIndex(Index) && PoolArray[Index].Poolable == Poolable,
	TEXT("Attempted to return a poolable that does not belong to this pool or has an invalid index (Index: %d)."), Index))
	{
		return;
	}

	
	ReturnActor(Index);
}
 