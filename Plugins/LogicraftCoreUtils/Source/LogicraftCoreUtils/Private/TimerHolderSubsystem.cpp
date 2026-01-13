#include "TimerHolderSubsystem.h"

UTimerHolderSubsystem::ThisClass* UTimerHolderSubsystem::Get(const UObject* WorldContext)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		return World->GetSubsystem<ThisClass>();
	}

	return nullptr;
}

FTimerHandle UTimerHolderSubsystem::ScheduleTimer(UObject* WorldContext, const FTimerDelegate& TimerDelegate,
                                                  FTimerParameters TimerParameters, ETimerScope TimerScope)
{
	if (Internal_CheckRate(TimerParameters.Rate))
	{
		return {};
	}

	//TODO : Put chain verification for null pointer.
	
	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };

	TSharedRef TimerHandle{ MakeShared<FTimerHandle>() };
	return TimerHolderSubsystem->Internal_LaunchAndRegister(WorldContext, TimerParameters, TimerScope,
	                                                        TimerHandle, TimerDelegate);
}

FTimerHandle UTimerHolderSubsystem::ScheduleTimer(UObject* WorldContext, const FTimerDynamicDelegate& TimerDelegate,
                                                  FTimerParameters TimerParameters, ETimerScope TimerScope)
{
	if (Internal_CheckRate(TimerParameters.Rate))
	{
		return {};
	}

	//TODO : Put chain verification for null pointer.

	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };

	TSharedRef TimerHandle{ MakeShared<FTimerHandle>() };
	return TimerHolderSubsystem->Internal_LaunchAndRegister(WorldContext, TimerParameters, TimerScope,
	                                                        TimerHandle, TimerDelegate);
}

FTimerHandle UTimerHolderSubsystem::ScheduleTimer(UObject* WorldContext, TFunction<void()>&& TimerDelegate,
                                                  FTimerParameters TimerParameters, ETimerScope TimerScope)
{
	if (Internal_CheckRate(TimerParameters.Rate))
	{
		return {};
	}

	//TODO : Put chain verification for null pointer.

	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };

	TSharedRef TimerHandle{ MakeShared<FTimerHandle>() };
	return TimerHolderSubsystem->Internal_LaunchAndRegister(WorldContext, TimerParameters, TimerScope,
	                                                        TimerHandle,
	                                                        TimerDelegate);
}

void UTimerHolderSubsystem::CancelTimer(UObject* Object)
{
	//TODO : Put chain verification for null pointer.
	
	ThisClass* TimerHolderSubsystem{ Get(Object) };

	FActiveTimerRegistry TimerHandleSet;
	if (TimerHolderSubsystem->OwnerBoundTimers.RemoveAndCopyValue(Object, TimerHandleSet))
	{
		for (auto& TimerHandle : TimerHandleSet)
		{
			TimerHolderSubsystem->GetTimerManager().ClearTimer(TimerHandle);
		}
	}
}

void UTimerHolderSubsystem::CancelTimer(const UObject* WorldContext, FTimerHandle& TimerHandle, ETimerScope TimerScope)
{
	//TODO : Put chain verification for null pointer.
	
	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };

	if (TimerScope == ETimerScope::Global)
	{
		if (TimerHolderSubsystem->UnboundActiveTimers.Remove(TimerHandle))
		{
			TimerHolderSubsystem->GetTimerManager().ClearTimer(TimerHandle);	
		}
		return;
	}

	if (FActiveTimerRegistry* ActiveTimerRegistry{ TimerHolderSubsystem->OwnerBoundTimers.Find(WorldContext) })
	{
		if (ActiveTimerRegistry->Remove(TimerHandle))
		{
			TimerHolderSubsystem->GetTimerManager().ClearTimer(TimerHandle);	
		}
	}
}

void UTimerHolderSubsystem::Deinitialize()
{
	Super::Deinitialize();

	for (FTimerHandle& TimerHandle : UnboundActiveTimers)
	{
		GetTimerManager().ClearTimer(TimerHandle);
	}

	for (auto& TimerHandleWithContext : OwnerBoundTimers)
	{
		for (auto& TimerHandle : TimerHandleWithContext.Value)
		{
			GetTimerManager().ClearTimer(TimerHandle);
		}
	}

	UnboundActiveTimers.Empty();
}

bool UTimerHolderSubsystem::Internal_CheckRate(const float InRate)
{
	return FMath::IsNearlyZero(InRate) || InRate < 0.f;
}

FTimerManager& UTimerHolderSubsystem::GetTimerManager() const
{
	return GetWorld()->GetTimerManager();
}
