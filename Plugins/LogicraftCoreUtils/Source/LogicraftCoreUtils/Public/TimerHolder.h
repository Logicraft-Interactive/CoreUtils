// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

namespace Concepts
{
	template <typename Ty>
	concept DerivedFromActor = std::derived_from<Ty, AActor>;

	template <typename Ty>
	concept DerivedFromObject = std::derived_from<Ty, UObject>;

	template<typename TCallable, typename ...TArgs>
	concept Invocable = std::invocable<TCallable, TArgs...>;
} // Concepts

namespace TypeTraits
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
 * 
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
		if (RetrieveTimerManager())
		{
			TimerManager->SetTimer(TimerHandle, Forward<TCallable>(TimerCallback), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);	
		}
		
		return TimerHandle;
	}

	template <Concepts::DerivedFromObject TObject, typename TMethod>
		requires Concepts::Invocable<TMethod, TObject>
	FTimerHandle Schedule(TObject* Object, TMethod&& TimerCallback, const FTimerParameters TimerParameters)
	{
		if (RetrieveTimerManager())
		{
			TimerManager->SetTimer(TimerHandle, Object, Forward<TMethod>(TimerCallback), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);	
		}
		
		return TimerHandle;
	}

	void Pause() const;
	void Clear();

	bool IsPaused() const;
	bool IsAlreadyRunning() const;

	float GetElapsedTime() const;
	float GetRate() const;
	float GetRemainingTime() const;

private:
	bool RetrieveTimerManager();
};
