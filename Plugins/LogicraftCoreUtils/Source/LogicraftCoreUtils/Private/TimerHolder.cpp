// Fill out your copyright notice in the Description page of Project Settings.


#include "TimerHolder.h"

FTimerHolder::~FTimerHolder()
{
	Clear();
}

void FTimerHolder::Pause()
{
	if (ensureMsgf(RetrieveTimerManager(), TEXT("Unable to retrieve the timer manager because no valid context was found.")))
	{
		TimerManager->PauseTimer(TimerHandle);
	}
}

void FTimerHolder::Clear()
{
	if (ensureMsgf(RetrieveTimerManager(), TEXT("Unable to retrieve the timer manager because no valid context was found.")))
	{
		TimerManager->ClearTimer(TimerHandle);
	}
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
