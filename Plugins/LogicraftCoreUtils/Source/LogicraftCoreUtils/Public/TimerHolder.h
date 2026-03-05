// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Engine/EngineTypes.h"
#include "Engine/TimerHandle.h"
#include "Meta/LCUConcepts.h"
#include "GameFramework/Actor.h"

namespace TypeTrait
{
	template<typename TCallable, typename ...TArgs>
	static constexpr bool IsInvocable_V = std::is_invocable_v<TCallable, TArgs...>;
} // TypeTraits

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

	template<typename TCallable>
	FTimerHandle Schedule(TCallable&& TimerCallback, const FTimerParameters TimerParameters)
	{
		if (ensureMsgf(RetrieveTimerManager(), TEXT("Unable to retrieve the timer manager because no valid context was found.")))
		{
			TimerManager->SetTimer(TimerHandle, Forward<TCallable>(TimerCallback), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);	
		}
		
		return TimerHandle;
	}

	template <Concept::DerivedFromObject TObject, typename TMethod>
		requires Concept::Invocable<TMethod, TObject>
	FTimerHandle Schedule(TObject* Object, TMethod&& TimerCallback, const FTimerParameters TimerParameters)
	{
		if (ensureMsgf(RetrieveTimerManager(), TEXT("Unable to retrieve the timer manager because no valid context was found.")))
		{
			TimerManager->SetTimer(TimerHandle, Object, Forward<TMethod>(TimerCallback), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);	
		}
		
		return TimerHandle;
	}

	void Pause();
	void Clear();

	bool IsPaused() const;
	bool IsAlreadyRunning() const;

	float GetElapsedTime() const;
	float GetRate() const;
	float GetRemainingTime() const;

private:
	bool RetrieveTimerManager();
};
