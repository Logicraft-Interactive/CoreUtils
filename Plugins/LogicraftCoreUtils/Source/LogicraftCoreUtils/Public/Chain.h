// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


/**
 * 
 */

namespace Chain
{
	namespace Private
	{		
		template<typename T>
		using CleanType = std::remove_pointer_t<std::remove_cvref_t<T>>;
		
		template<typename T>
		concept IsUObject = std::is_base_of_v<UObject, CleanType<T>>;

		void LOGICRAFTCOREUTILS_API ChainLog(const FString& Msg);
		
		template<IsUObject T>
		class TChain
		{
		private:
			T* Object = nullptr;
			bool bThrowWarning = false;
	
			bool CanChain() const
			{
				return IsValid(Object);
			}
		public:
			TChain() {};
			TChain(const TChain&) = default;
			TChain(TChain&&) = default;
			TChain& operator=(const TChain&) = default;
			TChain& operator=(TChain&&) = default; 
			TChain(T& InObject, bool bInThrowWarning) : Object(&InObject), bThrowWarning(bInThrowWarning) {}
			TChain(T* InObject, bool bInThrowWarning) : Object(InObject), bThrowWarning(bInThrowWarning) {}
					
			T* Get()
			{
				return Object;
			}

			T* Get() const
			{
				return Object;
			}

			T* operator->()
			{
				return Get();
			}

			T* operator->() const
			{
				return Get();
			}
			
			T& operator*() const
			{
				return *Object;
			}
			
			T& operator*()
			{
				return *Object;
			}

			operator T*() const
			{
				return Get();				
			}

			operator T*()
			{
				return Get();				
			}
			
			template<typename Func>
			TChain Execute(const Func& Function, const std::source_location& SourceLocation = std::source_location::current())
			{
				if (!CanChain())
				{
					if (bThrowWarning)
					{
						ChainLog(FString::Printf(TEXT("Object class %s from file %hs, line %d wasn't valid !"),
							*T::StaticClass()->GetName(),SourceLocation.file_name(), SourceLocation.line()));	
					}					
					return TChain();
				}
		
				Function(Object);
				return *this;
			}

			TChain Execute(void (T::*Function)(void), const std::source_location& SourceLocation = std::source_location::current())
			{
				if (!CanChain())
				{
					if (bThrowWarning)
					{
						ChainLog(FString::Printf(TEXT("Object class %s from file  %hs, line %d wasn't valid !"),
							*T::StaticClass()->GetName(),SourceLocation.file_name(), SourceLocation.line()));	
					}	
					return TChain();
				}
		
				(Object.*Function)();
				return *this;
			}

			template<typename Func>
			auto Transform(const Func& Function, const std::source_location& SourceLocation = std::source_location::current()) -> TChain<std::remove_pointer_t<decltype(Function(Object))>>
			{
				using NewType = std::remove_pointer_t<decltype(Function(Object))>;
				if (!CanChain())
				{
					if (bThrowWarning)
					{
						ChainLog(FString::Printf(TEXT("Object class %s from file %hs, line %d wasn't valid !"),
							*T::StaticClass()->GetName(),SourceLocation.file_name(), SourceLocation.line()));	
					}	
					return TChain<NewType>();
				}
		
				auto NewObject = Function(Object);
				return TChain<NewType>(NewObject, bThrowWarning);
			}
		};
	}
	
	


	template<Private::IsUObject T> 
	Private::TChain<T> StartChain(T* Object, bool bThrowWarning = true)
	{
		return Private::TChain<T>(Object, bThrowWarning);
	}

	template<Private::IsUObject T> 
	Private::TChain<T> StartChain(T& Object, bool bThrowWarning = true)
	{
		return Private::TChain<T>(Object, bThrowWarning);
	}

	template<Private::IsUObject T, std::invocable<Private::CleanType<T>*> Func>
	void Execute(T& Object, const Func& Function, bool bThrowWarning = true)
	{
		StartChain(Object).Execute(Function);
	}

	template<Private::IsUObject T, std::invocable<Private::CleanType<T>*> Func>
	auto Transform(T& Object, const Func& Function, bool bThrowWarning = true) -> Private::TChain<std::remove_pointer_t<decltype(Function(Object))>>
	{
		return StartChain(Object).Transform(Function);
	}
}
