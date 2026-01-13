// Fill out your copyright notice in the Description page of Project Settings.


#include "TimerHolder.h"

FTimerHolder::~FTimerHolder()
{
	if (!TimerManager)
	{
		return;
	}

	TimerManager->ClearTimer(TimerHandle);
}

FTimerHandle FTimerHolder::ScheduleTimer(const FTimerDelegate& TimerDelegate, const FTimerParameters TimerParameters)
{
	RetrieveTimerManager();
	
	TimerManager->SetTimer(TimerHandle, TimerDelegate, TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);
	return TimerHandle;
}

FTimerHandle FTimerHolder::ScheduleTimer(const FTimerDynamicDelegate& TimerDelegate, const FTimerParameters TimerParameters)
{
	RetrieveTimerManager();
	
	TimerManager->SetTimer(TimerHandle, TimerDelegate, TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);
	return TimerHandle;
}

FTimerHandle FTimerHolder::ScheduleTimer(TFunction<void()>&& TimerDelegate, const FTimerParameters TimerParameters)
{
	RetrieveTimerManager();
	
	TimerManager->SetTimer(TimerHandle, MoveTemp(TimerDelegate), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);
	return TimerHandle;
}

void FTimerHolder::RetrieveTimerManager()
{
	if (TimerManager)
	{
		return;
	}
	
	if (UWorld* World{ GEngine->GetWorld() })
	{
		TimerManager = &World->GetTimerManager();
	}
}
