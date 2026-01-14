// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

namespace Linq
{
	namespace Private
	{
		namespace Concept
		{
			template <typename Collection, typename T>
			concept IsCollection = std::is_same_v<Collection, TArray<T>>;

			template <typename Ty>
			concept DerivedFromObject = std::derived_from<Ty, UObject>;

			template <TIsArray T>
			struct GetArrayValueType
			{
				using Type = std::remove_cvref_t<decltype(*std::declval<T>().begin())>;
			};
		}

		template <typename T>
		struct TStorageTraits
		{ 
			using StorageType = std::conditional_t<std::is_reference_v<T>, std::remove_reference_t<T>*, T>;
		private:
			StorageType Value;
		public:

			
			void Set(T&& InValue)
			{
				if constexpr (std::is_reference_v<T>)
				{
					Value = &InValue; 
				}
				else
				{
					Value = Forward<T>(InValue);
				}
			}

			const T& Get()
			{
				if constexpr (std::is_reference_v<T>)
				{
					return *Value; 
				}
				else
				{
					return Value;
				}
			}
		};
		

		template <typename T>
		class ILinqIterator
		{
		public:
			virtual ~ILinqIterator() = default;

			virtual bool Next() = 0;

			virtual void Reset() = 0;

			virtual const T& GetCurrent() = 0;
		};

		template <typename T>
		class TLValueSourceIterator : public ILinqIterator<T>
		{
			const TArray<T>* Collection = nullptr;
			int Current = -1;

		public:
			explicit TLValueSourceIterator(const TArray<T>* InSource)
				: Collection(InSource)
			{
			}

			virtual bool Next() override
			{
				return ++Current < Collection->Num();
			}

			virtual void Reset() override
			{
				Current = -1;
			}

			virtual const T& GetCurrent() override
			{
				return (*Collection)[Current];
			}
		};

		template <typename T>
		class TRValueSourceIterator : public ILinqIterator<T>
		{
			TArray<T> Collection = nullptr;
			int Current = -1;

		public:
			explicit TRValueSourceIterator(TArray<T>&& InSource)
				: Collection(MoveTemp(InSource))
			{
			}

			virtual bool Next() override
			{
				return ++Current < Collection.Num();
			}

			virtual void Reset() override
			{
				Current = -1;
			}

			virtual const T& GetCurrent() override
			{
				return Collection[Current];
			}
		};

		template <typename T, typename Pred>
		class TWhereIterator : public ILinqIterator<T>
		{
			TUniquePtr<ILinqIterator<T>> Source = nullptr;
			const T* CurrentValue = {};
			Pred Predicate;

		public:
			template <typename P = Pred>
			requires std::is_same_v<P, Pred>
			TWhereIterator(TUniquePtr<ILinqIterator<T>>&& InSource, P&& InPredicate)
				: Source(MoveTemp(InSource)), Predicate(Forward<Pred>(InPredicate))
			{
			}

			virtual bool Next() override
			{
				while (Source->Next())
				{
					CurrentValue = &Source->GetCurrent();
					if (Invoke(Predicate, *CurrentValue))
					{
						return true;
					}
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
				CurrentValue = {};
			}

			virtual const T& GetCurrent() override
			{
				return *CurrentValue;
			}
		};

		template <typename In, typename Out, typename Sel>
		class TSelectIterator : public ILinqIterator<std::remove_reference_t<Out>>
		{
			TUniquePtr<ILinqIterator<In>> Source = nullptr; 
			TStorageTraits<Out> CurrentValue = {};
			Sel Selector;

		public:
			template <typename S = Sel>
			requires std::is_same_v<S, Sel>
			TSelectIterator(TUniquePtr<ILinqIterator<In>>&& InSource, S&& InSelector)
				: Source(MoveTemp(InSource)), Selector(Forward<Sel>(InSelector))
			{
			}

			virtual bool Next() override
			{
				while (Source->Next())
				{
					CurrentValue.Set(Invoke(Selector, Source->GetCurrent()));
					return true;
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
				CurrentValue.Set({});
			}

			virtual const std::remove_reference_t<Out>& GetCurrent() override
			{
				return CurrentValue.Get();
			}
		};

		template <typename T, typename Func>
		class TApplyIterator : public ILinqIterator<std::remove_reference_t<T>>
		{
			TUniquePtr<ILinqIterator<T>> Source = nullptr;
			TStorageTraits<T> CurrentValue;
			Func Modifier;

