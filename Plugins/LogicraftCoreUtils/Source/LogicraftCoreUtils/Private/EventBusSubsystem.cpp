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
