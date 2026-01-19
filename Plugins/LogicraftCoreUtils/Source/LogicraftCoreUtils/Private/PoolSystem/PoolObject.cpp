// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "PoolSystem/PoolObject.h"

#include "PoolSystem/Poolable.h"
 
void UPoolObject::AddNewChunk()
{
	PoolArray.Reserve(PoolArray.Num() + PoolSettings.MinPoolSize);
	NextIndex = PoolArray.Num();
	for (int i = PoolArray.Num(); i < PoolArray.Num() - 1 + PoolSettings.MinPoolSize; ++i)
	{
		PoolArray.Add(FPoolableInfo{.Poolable = PoolSettings.WorldContext.Get()->SpawnActor<IPoolable>(PoolSettings.SpawnClass)});
	}
}

bool UPoolObject::FindNextIndex()
{
	if (NextIndex)
	{
		bool bHasFound = false;
		for (int i = *NextIndex; i < PoolArray.Num(); ++i)
		{
			if (PoolArray[i].bIsFree)
			{
				NextIndex = i;
				bHasFound = true;
				break;
			}
		}

		if (!bHasFound)
		{
			for (int i = 0; i < *NextIndex; ++i)
			{
				if (PoolArray[i].bIsFree)
				{
					NextIndex = i;
					break;
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < PoolArray.Num(); ++i)
		{
			if (PoolArray[i].bIsFree)
			{
				NextIndex = i;
				break;
			}
		}
	}
	return NextIndex.IsSet();
}

IPoolable* UPoolObject::SpawnActor(const FTransform& SpawnTransform)
{
	IPoolable* Poolable = PoolArray[*NextIndex].Poolable;
	PoolArray[*NextIndex].bIsFree = false;
	if (ensureMsgf(Poolable && Poolable->_getUObject()->IsValidLowLevel(), TEXT("Selected pool actor is null!")))
	{
		return nullptr;
	}
	
	AActor* PoolableActor = static_cast<AActor*>(Poolable->_getUObject());
	PoolableActor->SetActorTransform(SpawnTransform);
	SwitchActorState(PoolableActor, ESwitchState::Activate);

	IPoolable::Execute_OnSpawn(PoolableActor);
	
	return Poolable;
}

void UPoolObject::ReturnActor(int Index)
{
	const IPoolable* Poolable = PoolArray[Index].Poolable;
	PoolArray[*NextIndex].bIsFree = true;
	if (ensureMsgf(Poolable && !Poolable->_getUObject()->IsValidLowLevel(), TEXT("Selected pool actor is null!")))
	{
		return;
	}
	
	AActor* PoolableActor = static_cast<AActor*>(Poolable->_getUObject());
	SwitchActorState(PoolableActor, ESwitchState::Deactivate);

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
	for (int i = PoolSettings.MinPoolSize; i < PoolArray.Num(); ++i)
	{
		FPoolableInfo& PoolableInfo = PoolArray[i];
		PoolableInfo.ReturnTime += PoolSettings.AutoShrinkUpdateTime;

		if (PoolableInfo.ReturnTime >= PoolSettings.ShrinkObjectAfterReturnTime)
		{
			PoolArray.RemoveAt(i--);
		}
	}
}

void UPoolObject::SetupPoolObject(const FPoolSettings& InPoolSettings)
{
	PoolSettings = InPoolSettings;
	if (ensureMsgf(PoolSettings.WorldContext.IsValid(), TEXT("World Context of the pool is null!")))
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

TScriptInterface<IPoolable> UPoolObject::Spawn(const FTransform& SpawnTransform)
{
	if (ensureMsgf(PoolSettings.WorldContext.IsValid(), TEXT("World Context of the pool is null!")))
	{
		return nullptr;
	}
	
	if (!NextIndex && !FindNextIndex())
	{		 
		AddNewChunk();
	}

	TScriptInterface<IPoolable> Result;
	if (ensureMsgf(NextIndex, TEXT("Couldn't find or allocate next actor!")))
	{
		return Result;
	}
	
	Result.SetInterface(SpawnActor(SpawnTransform));
	
	return Result;
}

void UPoolObject::Return(TScriptInterface<IPoolable> Poolable)
{
	if (!Poolable)
	{
		return;
	}
	auto FindPredicate = [=](const FPoolableInfo& PoolableInfo){return PoolableInfo.Poolable == Poolable.GetInterface();};
	
	if (ensureMsgf(
		PoolArray.ContainsByPredicate(FindPredicate),
		TEXT("This poolable object is not from this pool!")))
	{
		return;
	}
 
	ReturnActor(PoolArray.IndexOfByPredicate(FindPredicate));
}
