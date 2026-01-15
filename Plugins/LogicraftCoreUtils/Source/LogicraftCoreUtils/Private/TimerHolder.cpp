// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "TimerHolder.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define TIMER_HOLDER_CHECK() \
		checkf(TimerManager != nullptr, TEXT("The timer manager was not retrieved prior to using this function."))

#define TIMER_HOLDER_ENSURE() \
		ensureMsgf(RetrieveTimerManager(), TEXT("Unable to retrieve the timer manager because no valid context was found."))

FTimerHolder::~FTimerHolder()
{
	if (RetrieveTimerManager())
	{
		TimerManager->ClearTimer(TimerHandle);	
	}
}

void FTimerHolder::Pause()
{
	if (TIMER_HOLDER_ENSURE())
	{
		TimerManager->PauseTimer(TimerHandle);
	}
}

void FTimerHolder::Clear()
{
	if (TIMER_HOLDER_ENSURE())
	{
		TimerManager->ClearTimer(TimerHandle);
	}
}

bool FTimerHolder::IsPaused() const
{
	TIMER_HOLDER_CHECK();
	return TimerManager->IsTimerPaused(TimerHandle);
}

bool FTimerHolder::IsAlreadyRunning() const
{
	TIMER_HOLDER_CHECK();
	return TimerManager->TimerExists(TimerHandle);
}

float FTimerHolder::GetElapsedTime() const
{
	TIMER_HOLDER_CHECK();
	return TimerManager->GetTimerElapsed(TimerHandle);
}

float FTimerHolder::GetRate() const
{
	TIMER_HOLDER_CHECK();
	return TimerManager->GetTimerRate(TimerHandle);
}

float FTimerHolder::GetRemainingTime() const
{
	TIMER_HOLDER_CHECK();
	return TimerManager->GetTimerRemaining(TimerHandle);
}

bool FTimerHolder::RetrieveTimerManager()
{
	if (TimerManager)
	{
		return true;
	}
	
	if (!GEngine)
	{
		return false;
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