		public:
			template <typename F = Func>
			requires std::is_same_v<F, Func>
			TApplyIterator(TUniquePtr<ILinqIterator<T>>&& InSource, F&& InPredicate)
				: Source(MoveTemp(InSource)), Modifier(Forward<Func>(InPredicate))
			{
			}

			virtual bool Next() override
			{
				while (Source->Next())
				{
					CurrentValue.Set(Invoke(Modifier, Source->GetCurrent()));
					return true;
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
				CurrentValue.Set({});
			}

			virtual const std::remove_reference_t<T>& GetCurrent() override
			{
				return CurrentValue.Get();
			}
		};

		template <typename T, typename Func>
		class TExecuteIterator : public ILinqIterator<T>
		{
			TUniquePtr<ILinqIterator<T>> Source = nullptr;
			Func Modifier;

		public:
			template <typename F = Func>
			requires std::is_same_v<F, Func>
			TExecuteIterator(TUniquePtr<ILinqIterator<T>>&& InSource, F&& InPredicate)
				: Source(MoveTemp(InSource)), Modifier(Forward<Func>(InPredicate))
			{
			}

			virtual bool Next() override
			{
				while (Source->Next())
				{
					Invoke(Modifier, Source->GetCurrent());
					return true;
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
			}

			virtual const T& GetCurrent() override
			{
				return Source->GetCurrent();
			}
		};

		template <typename In, typename Out>
		requires Concept::DerivedFromObject<std::remove_pointer_t<In>>
		&& Concept::DerivedFromObject<std::remove_pointer_t<Out>>
		class TCastIterator : public ILinqIterator<Out>
		{
			TUniquePtr<ILinqIterator<In>> Source = nullptr;
			Out CurrentValue = {};

