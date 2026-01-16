// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "EventBus.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UEventBus::ThisClass* UEventBus::Get(const UObject* WorldContext)
{
	if (UWorld* World{ GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) })
	{
		return World->GetSubsystem<ThisClass>();
	}

	return nullptr;
}

bool UEventBus::Remove(const UObject* WorldContext, const FGameplayTag& GameplayTag, FDelegateHandle Handle)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	return Internal_ExecuteOnValidContext(WorldContext, [&GameplayTag, &Handle](ThisClass* EventBus)
	{
		if (const auto BaseEventContainer{ EventBus->Internal_Find(GameplayTag) })
		{
			const bool Result{ BaseEventContainer->Remove(Handle) };
			if (BaseEventContainer->GetSubscriberCount() == 0)
			{
				EventBus->Internal_Remove(GameplayTag);
			}

			return Result;
		}

		return false;
	});
}

int32 UEventBus::RemoveAll(const UObject* WorldContext, const FGameplayTag& GameplayTag, const void* UserObject)
{
	if (!UserObject)
	{
		return 0;
	}

	return Internal_ExecuteOnValidContext(WorldContext, [&GameplayTag, UserObject](ThisClass* EventBus)
	{
		if (const auto BaseEventContainer{ EventBus->Internal_Find(GameplayTag) })
		{
			const int32 RemovedSubscriber{ BaseEventContainer->RemoveAll(UserObject) };
			BaseEventContainer->SetSubscriberCount(BaseEventContainer->GetSubscriberCount() - RemovedSubscriber);
			if (BaseEventContainer->GetSubscriberCount() == 0)
			{
				EventBus->Internal_Remove(GameplayTag);
			}
		
			return RemovedSubscriber;            	
		}

		return 0;
	});
}

bool UEventBus::IsBound(const UObject* WorldContext, const FGameplayTag& GameplayTag)
{
	return Internal_ExecuteOnValidContext(WorldContext, [&GameplayTag](const ThisClass* EventBus)
	{
		const auto BaseEventContainer{ EventBus->Internal_Find(GameplayTag) };
		return BaseEventContainer ? BaseEventContainer->GetSubscriberCount() > 0U : false;	
	});
}

bool UEventBus::IsBoundToObject(const UObject* WorldContext, const FGameplayTag& GameplayTag, const void* UserObject)
{
	if (!UserObject)
	{
		return false;
	}
	
	return Internal_ExecuteOnValidContext(WorldContext, [&GameplayTag, &UserObject](const ThisClass* EventBus)
	{
		const auto BaseEventContainer{ EventBus->Internal_Find(GameplayTag) };    
		return BaseEventContainer ? BaseEventContainer->IsBoundToObject(UserObject) : false;
	});
}

TSharedPtr<IEventContainerBase> UEventBus::Internal_Find(const FGameplayTag& GameplayTag) const
{
	const auto* FoundContainerBase{ ActiveEvents.Find(GameplayTag) };
	return FoundContainerBase ? FoundContainerBase->ToSharedPtr() : nullptr;
}

bool UEventBus::Internal_Remove(const FGameplayTag& GameplayTag)
{
	return ActiveEvents.Remove(GameplayTag) > 0;
}
