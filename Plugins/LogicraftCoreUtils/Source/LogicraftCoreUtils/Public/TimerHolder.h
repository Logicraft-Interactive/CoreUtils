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
}

struct FTimerParameters
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
	FTimerHandle TimerHandle;
public:
	FTimerHolder() = default;
	~FTimerHolder();
	
	FTimerHandle ScheduleTimer(const FTimerDelegate& TimerDelegate, const FTimerParameters TimerParameters);
	FTimerHandle ScheduleTimer(const FTimerDynamicDelegate& TimerDelegate, const FTimerParameters TimerParameters);
	FTimerHandle ScheduleTimer(TFunction<void()>&& TimerDelegate, const FTimerParameters TimerParameters);

	template <Concepts::DerivedFromObject TObject, typename TMethod>
		requires Concepts::Invocable<TMethod, TObject>
	FTimerHandle ScheduleTimer(TObject* Object, TMethod&& TimerDelegate, const FTimerParameters TimerParameters)
	{
		RetrieveTimerManager();
		
		TimerManager->SetTimer(TimerHandle, Object, Forward<TMethod>(TimerDelegate), TimerParameters.Rate, TimerParameters.bIsLooping, TimerParameters.FirstDelay);
		return TimerHandle;
	}

private:
	void RetrieveTimerManager();
};
