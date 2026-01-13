#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerHolderSubsystem.generated.h"

namespace Private
{
	namespace Concepts
	{
		template <typename Ty>
		concept DerivedFromActor = std::derived_from<Ty, AActor>;
	}
} // Details

struct FTimerParameters
{
	bool bIsLooping{ false };
	float Rate{ 0.f };
	float FirstDelay{ -1.f };
};

enum class ETimerScope : uint8
{
	Global,
	ContextBound
};

/**
 * Subsystem designed to centralize and secure timer management.
 * Automatically handles timer cancellation to prevent crashes caused by 
 * execution on invalid objects (garbage collected or destroyed).
 * Provides a safer alternative to the standard FTimerManager for raw C++ classes.
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UTimerHolderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	using FActiveTimerRegistry = TSet<FTimerHandle>;
	using FOwnerBoundTimers = TMap<TWeakObjectPtr<UObject>, FActiveTimerRegistry>;

	FActiveTimerRegistry UnboundActiveTimers;
	FOwnerBoundTimers OwnerBoundTimers;
	
public:
	static ThisClass* Get(const UObject* WorldContext);

	/**
	 * Schedules a new timer based on the provided parameters.
	 * @param WorldContext Object used to retrieve the World. Can be 'this', the World itself or anything linked to the world.
	 * @param TimerDelegate The delegate to execute when the timer expires.
	 * @param TimerParameters Parameters configuration (Rate, Loop, Delay).
	 * @param TimerScope Determines if the timer is standalone (Global) or linked to the WorldContext.
	 * @return A handle to the scheduled timer. Returns an invalid handle if Rate is <= 0.
	 */
	static FTimerHandle ScheduleTimer(UObject* WorldContext, const FTimerDelegate& TimerDelegate,
	                                  FTimerParameters TimerParameters, ETimerScope TimerScope = ETimerScope::Global);

	/**
	 * Schedules a new timer based on the provided parameters.
	 * @param WorldContext Object used to retrieve the World. Can be 'this', the World itself or anything linked to the world.
	 * @param TimerDelegate The delegate to execute when the timer expires.
	 * @param TimerParameters Parameters configuration (Rate, Loop, Delay).
	 * @param TimerScope Determines if the timer is standalone (Global) or linked to the WorldContext.
	 * @return A handle to the scheduled timer. Returns an invalid handle if Rate is <= 0.
	 */
	static FTimerHandle ScheduleTimer(UObject* WorldContext, const FTimerDynamicDelegate& TimerDelegate,
	                                  FTimerParameters TimerParameters, ETimerScope TimerScope = ETimerScope::Global);

	/**
	 * Schedules a new timer based on the provided parameters.
	 * @param WorldContext Object used to retrieve the World. Can be 'this', the World itself or anything linked to the world.
	 * @param TimerDelegate The delegate to execute when the timer expires.
	 * @param TimerParameters Parameters configuration (Rate, Loop, Delay).
	 * @param TimerScope Determines if the timer is standalone (Global) or linked to the WorldContext.
	 * @return A handle to the scheduled timer. Returns an invalid handle if Rate is <= 0.
	 */
	static FTimerHandle ScheduleTimer(UObject* WorldContext, TFunction<void(void)>&& TimerDelegate,
	                                  FTimerParameters TimerParameters, ETimerScope TimerScope = ETimerScope::Global);

	/**
	 * Schedules a new timer based on the provided parameters.
	 * @param Actor The actor to which the function is bound.
	 * @param TimerDelegate The function that the actor can call.
	 * @param TimerParameters Parameters configuration (Rate, Loop, Delay).
	 * @return A handle to the scheduled timer. Returns an invalid handle if Rate is <= 0.
	 */
	template <Private::Concepts::DerivedFromActor TActor>
	static FTimerHandle ScheduleTimer(TActor* Actor, FTimerDelegate::TMethodPtr<TActor> TimerDelegate,
	                                  FTimerParameters TimerParameters)
	{
		if (Internal_CheckRate(TimerParameters.Rate))
		{
			return {};
		}
		
		//TODO : Put chain verification for null pointer.
		
		ThisClass* TimerHolderSubsystem{ Get(Actor) };
		return TimerHolderSubsystem->Internal_BindTimerToActor(Actor, TimerDelegate, TimerParameters);
	}

	/**
	 * Schedules a new timer based on the provided parameters.
	 * @param Actor The actor to which the function is bound.
	 * @param TimerDelegate The function that the actor can call.
	 * @param TimerParameters Parameters configuration (Rate, Loop, Delay).
	 * @return A handle to the scheduled timer. Returns an invalid handle if Rate is <= 0.
	 */
	template <Private::Concepts::DerivedFromActor TActor>
	static FTimerHandle ScheduleTimer(TActor* Actor, FTimerDelegate::TConstMethodPtr<TActor> TimerDelegate,
	                                  FTimerParameters TimerParameters)
	{
		if (Internal_CheckRate(TimerParameters.Rate))
		{
			return {};
		}

		//TODO : Put chain verification for null pointer.
		
		ThisClass* TimerHolderSubsystem{ Get(Actor) };
		return TimerHolderSubsystem->Internal_BindTimerToActor(Actor, TimerDelegate, TimerParameters);
	}

	/**
	 * Cancels all timers currently bound to the specified WorldContext.
	 * Useful for cleaning up timers associated with an object before it is destroyed.
	 * Only affects timers scheduled with ETimerScope::ContextBound.
	 * @param WorldContext The object acting as the context/owner of the timers.
	 */
	static void CancelTimer(UObject* WorldContext);

	/**
	 * Cancels a specific active timer.
	 * @param WorldContext The object used to retrieve the subsystem. If TimerScope is ContextBound, this must be the same object used to schedule the timer.
	 * @param TimerHandle The handle of the timer to cancel. This handle will be invalidated.
	 * @param TimerScope Specifies if the timer is global or bound to the WorldContext.
	 */
	static void CancelTimer(const UObject* WorldContext, FTimerHandle& TimerHandle,
	                        ETimerScope TimerScope = ETimerScope::Global);

	/**
	 * Cancels all timers associated with the specified Actor.
	 * @param Actor The actor whose timers should be stopped.
	 * @param TimerScope Determines the scope of the cancellation:
	 * - ETimerScope::Global: Performs a full cleanup using the engine's native ClearAllTimersForObject (stops ALL timers on this actor).
	 * - ETimerScope::ContextBound: Only cancels timers specifically registered to this Actor within this subsystem's tracking map.
	 */
	template <Private::Concepts::DerivedFromActor TActor>
	static void CancelTimer(TActor* Actor, ETimerScope TimerScope = ETimerScope::Global)
	{
		//TODO : Put chain verification for null pointer.
		
		if (TimerScope == ETimerScope::ContextBound)
		{
			CancelTimer(Cast<UObject>(Actor));
			return;
		}

		ThisClass* TimerHolderSubsystem{ Get(Actor) };
		TimerHolderSubsystem->GetTimerManager().ClearAllTimersForObject(Actor);
	}

	virtual void Deinitialize() override;

