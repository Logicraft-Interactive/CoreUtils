// Fill out your copyright notice in the Description page of Project Settings.


#include "TimerHolder.h"

FTimerHolder::~FTimerHolder()
{
	if (TimerManager)
	{
		Clear();	
	}
}

void FTimerHolder::Pause() const
{
	check(TimerManager != nullptr);
	TimerManager->PauseTimer(TimerHandle);
}

void FTimerHolder::Clear()
{
	check(TimerManager != nullptr);
	TimerManager->ClearTimer(TimerHandle);
}

bool FTimerHolder::IsPaused() const
{
	check(TimerManager != nullptr);
	return TimerManager->IsTimerPaused(TimerHandle);
}

bool FTimerHolder::IsAlreadyRunning() const
{
	check(TimerManager != nullptr);
	return TimerManager->TimerExists(TimerHandle);
}

float FTimerHolder::GetElapsedTime() const
{
	check(TimerManager != nullptr);
	return TimerManager->GetTimerElapsed(TimerHandle);
}

float FTimerHolder::GetRate() const
{
	check(TimerManager != nullptr);
	return TimerManager->GetTimerRate(TimerHandle);
}

float FTimerHolder::GetRemainingTime() const
{
	check(TimerManager != nullptr);
	return TimerManager->GetTimerRemaining(TimerHandle);
}

bool FTimerHolder::RetrieveTimerManager()
{
	if (TimerManager)
	{
		return true;
	}
	
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
		{
			TimerManager = &Context.World()->GetTimerManager();
			return true;
		}
	}
	
	return false;
}
