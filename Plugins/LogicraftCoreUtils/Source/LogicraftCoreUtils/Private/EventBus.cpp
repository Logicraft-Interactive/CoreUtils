// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "EventBus.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UEventBus::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEventBus::Deinitialize()
{
	Super::Deinitialize();
}

UEventBus::ThisClass* UEventBus::Get(const UObject* WorldContext)
{
	// Resolve the world from the context object, then fetch the subsystem from the GameInstance.
	// Returns nullptr and logs a warning if the world cannot be resolved.
	if (UWorld* World{ GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) })
	{
		return World->GetGameInstance()->GetSubsystem<ThisClass>();
	}

	return nullptr;
}

void UEventBus::UnlockSignature(const FGameplayTag& GameplayTag)
{
	// Write lock: we may remove the container from the map.
	FWriteScopeLock WriteScopeLock{ RWLock };
	if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
	{
		BaseEventContainer->SetLockedSignature(false);

		// If nobody is subscribed anymore, the container serves no purpose — clean it up.
		if (BaseEventContainer->GetSubscriberCount() == 0)
		{
			Internal_Remove(GameplayTag);
		}
	}
}

bool UEventBus::Remove(const FGameplayTag& GameplayTag, FDelegateHandle Handle)
{
	// Early-out before acquiring a lock: an invalid handle can never match anything.
	if (!Handle.IsValid())
	{
		return false;
	}

	// Write lock: Remove modifies the delegate list and may remove the container from the map.
	FWriteScopeLock WriteScopeLock{ RWLock };
	if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
	{
		const bool Result{ BaseEventContainer->Remove(Handle) };

		// Clean up the container if it is now empty and its signature is not locked.
		if (BaseEventContainer->GetSubscriberCount() == 0 && !BaseEventContainer->GetLockedSignature())
		{
			Internal_Remove(GameplayTag);
		}

		return Result;
	}

	return false;
}

int32 UEventBus::RemoveAll(const FGameplayTag& GameplayTag, const void* UserObject)
{
	// Early-out: a null object pointer cannot match any binding.
	if (!UserObject)
	{
		return 0;
	}

	// Write lock: RemoveAll modifies the delegate list and may remove the container from the map.
	FWriteScopeLock WriteScopeLock{ RWLock };
	if (const auto BaseEventContainer{ Internal_Find(GameplayTag) })
	{
		const int32 RemovedSubscriber{ BaseEventContainer->RemoveAll(UserObject) };

		// Clean up the container if it is now empty and its signature is not locked.
		if (BaseEventContainer->GetSubscriberCount() == 0 && !BaseEventContainer->GetLockedSignature())
		{
			Internal_Remove(GameplayTag);
		}
	
		return RemovedSubscriber;            	
	}

	return 0;
}

bool UEventBus::IsBound(const FGameplayTag& GameplayTag) const
{
	// Read lock: pure query, no modification to the map or containers.
	FReadScopeLock ReadScopeLock{ RWLock };
	const auto BaseEventContainer{ Internal_Find(GameplayTag) };
	return BaseEventContainer ? BaseEventContainer->GetSubscriberCount() > 0 : false;	
}

bool UEventBus::IsBoundToObject(const FGameplayTag& GameplayTag, const void* UserObject) const
{
	// Early-out: a null object can never be bound.
	if (!UserObject)
	{
		return false;
	}
	
	// Read lock: pure query, no modification to the map or containers.
	FReadScopeLock ReadScopeLock{ RWLock };
	const auto BaseEventContainer{ Internal_Find(GameplayTag) };    
	return BaseEventContainer ? BaseEventContainer->IsBoundToObject(UserObject) : false;
}

TSharedPtr<IEventContainerBase> UEventBus::Internal_Find(const FGameplayTag& GameplayTag) const
{
	// Converts the TSharedRef stored in the map to a TSharedPtr so callers can handle
	// the "not found" case as nullptr without a separate validity check.
	// Must be called while holding at least a read lock on RWLock.
	const auto* FoundContainerBase{ ActiveEvents.Find(GameplayTag) };
	return FoundContainerBase ? FoundContainerBase->ToSharedPtr() : nullptr;
}

bool UEventBus::Internal_Remove(const FGameplayTag& GameplayTag)
{
	// Removes the entry from the map. TSharedRef ref-count will drop to zero
	// and the container will be destroyed if no other TSharedPtr holds a reference
	// (e.g. a Broadcast in progress on another thread holding a TSharedPtr copy).
	// Must be called while holding a write lock on RWLock.
	return ActiveEvents.Remove(GameplayTag) > 0;
}