// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "Meta/LCUConcepts.h"
#include "GameFramework/Actor.h"

#define TIMER_HOLDER_ENSURE() \
ensureMsgf(RetrieveTimerManager(), TEXT("Unable to retrieve the timer manager because no valid context was found."))

struct LOGICRAFTCOREUTILS_API FTimerParameters
{
	bool bIsLooping{ false };
	float Rate{ 0.f };
	float FirstDelay{ -1.f };
};

/**
 * A wrapper that contains a timer handle.
 * It uses the RAII idiom to automatically clear the scheduled timer upon destruction.
 */
class LOGICRAFTCOREUTILS_API FTimerHolder
{
	FTimerManager* TimerManager{ nullptr };
	FTimerHandle TimerHandle{};
	
public:
	FTimerHolder() = default;
	~FTimerHolder();

	template<typename TFunctor>
		requires Concept::Invocable<TFunctor>
	FTimerHandle Schedule(TFunctor&& TimerCallback, const FTimerParameters TimerParameters)
	{
		if (TIMER_HOLDER_ENSURE())
		{
			TimerManager->SetTimer(TimerHandle, Forward<TFunctor>(TimerCallback), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);	
		}
		
		return TimerHandle;
	}

	template <Concept::DerivedFromObject TObject, typename TMethod>
		requires Concept::Invocable<TMethod, TObject>
	FTimerHandle Schedule(TObject* Object, TMethod&& TimerCallback, const FTimerParameters TimerParameters)
	{
		if (TIMER_HOLDER_ENSURE())
		{
			TimerManager->SetTimer(TimerHandle, Object, Forward<TMethod>(TimerCallback), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);	
		}
		
		return TimerHandle;
	}

	void Pause();
	void Clear();

	bool IsPaused();
	bool IsAlreadyRunning();

	float GetElapsedTime();
	float GetRate();
	float GetRemainingTime();

private:
	bool RetrieveTimerManager();
};