private:
	template <typename TCallable>
	FTimerHandle Internal_LaunchAndRegister(UObject* WorldContext, FTimerParameters TimerParameters,
	                                        ETimerScope TimerScope,
	                                        TSharedRef<FTimerHandle> TimerHandle,
	                                        TCallable&& InDelegate)
	{
		//Lambda wrapper to automatically unregister the handle from our internal lists when the timer finishes.
		//Prevents "zombie" entries in the maps/sets.
		auto TimerCallback =
			[this, TimerScope, WorldContext, TimerHandle,
				Delegate = Forward<TCallable>(InDelegate)]() mutable
		{
			if constexpr (std::is_invocable_v<TCallable>)
			{
				Delegate();
			}
			else
			{
				Delegate.ExecuteIfBound();
			}

			//TODO : Put chain verification for null pointer.
			if (TimerScope == ETimerScope::Global)
			{
				UnboundActiveTimers.Remove(*TimerHandle);
				UE_LOG(LogTemp, Warning, TEXT("Removed Unbound timer [UnboundActiveTimers size : %d]"), UnboundActiveTimers.Num())
				return;
			}

			if (FActiveTimerRegistry* TimerHandleMap { OwnerBoundTimers.Find(WorldContext) })
			{
				TimerHandleMap->Remove(*TimerHandle);
				UE_LOG(LogTemp, Warning, TEXT("Unbound timer to %p [OwnerBoundTimers size : %d]"), WorldContext, UnboundActiveTimers.Num())
			}
		};

		GetTimerManager().SetTimer(*TimerHandle, MoveTemp(TimerCallback), TimerParameters.Rate,
		                           TimerParameters.bIsLooping, TimerParameters.FirstDelay);

		if (TimerScope == ETimerScope::ContextBound)
		{
			FActiveTimerRegistry& TimerHandleSet{ OwnerBoundTimers.FindOrAdd(WorldContext) };
			TimerHandleSet.Add(*TimerHandle);

			UE_LOG(LogTemp, Warning, TEXT("Bound timer to %p [OwnerBoundTimers size : %d]"), WorldContext, OwnerBoundTimers.Num())
		}
		else
		{
			UnboundActiveTimers.Add(*TimerHandle);
			UE_LOG(LogTemp, Warning, TEXT("Registered Unbound timer with the help of %p"), WorldContext)
		}

		return *TimerHandle;
	}

	template <Private::Concepts::DerivedFromActor TActor, typename TFunc>
		requires std::invocable<TFunc, TActor>
	FTimerHandle Internal_BindTimerToActor(TActor* Actor, TFunc Function, FTimerParameters TimerParameters)
	{
		//TODO : Put chain verification for null pointer.
		
		FTimerHandle Handle;
		GetTimerManager().SetTimer(Handle, Actor, Function, TimerParameters.Rate,
		                           TimerParameters.bIsLooping, TimerParameters.FirstDelay);
		return Handle;
	}

	static bool Internal_CheckRate(const float InRate);

	FTimerManager& GetTimerManager() const;
};