		public:
			TCastIterator(TUniquePtr<ILinqIterator<In>>&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			virtual bool Next() override
			{
				while (Source->Next())
				{
					if (CurrentValue = Cast<std::remove_pointer_t<Out>>(Source->GetCurrent()); CurrentValue)
					{
						return true;
					}
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
				CurrentValue = {};
			}

			virtual const Out& GetCurrent() override
			{
				return CurrentValue;
			}
		};

		template <typename T>
		requires Concept::DerivedFromObject<std::remove_pointer_t<T>>
		class TIsValidIterator : public ILinqIterator<T>
		{
			TUniquePtr<ILinqIterator<T>> Source = nullptr;

		public:
			TIsValidIterator(TUniquePtr<ILinqIterator<T>>&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			virtual bool Next() override
			{
				while (Source->Next())
				{
					if (IsValid(Source->GetCurrent()))
					{
						return true;
					}
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
			}

			virtual const T& GetCurrent() override
			{
				return Source->GetCurrent();
			}
		};

		template<typename T>
		class TTakeIterator : public ILinqIterator<T>
		{
			TUniquePtr<ILinqIterator<T>> Source = nullptr;
			int MaxElement = 0;
			int Counter = 0;
		public:
			TTakeIterator(TUniquePtr<ILinqIterator<T>>&& InSource, const int InNumber)
				: Source(MoveTemp(InSource)), MaxElement(InNumber)
			{
			}

			virtual bool Next() override
			{				
				if (Counter >= MaxElement)
				{
					return false;
				}
				
				while (Source->Next())
				{
					if (Counter++ < MaxElement)
					{
						return true;
					}
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
				Counter = 0;
			}

			virtual const T& GetCurrent() override
			{
				return Source->GetCurrent();
			}
		};

		template<typename T>
		class TSkipIterator : public ILinqIterator<T>
		{
			TUniquePtr<ILinqIterator<T>> Source = nullptr;
			int MaxElement = 0;
			int Counter = 0;
		public:
			TSkipIterator(TUniquePtr<ILinqIterator<T>>&& InSource, const int InNumber)
				: Source(MoveTemp(InSource)), MaxElement(InNumber)
			{
			}

			virtual bool Next() override
			{				
				while (Source->Next())
				{
					if (Counter++ < MaxElement)
					{
						continue;
					}
					return true;
				}
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
				Counter = 0;
			}

			virtual const T& GetCurrent() override
			{
				return Source->GetCurrent();
			}
		};
		


		template <typename T>
		class TLinqQuery
		{
			TUniquePtr<ILinqIterator<T>> Iterator;

		public:
			explicit TLinqQuery(TUniquePtr<ILinqIterator<T>>&& InSource) : Iterator(MoveTemp(InSource))
			{
			}

			template <typename Pred>
			TLinqQuery<T> Where(Pred&& Predicate)
			{
				return TLinqQuery<T>(MakeUnique<TWhereIterator<T, Pred>>(
					MoveTemp(Iterator), Forward<Pred>(Predicate)));
			}

			template <typename Sel, typename Out = std::invoke_result_t<Sel, T>>
			TLinqQuery<Out> Select(Sel&& Selector)
			{
				return TLinqQuery<Out>(MakeUnique<TSelectIterator<T, Out, Sel>>(
					MoveTemp(Iterator), Forward<Sel>(Selector)));
			}

			template <typename Func>
			TLinqQuery<T> Apply(Func&& Modifier)
			{
				return TLinqQuery<T>(MakeUnique<TApplyIterator<T, Func>>(
					MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			template <typename Func>
			TLinqQuery<T> Execute(Func&& Modifier)
			{
				return TLinqQuery<T>(MakeUnique<TExecuteIterator<T, Func>>(
					MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			template <typename NewType>
			TLinqQuery<NewType*> Cast()
			{
				return TLinqQuery<NewType*>(MakeUnique<TCastIterator<T, NewType*>>(
					MoveTemp(Iterator)));
			}

			template <typename NewType = T>
			TLinqQuery IsValid()
			{
				return TLinqQuery<NewType>(MakeUnique<TIsValidIterator<T>>(
					MoveTemp(Iterator)));
			}

			TLinqQuery Skip(const int Number)
			{
				return TLinqQuery(MakeUnique<TSkipIterator<T>>(
					MoveTemp(Iterator), Number));
			}
 
			TLinqQuery Take(const int Number)
			{
				return TLinqQuery(MakeUnique<TTakeIterator<T>>(
					MoveTemp(Iterator), Number));
			}

			template <typename Pred>
			T First(Pred&& Predicate)
			{
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Forward<Pred>(Predicate), Iterator->GetCurrent()))
					{
						return Iterator->GetCurrent();
					}
				}
				checkf(false, TEXT("Unable to find object that satisfies predicate expression!"));
				return {};
			}

			template <typename Pred, typename DefValue = T>
			requires std::is_same_v<T, DefValue>
			T FirstOrDefault(Pred&& Predicate, DefValue&& DefaultValue)
			{
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Forward<Pred>(Predicate), Iterator->GetCurrent()))
					{
						return Iterator->GetCurrent();
					}
				}
				return Forward<T>(DefaultValue);
			}

			template <typename Pred>
			T Last(Pred&& Predicate)
			{
				Iterator->Reset();
				TOptional<T> LastFound;
				while (Iterator->Next())
				{
					if (Invoke(Forward<Pred>(Predicate), Iterator->GetCurrent()))
					{
						LastFound = Iterator->GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				checkf(false, TEXT("Unable to find object that satisfies predicate expression!"));
				return {};
			}

			template <typename Pred, typename DefValue = T>
			requires std::is_same_v<T, DefValue>
			T LastOrDefault(Pred&& Predicate, DefValue&& DefaultValue)
			{
				Iterator->Reset();
				TOptional<T> LastFound;
				while (Iterator->Next())
				{
					if (Invoke(Forward<Pred>(Predicate), Iterator->GetCurrent()))
					{
						LastFound = Iterator->GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				return Forward<T>(DefaultValue);
			}

			template <typename Pred>
			T Single(Pred&& Predicate)
			{
				Iterator->Reset();
				TOptional<T> LastFound;
				while (Iterator->Next())
				{
					if (Invoke(Forward<Pred>(Predicate), Iterator->GetCurrent()))
					{
						if (LastFound)
						{
							checkf(false, TEXT("Two objects found when only one was expected!"));
							return {};
						}
						LastFound = Iterator->GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				checkf(false, TEXT("No objects found when only one was expected!"));
				return {};
			}

			template <typename Pred>
			int Count(Pred&& Predicate)
			{
				int Counter = 0;
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Forward<Pred>(Predicate), Iterator->GetCurrent()))
					{
						++Counter;
					}
				}

				return Counter;
			}

			template <typename Sel, typename ReturnType = std::invoke_result_t<Sel, T>>
			requires std::is_arithmetic_v<ReturnType>
			ReturnType Sum(Sel&& Selector)
			{
				ReturnType Result{};
				Iterator->Reset();
				while (Iterator->Next())
				{
					Result += Invoke(Forward<Sel>(Selector), Iterator->GetCurrent());
				}
				return Result;
			}

			T Sum()
			{
				static_assert(std::is_arithmetic_v<T>, "Current collection is not arithmetic");
				T Result{};
				Iterator->Reset();
				while (Iterator->Next())
				{
					Result += Iterator->GetCurrent();
				}
				return Result;
			}

			template <typename Comp>
			requires std::predicate<Comp, T, T>
			TOptional<T> Min(Comp&& Comparator)
			{
				TOptional<T> Result;  
				Iterator->Reset();

				while (Iterator->Next())
				{
					const T& Current = Iterator->GetCurrent(); 

					if (!Result)
					{
						Result.Emplace(Current); 
					} 
					else if (Invoke(Comparator, Current, *Result))
					{
						Result.Emplace(Current); 
					}
				}
				return Result;
			}
 
			TOptional<T> Min()
			{
				static_assert(std::totally_ordered<T>, "T must implement operator < for Min()");

				TOptional<T> Result; 
				Iterator->Reset();

				while (Iterator->Next())
				{
					const T& Current = Iterator->GetCurrent();

					if (!Result)
					{
						Result.Emplace(Current); 
					}
					else if (Current < *Result)
					{
						Result.Emplace(Current); 
					}
				}
				return Result;
			}

			template <typename Comp>
			requires std::predicate<Comp, T, T>
			TOptional<T> Max(Comp&& Comparator)
			{
				TOptional<T> Result; 
				Iterator->Reset();

				while (Iterator->Next())
				{
					const T& Current = Iterator->GetCurrent();

					if (!Result)
					{
						Result.Emplace(Current); 
					} 
					else if (Invoke(Comparator, *Result, Current))
					{
						Result.Emplace(Current); 
					}
				}
				return Result;
			}
 
			TOptional<T> Max()
			{
				static_assert(std::totally_ordered<T>, "T must implement operator > for Max()");

				TOptional<T> Result; 
				Iterator->Reset();

				while (Iterator->Next())
				{
					const T& Current = Iterator->GetCurrent();

					if (!Result)
					{
						Result.Emplace(Current); 
					} 
					else if (Current > *Result)
					{
						Result.Emplace(Current); 
					}
				}
				return Result;
			}

			template<typename Pred>
			requires std::predicate<Pred, T> && std::is_convertible_v<std::invoke_result_t<Pred,T>, bool>
			bool Any(Pred&& Predicate)
			{
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Predicate, Iterator->GetCurrent()))
					{
						return true;
					}
				}
				return false;
			}

			template<typename Pred>
			requires std::predicate<Pred, T> && std::is_convertible_v<std::invoke_result_t<Pred,T>, bool>
			bool All(Pred&& Predicate)
			{
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (!Invoke(Predicate, Iterator->GetCurrent()))
					{
						return false;
					}
				}
				return true;
			}

			template<typename Comp>
			requires std::predicate<Comp, T, T> && std::is_convertible_v<std::invoke_result_t<Comp,T,T>, bool>
			bool Contains(const T& ValueToFind, Comp&& Comparator)
			{
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Comparator, ValueToFind, Iterator->GetCurrent()))
					{
						return true;
					}
				}
				return false;
			}
 
			bool Contains(const T& ValueToFind)
			{
				static_assert(std::equality_comparable<T>, "T must implement operator == for Contains()");
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (ValueToFind == Iterator->GetCurrent())
					{
						return true;
					}
				}
				return false;
			}

			
			
			TArray<T> ToArray()
			{
				TArray<T> Result;
				Iterator->Reset();
				while (Iterator->Next())
				{
					Result.Add(Iterator->GetCurrent());
				}
				return Result;
			}

			void End()
			{
				Iterator->Reset();
				while (Iterator->Next())
				{}
			}
			
		};
	}

	template <typename T>
	Private::TLinqQuery<T> Start(TArray<T>& Source)
	{
		return Private::TLinqQuery<T>(MakeUnique<Private::TLValueSourceIterator<T>>(&Source));
	}

	template <typename T>
	Private::TLinqQuery<T> Start(TArray<T>&& Source)
	{
		return Private::TLinqQuery<T>(MakeUnique<Private::TRValueSourceIterator<T>>(MoveTemp(Source)));
	}
}
