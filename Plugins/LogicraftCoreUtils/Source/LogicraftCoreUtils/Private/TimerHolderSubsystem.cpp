// Fill out your copyright notice in the Description page of Project Settings.


#include "TimerHolderSubsystem.h"

UTimerHolderSubsystem::ThisClass* UTimerHolderSubsystem::Get(const UObject* WorldContext)
{
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		return World->GetSubsystem<ThisClass>();
	}

	return nullptr;
}

FTimerHandle UTimerHolderSubsystem::StartTimer(const UObject* WorldContext, const FTimerDelegate& TimerDelegate, const float Rate, const bool bIsLooping, const float FirstDelay)
{
	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };
	
	FTimerHandle Handle;
	auto TimerCallBack{ [TimerDelegate, TimerHolderSubsystem, Handle]()
	{
		TimerDelegate.ExecuteIfBound();
		TimerHolderSubsystem->TimerHandles.Remove(Handle);
	} };
	TimerHolderSubsystem->GetTimerManager().SetTimer(Handle, TimerDelegate, Rate, bIsLooping, FirstDelay);
	TimerHolderSubsystem->TimerHandles.Add(Handle);
	return Handle;
}

FTimerHandle UTimerHolderSubsystem::StartTimer(const UObject* WorldContext, const FTimerDynamicDelegate& TimerDelegate, const float Rate,
	const bool bIsLooping, const float FirstDelay)
{
	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };
	
	FTimerHandle Handle;
	auto TimerCallBack{ [TimerDelegate, TimerHolderSubsystem, Handle]()
	{
		TimerDelegate.ExecuteIfBound();
		TimerHolderSubsystem->TimerHandles.Remove(Handle);
	} };
	TimerHolderSubsystem->GetTimerManager().SetTimer(Handle, TimerCallBack, Rate, bIsLooping, FirstDelay);
	TimerHolderSubsystem->TimerHandles.Add(Handle);
	return Handle;
}

FTimerHandle UTimerHolderSubsystem::StartTimer(const UObject* WorldContext, TFunction<void()>&& TimerDelegate,
	const float Rate, const bool bIsLooping, const float FirstDelay)
{
	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };
	
	FTimerHandle Handle;
	auto TimerCallBack{ [Delegate = MoveTemp(TimerDelegate), TimerHolderSubsystem, Handle]()
	{
		Delegate();
		TimerHolderSubsystem->TimerHandles.Remove(Handle);
	} };
	TimerHolderSubsystem->GetTimerManager().SetTimer(Handle, MoveTemp(TimerCallBack), Rate, bIsLooping, FirstDelay);
	TimerHolderSubsystem->TimerHandles.Add(Handle);
	return Handle;
}

void UTimerHolderSubsystem::EndTimer(const UObject* WorldContext, FTimerHandle& TimerHandle)
{
	ThisClass* TimerHolderSubsystem{ Get(WorldContext) };
	
	TimerHolderSubsystem->GetTimerManager().ClearTimer(TimerHandle);
	if (TimerHolderSubsystem->TimerHandles.Remove(TimerHandle))
	{
		return;
	}

	if (TimerHandleSet* TimerHandleSet{ TimerHolderSubsystem->ObjectTimerHandle.Find(WorldContext) })
	{
		TimerHandleSet->Remove(TimerHandle);
	}
}

void UTimerHolderSubsystem::Deinitialize()
{
	Super::Deinitialize();

	for (auto& TimerHandle : TimerHandles)
	{
		GetTimerManager().ClearTimer(TimerHandle);
	}

	TimerHandles.Empty();
}

FTimerManager& UTimerHolderSubsystem::GetTimerManager() const
{
	return GetWorld()->GetTimerManager();
}
