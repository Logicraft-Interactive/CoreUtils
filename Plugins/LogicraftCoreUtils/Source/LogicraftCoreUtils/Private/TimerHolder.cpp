// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "TimerHolder.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

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

bool FTimerHolder::IsPaused()
{
	if (TIMER_HOLDER_ENSURE())
	{
		return TimerManager->IsTimerPaused(TimerHandle);	
	}

	return false;
}

bool FTimerHolder::IsAlreadyRunning()
{
	if (TIMER_HOLDER_ENSURE())
	{
		return TimerManager->TimerExists(TimerHandle);	
	}

	return false;
}

float FTimerHolder::GetElapsedTime()
{
	if (TIMER_HOLDER_ENSURE())
	{
		return TimerManager->GetTimerElapsed(TimerHandle);	
	}

	return 0.f;
}

float FTimerHolder::GetRate()
{
	if (TIMER_HOLDER_ENSURE())
	{
		return TimerManager->GetTimerRate(TimerHandle);
	}

	return 0.f;
}

float FTimerHolder::GetRemainingTime()
{
	if (TIMER_HOLDER_ENSURE())
	{
		return TimerManager->GetTimerRemaining(TimerHandle);
	}

	return 0.f;
}

bool FTimerHolder::RetrieveTimerManager()
{
	if (TimerManager)
	{
		return true;
	}
	
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
			{
				TimerManager = &Context.World()->GetTimerManager();
				return true;
			}
		}
	}
	
#if WITH_EDITOR
	if (!GEditor || !GEditor->GetEditorWorldContext().World())
	{
		return false;
	}

	TimerManager = &GEditor->GetEditorWorldContext().World()->GetTimerManager();

	return true;
#else
	return false;
#endif
	
	
}
