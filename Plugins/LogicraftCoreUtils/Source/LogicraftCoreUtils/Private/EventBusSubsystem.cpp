// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "EventBusSubsystem.h"

UEventBusSubsystem::ThisClass* UEventBusSubsystem::Get(const UObject* WorldContext)
{
	if (UWorld* World{ GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) })
	{
		return World->GetSubsystem<ThisClass>();
	}

	return nullptr;
}

bool UEventBusSubsystem::Remove(const FGameplayTag& GameplayTag, FDelegateHandle DelegateHandle)
{
	if (!DelegateHandle.IsValid())
	{
		return false;
	}

	if (auto BaseEventContainer{ Internal_Find(GameplayTag) })
	{
		const bool Result{ BaseEventContainer->Remove(DelegateHandle) };
		if (BaseEventContainer->GetSubscriberCount() == 0)
		{
			Internal_Remove(GameplayTag);
		}

		return Result;
	}

	return false;
}

int32 UEventBusSubsystem::RemoveAll(const FGameplayTag& GameplayTag, const void* UserObject)
{
	if (!UserObject)
	{
		return 0;
	}
		
	if (auto BaseEventContainer{ Internal_Find(GameplayTag) })
	{
		const int32 RemovedSubscriber{ BaseEventContainer->RemoveAll(UserObject) };
		BaseEventContainer->SetSubscriberCount(BaseEventContainer->GetSubscriberCount() - RemovedSubscriber);
		if (BaseEventContainer->GetSubscriberCount() == 0)
		{
			Internal_Remove(GameplayTag);
		}
			
		return RemovedSubscriber;            	
	}

	return 0;
}

bool UEventBusSubsystem::IsBound(const FGameplayTag& GameplayTag) const
{
	const auto BaseEventContainer{ Internal_Find(GameplayTag) };
	return BaseEventContainer ? BaseEventContainer->GetSubscriberCount() > 0U : false;
}

bool UEventBusSubsystem::IsBoundToObject(const FGameplayTag& GameplayTag, const void* UserObject) const
{
	const auto BaseEventContainer{ Internal_Find(GameplayTag) };    
	return BaseEventContainer ? BaseEventContainer->IsBoundToObject(UserObject) : false;
}

TSharedPtr<IEventContainerBase> UEventBusSubsystem::Internal_Find(const FGameplayTag& GameplayTag) const
{
	const auto* FoundContainerBase{ ActiveEvents.Find(GameplayTag) };
	return FoundContainerBase ? FoundContainerBase->ToSharedPtr() : nullptr;
}

bool UEventBusSubsystem::Internal_Remove(const FGameplayTag& GameplayTag)
{
	return ActiveEvents.Remove(GameplayTag) > 0;
}
