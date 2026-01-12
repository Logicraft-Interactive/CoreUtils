// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerHolderSubsystem.generated.h"

namespace Details
{
	struct FTimerHandleHash : DefaultKeyFuncs<FTimerHandle>
	{
		static FORCEINLINE uint32 GetKeyHash(const FTimerHandle& Key)
		{
			UE_LOG(LogTemp, Warning, TEXT("Custom Hash"))
			
			const uint64 HandleId{ *reinterpret_cast<const uint64*>(&Key) };
			constexpr unsigned GoldenRatio{ 0x9e3779b9 }; 
			return static_cast<uint32>((HandleId * GoldenRatio) >> 32);
		}
	};

	template<typename Ty>
	concept IsUObject = std::derived_from<Ty, UObject>;
} // Details

/**
 * 
 */
UCLASS()
class LOGICRAFTCOREUTILS_API UTimerHolderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	using TimerHandleSet = TSet<FTimerHandle, Details::FTimerHandleHash>;
	using ObjectTimerHandleMap = TMap<TWeakObjectPtr<UObject>, TimerHandleSet>;
	
	TimerHandleSet TimerHandles;
	ObjectTimerHandleMap ObjectTimerHandle;
public:
	static ThisClass* Get(const UObject* WorldContext);

	//Retenir les timers affilié a un objets si l'objet est detruit tout les timers doivent s'arreter.
	//Quand le timer est terminer enlever le timer handle.
	
	static FTimerHandle StartTimer(const UObject* WorldContext, const FTimerDelegate& TimerDelegate, const float Rate, const bool bIsLooping, const float FirstDelay = -1.f);
	static FTimerHandle StartTimer(const UObject* WorldContext, const FTimerDynamicDelegate& TimerDelegate, const float Rate, const bool bIsLooping, const float FirstDelay = -1.f);
	static FTimerHandle StartTimer(const UObject* WorldContext, TFunction<void(void)>&& TimerDelegate, const float Rate, const bool bIsLooping, const float FirstDelay = -1.f);

	template<Details::IsUObject UObject>
	static FTimerHandle StartTimer(UObject* Object, FTimerDelegate::TMethodPtr<UObject> TimerDelegate, const float Rate, const bool bIsLooping, const float FirstDelay = -1.f)
	{
		ThisClass* TimerHolderSubsystem{ Get(Object) };
		
		FTimerHandle Handle;
		TimerHolderSubsystem->GetTimerManager().SetTimer(Handle, Object, TimerDelegate, Rate, bIsLooping, FirstDelay);
		TimerHandleSet& TimerHandleSet{ TimerHolderSubsystem->ObjectTimerHandle.FindOrAdd(Object) };
		TimerHandleSet.Add(Handle);
		return Handle;
	}

	template<Details::IsUObject UObject>
	static FTimerHandle StartTimer(UObject* Object, FTimerDelegate::TConstMethodPtr<UObject> TimerDelegate, const float Rate, const bool bIsLooping, const float FirstDelay = -1.f)
	{
		ThisClass* TimerHolderSubsystem{ Get(Object) };
		
		FTimerHandle Handle;
		TimerHolderSubsystem->GetTimerManager().SetTimer(Handle, Object, TimerDelegate, Rate, bIsLooping, FirstDelay);
		TimerHandleSet& TimerHandleSet{ TimerHolderSubsystem->ObjectTimerHandle.FindOrAdd(Object) };
		TimerHandleSet.Add(Handle);
		return Handle;
	}

	static void EndTimer(const UObject* WorldContext, FTimerHandle& TimerHandle);

	template<Details::IsUObject UObject>
	static void EndTimer(UObject* Object)
	{
		ThisClass* TimerHolderSubsystem{ Get(Object) };
		
		if (TimerHolderSubsystem->ObjectTimerHandle.Remove(Object))
		{
			TimerHolderSubsystem->GetTimerManager().ClearAllTimersForObject(Object);
		}
	}

	virtual void Deinitialize() override;
private:
	FTimerManager& GetTimerManager() const;
};
