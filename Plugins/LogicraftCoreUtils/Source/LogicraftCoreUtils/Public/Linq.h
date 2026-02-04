// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <concepts>
#include <algorithm> // Required for Algo::Sort

namespace Linq
{
	namespace Private
	{
		namespace Concept
		{
			// C++20 Concepts to constrain template types used in the library.
			template <typename Ty>
			concept DerivedFromObject = std::derived_from<Ty, UObject>;

			template <typename T>
			struct TIsTMap : std::false_type
			{
			};

			template <typename... Args>
			struct TIsTMap<TMap<Args...>> : std::true_type
			{
			};

			template <typename T>
			inline constexpr bool TIsTMap_v = TIsTMap<std::remove_cvref_t<T>>::value;

			template <typename C, typename = void>
			struct TContainerValueType
			{
				using Type = typename C::ElementType;
			};

			template <typename... Args>
			struct TContainerValueType<TMap<Args...>>
			{
				using Type = TPair<typename TMap<Args...>::KeyType, typename TMap<Args...>::ValueType>;
			};

			template <typename C>
			using TContainerValueType_t = typename TContainerValueType<std::remove_reference_t<C>>::Type;
		}

		// Traits helper to determine how to store a value.
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

		// Base interface for all LINQ iterators (CRTP).
		template <typename Derived, typename T>
		class ILinqIterator
		{
		public:
			bool Next()
			{
				return static_cast<Derived*>(this)->Next_Implementation();
			}

			void Reset()
			{
				static_cast<Derived*>(this)->Reset_Implementation();
			}

			const T& GetCurrent()
			{
				return static_cast<Derived*>(this)->GetCurrent_Implementation();
			}

			using ValueType = T;
		};

		// L-Value Source Iterator
		template <typename ContainerType, typename ValueType, typename IteratorType>
		class TLValueSourceIterator : public ILinqIterator<
				TLValueSourceIterator<ContainerType, ValueType, IteratorType>, ValueType>
		{
			using FCollectionType = const ContainerType*;
			FCollectionType Collection = nullptr;


			TOptional<IteratorType> Current;

			IteratorType EndIterator;

		public:
			explicit TLValueSourceIterator(FCollectionType InSource)
				: Collection(InSource), EndIterator(InSource->end())
			{
			}

			bool Next_Implementation()
			{
				if (!Current.IsSet())
				{
					Current.Emplace(Collection->begin());
				}
				else
				{
					++(*Current);
				}

				return *Current != EndIterator;
			}

			void Reset_Implementation()
			{
				Current.Reset();
			}

			const ValueType& GetCurrent_Implementation()
			{
				return **Current;
			}
		};

		// R-Value Source Iterator
		template <typename ContainerType, typename ValueType, typename IteratorType>
		class TRValueSourceIterator : public ILinqIterator<
				TRValueSourceIterator<ContainerType, ValueType, IteratorType>, ValueType>
		{
			using FCollectionType = ContainerType;
			FCollectionType Collection{};
			TOptional<IteratorType> Current;

		public:
			explicit TRValueSourceIterator(FCollectionType&& InSource)
				: Collection(MoveTemp(InSource))
			{
			}

			bool Next_Implementation()
			{
				if (!Current.IsSet())
				{
					Current.Emplace(Collection.begin());
				}
				else
				{
					++(*Current);
				}

				return *Current != Collection.end();
			}

			void Reset_Implementation()
			{
				Current.Reset();
			}

			const ValueType& GetCurrent_Implementation()
			{
				return **Current;
			}
		};

		// Where Iterator
		// FIXED: Template order in inheritance <Pred, InIterator, T> matches definition.
		// FIXED: Source accessed via dot (.) operator.
		template <typename Pred, typename InIterator, typename T = InIterator::ValueType>
		class TWhereIterator : public ILinqIterator<TWhereIterator<Pred, InIterator, T>, T>
		{
			InIterator Source;
			const T* CurrentValue = {};
			Pred Predicate;

		public:
			template <typename P = Pred>
				requires std::is_same_v<std::remove_cvref_t<P>, std::remove_cvref_t<Pred>>
			TWhereIterator(InIterator&& InSource, P&& InPredicate)
				: Source(MoveTemp(InSource)), Predicate(Forward<P>(InPredicate))
			{
			}

			bool Next_Implementation()
			{
				while (Source.Next())
				{
					CurrentValue = &Source.GetCurrent();
					if (Invoke(Predicate, *CurrentValue))
					{
						return true;
					}
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
				CurrentValue = {};
			}

			const T& GetCurrent_Implementation()
			{
				return *CurrentValue;
			}
		};

		// Select Iterator
		// FIXED: Template order in inheritance <Out, Sel, InIterator, In> matches definition.
		template <typename Out, typename Sel, typename InIterator, typename In = InIterator::ValueType>
		class TSelectIterator : public ILinqIterator<
				TSelectIterator<Out, Sel, InIterator, In>, std::remove_reference_t<Out>>
		{
			InIterator Source;
			TStorageTraits<Out> CurrentValue = {};
			Sel Selector;

		public:
			template <typename S = Sel>
				requires std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<Sel>>
			TSelectIterator(InIterator&& InSource, S&& InSelector)
				: Source(MoveTemp(InSource)), Selector(Forward<S>(InSelector))
			{
			}

			bool Next_Implementation()
			{
				while (Source.Next())
				{
					CurrentValue.Set(Invoke(Selector, Source.GetCurrent()));
					return true;
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
				CurrentValue.Set({});
			}

			const std::remove_reference_t<Out>& GetCurrent_Implementation()
			{
				return CurrentValue.Get();
			}
		};

		// Apply Iterator
		// FIXED: Template order in inheritance <Func, InIterator, T> matches definition.
		template <typename Func, typename InIterator, typename T = InIterator::ValueType>
		class TApplyIterator : public ILinqIterator<TApplyIterator<Func, InIterator, T>, std::remove_reference_t<T>>
		{
			InIterator Source;
			TStorageTraits<T> CurrentValue;
			Func Modifier;

		public:
			template <typename F = Func>
				requires std::is_same_v<std::remove_cvref_t<F>, std::remove_cvref_t<Func>>
			TApplyIterator(InIterator&& InSource, F&& InModifier)
				: Source(MoveTemp(InSource)), Modifier(Forward<F>(InModifier))
			{
			}

			bool Next_Implementation()
			{
				while (Source.Next())
				{
					CurrentValue.Set(Invoke(Modifier, Source.GetCurrent()));
					return true;
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
				CurrentValue.Set({});
			}

			const std::remove_reference_t<T>& GetCurrent_Implementation()
			{
				return CurrentValue.Get();
			}
		};

		// Execute Iterator
		// FIXED: Template order in inheritance <Func, InIterator, T> matches definition.
		template <typename Func, typename InIterator, typename T = InIterator::ValueType>
		class TExecuteIterator : public ILinqIterator<TExecuteIterator<Func, InIterator, T>, T>
		{
			InIterator Source;
			Func Modifier;

		public:
			template <typename F = Func>
				requires std::is_same_v<std::remove_cvref_t<F>, std::remove_cvref_t<Func>>
			TExecuteIterator(InIterator&& InSource, F&& InAction)
				: Source(MoveTemp(InSource)), Modifier(Forward<F>(InAction))
			{
			}

			bool Next_Implementation()
			{
				while (Source.Next())
				{
					Invoke(Modifier, Source.GetCurrent());
					return true;
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
			}

			const T& GetCurrent_Implementation()
			{
				return Source.GetCurrent();
			}
		};

		// Cast Iterator
		// FIXED: Template order in inheritance <Out, InIterator, In> matches definition.
		template <typename Out, typename InIterator, typename In = InIterator::ValueType>
			requires Concept::DerivedFromObject<std::remove_pointer_t<In>>
			&& Concept::DerivedFromObject<std::remove_pointer_t<Out>>
		class TCastIterator : public ILinqIterator<TCastIterator<Out, InIterator, In>, Out>
		{
			InIterator Source;
			Out CurrentValue = {};

		public:
			TCastIterator(InIterator&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			bool Next_Implementation()
			{
				while (Source.Next())
				{
					if (CurrentValue = Cast<std::remove_pointer_t<Out>>(Source.GetCurrent()); CurrentValue)
					{
						return true;
					}
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
				CurrentValue = {};
			}

			const Out& GetCurrent_Implementation()
			{
				return CurrentValue;
			}
		};

		// IsValid Iterator
		template <typename InIterator, typename T = InIterator::ValueType>
			requires Concept::DerivedFromObject<std::remove_pointer_t<T>>
		class TIsValidIterator : public ILinqIterator<TIsValidIterator<InIterator, T>, T>
		{
			InIterator Source;

		public:
			TIsValidIterator(InIterator&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			bool Next_Implementation()
			{
				while (Source.Next())
				{
					if (IsValid(Source.GetCurrent()))
					{
						return true;
					}
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
			}

			const T& GetCurrent_Implementation()
			{
				return Source.GetCurrent();
			}
		};

		// Take Iterator
		template <typename InIterator, typename T = InIterator::ValueType>
		class TTakeIterator : public ILinqIterator<TTakeIterator<InIterator, T>, T>
		{
			InIterator Source;
			int MaxElement = 0;
			int Counter = 0;

		public:
			TTakeIterator(InIterator&& InSource, const int InNumber)
				: Source(MoveTemp(InSource)), MaxElement(InNumber)
			{
			}

			bool Next_Implementation()
			{
				if (Counter >= MaxElement)
				{
					return false;
				}

				while (Source.Next())
				{
					if (Counter++ < MaxElement)
					{
						return true;
					}
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
				Counter = 0;
			}

			const T& GetCurrent_Implementation()
			{
				return Source.GetCurrent();
			}
		};

		// Skip Iterator
		template <typename InIterator, typename T = InIterator::ValueType>
		class TSkipIterator : public ILinqIterator<TSkipIterator<InIterator, T>, T>
		{
			InIterator Source;
			int MaxElement = 0;
			int Counter = 0;

		public:
			TSkipIterator(InIterator&& InSource, const int InNumber)
				: Source(MoveTemp(InSource)), MaxElement(InNumber)
			{
			}

			bool Next_Implementation()
			{
				while (Source.Next())
				{
					if (Counter++ < MaxElement)
					{
						continue;
					}
					return true;
				}
				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
				Counter = 0;
			}

			const T& GetCurrent_Implementation()
			{
				return Source.GetCurrent();
			}
		};

		// OrderBy Base
		template <typename Derived, typename InIterator, typename T = InIterator::ValueType>
		class TOrderByBaseIterator : public ILinqIterator<Derived, T>
		{
			InIterator Source;
			bool bHasReset = true;
			int CurrentIndex = -1;

			void BuildNewCollection()
			{
				if (!bHasReset)
				{
					return;
				}
				bHasReset = false;

				while (Source.Next())
				{
					NewCollection.Add(Source.GetCurrent());
				}
				static_cast<Derived*>(this)->SortMethod();
			}

		protected:
			TArray<T> NewCollection = {};

		public:
			TOrderByBaseIterator(InIterator&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			bool Next_Implementation()
			{
				BuildNewCollection();

				if (++CurrentIndex < NewCollection.Num())
				{
					return true;
				}

				return false;
			}

			void Reset_Implementation()
			{
				Source.Reset();
				NewCollection.Empty();
				bHasReset = true;
				CurrentIndex = -1;
			}

			const T& GetCurrent_Implementation()
			{
				return NewCollection[CurrentIndex];
			}
		};

		// Simple OrderBy
		// FIXED: Template order <Derived, InIterator, T>
		template <typename InIterator, typename T = InIterator::ValueType>
			requires std::totally_ordered<T> ||
			requires(T Result) { Result < std::declval<decltype(Result)>(); }
		class TSimpleOrderByIterator : public TOrderByBaseIterator<TSimpleOrderByIterator<InIterator, T>, InIterator, T>
		{
		protected:
			friend TOrderByBaseIterator<TSimpleOrderByIterator, InIterator, T>;

			void SortMethod()
			{
				Algo::Sort(this->NewCollection);
			}

		public:
			explicit TSimpleOrderByIterator(InIterator&& InSource)
				: TOrderByBaseIterator<TSimpleOrderByIterator, InIterator, T>(MoveTemp(InSource))
			{
			}
		};

		// Selector OrderBy
		// FIXED: Template order <Derived, InIterator, T>
		template <typename Sel, typename InIterator, typename T = InIterator::ValueType>
			requires std::totally_ordered<std::invoke_result_t<Sel, T>> ||
			requires(std::invoke_result_t<Sel, T> Result) { Result < std::declval<decltype(Result)>(); }
		class TSelectorOrderByIterator : public TOrderByBaseIterator<
				TSelectorOrderByIterator<Sel, InIterator, T>, InIterator, T>
		{
			Sel Selector;
			friend TOrderByBaseIterator<TSelectorOrderByIterator, InIterator, T>;

		protected:
			void SortMethod()
			{
				Algo::SortBy(this->NewCollection, Selector);
			}

		public:
			template <typename S = Sel>
				requires std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<Sel>>
			explicit TSelectorOrderByIterator(InIterator&& InSource, Sel&& InSelector)
				: TOrderByBaseIterator<TSelectorOrderByIterator, InIterator, T>(MoveTemp(InSource)),
				  Selector(Forward<S>(InSelector))
			{
			}
		};

		// Comparator OrderBy
		// FIXED: Template order <Derived, InIterator, T>
		template <typename Comp, typename InIterator, typename T = InIterator::ValueType>
			requires requires(T a, T b) { { a < b } -> std::convertible_to<bool>; } || std::totally_ordered<T>
		class TComparatorOrderByIterator : public TOrderByBaseIterator<
				TComparatorOrderByIterator<Comp, InIterator, T>, InIterator, T>
		{
			Comp Selector;
			friend TOrderByBaseIterator<TComparatorOrderByIterator, InIterator, T>;

		protected:
			void SortMethod()
			{
				Algo::Sort(this->NewCollection, Selector);
			}

		public:
			template <typename C = Comp>
				requires std::is_same_v<std::remove_cvref_t<C>, std::remove_cvref_t<Comp>>
			explicit TComparatorOrderByIterator(InIterator&& InSource, C&& InSelector)
				: TOrderByBaseIterator<TComparatorOrderByIterator, InIterator, T>(MoveTemp(InSource)),
				  Selector(Forward<C>(InSelector))
			{
			}
		};

		// The main wrapper class exposed to the user. 
		template <typename InIterator, typename T = InIterator::ValueType>
		class TLinqQuery
		{
			InIterator Iterator;

		public:
			explicit TLinqQuery(InIterator&& InSource) : Iterator(MoveTemp(InSource))
			{
			}

			// --- Intermediate Operations ---

			template <typename Pred>
			auto Where(Pred&& Predicate)
			{
				using NewIterType = TWhereIterator<Pred, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Pred>(Predicate)));
			}

			template <typename Sel, typename Out = std::invoke_result_t<Sel, T>>
			auto Select(Sel&& Selector)
			{
				using NewIterType = TSelectIterator<Out, Sel, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Sel>(Selector)));
			}

			template <typename Func>
			auto Apply(Func&& Modifier)
			{
				using NewIterType = TApplyIterator<Func, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			template <typename Func>
			auto Execute(Func&& Modifier)
			{
				using NewIterType = TExecuteIterator<Func, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			template <typename Comp>
				requires std::predicate<Comp, T, T>
			auto OrderBy(Comp&& Comparator)
			{
				using NewIterType = TComparatorOrderByIterator<Comp, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Comp>(Comparator)));
			}

			auto OrderBy()
			{
				using NewIterType = TSimpleOrderByIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator)));
			}

			template <typename Sel>
			auto OrderBy(Sel&& Selector)
			{
				using NewIterType = TSelectorOrderByIterator<Sel, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Sel>(Selector)));
			}

			template <typename NewType>
			auto Cast()
			{
				using NewIterType = TCastIterator<NewType*, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator)));
			}

			auto IsValid()
			{
				using NewIterType = TIsValidIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator)));
			}

			auto Skip(const int Number)
			{
				using NewIterType = TSkipIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Number));
			}

			auto Take(const int Number)
			{
				using NewIterType = TTakeIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Number));
			}

			// --- Terminal Operations ---

			template <typename Pred>
			TOptional<T> First(Pred&& Predicate)
			{
				Iterator.Reset();
				while (Iterator.Next())
				{
					if (Invoke(Predicate, Iterator.GetCurrent()))
					{
						return Iterator.GetCurrent();
					}
				}
				return NullOpt;
			}

			template <typename Pred, typename DefValue = T>
				requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<DefValue>>
			T FirstOrDefault(Pred&& Predicate, DefValue&& DefaultValue)
			{
				Iterator.Reset();
				while (Iterator.Next())
				{
					if (Invoke(Predicate, Iterator.GetCurrent()))
					{
						return Iterator.GetCurrent();
					}
				}
				return Forward<DefValue>(DefaultValue);
			}

			template <typename Pred>
			TOptional<T> Last(Pred&& Predicate)
			{
				Iterator.Reset();
				TOptional<T> LastFound;
				while (Iterator.Next())
				{
					if (Invoke(Predicate, Iterator.GetCurrent()))
					{
						LastFound = Iterator.GetCurrent();
					}
				}

				return LastFound;
			}

			template <typename Pred, typename DefValue = T>
				requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<DefValue>>
			T LastOrDefault(Pred&& Predicate, DefValue&& DefaultValue)
			{
				Iterator.Reset();
				TOptional<T> LastFound;
				while (Iterator.Next())
				{
					if (Invoke(Predicate, Iterator.GetCurrent()))
					{
						LastFound = Iterator.GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				return Forward<DefValue>(DefaultValue);
			}

			template <typename Pred>
			T Single(Pred&& Predicate)
			{
				Iterator.Reset();
				TOptional<T> LastFound;
				while (Iterator.Next())
				{
					if (Invoke(Predicate, Iterator.GetCurrent()))
					{
						if (LastFound)
						{
							ensureMsgf(false, TEXT("Two objects found when only one was expected!"));
							return {};
						}
						LastFound = Iterator.GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				ensureMsgf(false, TEXT("No objects found when only one was expected!"));
				return {};
			}

			template <typename Pred>
			int Count(Pred&& Predicate)
			{
				int Counter = 0;
				Iterator.Reset();
				while (Iterator.Next())
				{
					if (Invoke(Predicate, Iterator.GetCurrent()))
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
				Iterator.Reset();
				while (Iterator.Next())
				{
					Result += Invoke(Selector, Iterator.GetCurrent());
				}
				return Result;
			}

			T Sum()
			{
				static_assert(std::is_arithmetic_v<T>, "Current collection is not arithmetic");
				T Result{};
				Iterator.Reset();
				while (Iterator.Next())
				{
					Result += Iterator.GetCurrent();
				}
				return Result;
			}

			template <typename Comp>
				requires std::predicate<Comp, T, T>
			TOptional<T> Min(Comp&& Comparator)
			{
				TOptional<T> Result;
				Iterator.Reset();

				while (Iterator.Next())
				{
					const T& Current = Iterator.GetCurrent();

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
				Iterator.Reset();

				while (Iterator.Next())
				{
					const T& Current = Iterator.GetCurrent();

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
				Iterator.Reset();

				while (Iterator.Next())
				{
					const T& Current = Iterator.GetCurrent();

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
				Iterator.Reset();

				while (Iterator.Next())
				{
					const T& Current = Iterator.GetCurrent();

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

			template <typename Pred>
				requires std::predicate<Pred, T> && std::is_convertible_v<std::invoke_result_t<Pred, T>, bool>
			bool Any(Pred&& Predicate)
			{
				Iterator.Reset();
				while (Iterator.Next())
				{
					if (Invoke(Predicate, Iterator.GetCurrent()))
					{
						return true;
					}
				}
				return false;
			}

			template <typename Pred>
				requires std::predicate<Pred, T> && std::is_convertible_v<std::invoke_result_t<Pred, T>, bool>
			bool All(Pred&& Predicate)
			{
				Iterator.Reset();
				while (Iterator.Next())
				{
					if (!Invoke(Predicate, Iterator.GetCurrent()))
					{
						return false;
					}
				}
				return true;
			}

			template <typename Comp>
				requires std::predicate<Comp, T, T> && std::is_convertible_v<std::invoke_result_t<Comp, T, T>, bool>
			bool Contains(const T& ValueToFind, Comp&& Comparator)
			{
				Iterator.Reset();
				while (Iterator.Next())
				{
					if (Invoke(Comparator, ValueToFind, Iterator.GetCurrent()))
					{
						return true;
					}
				}
				return false;
			}

			bool Contains(const T& ValueToFind)
			{
				static_assert(std::equality_comparable<T>, "T must implement operator == for Contains()");
				Iterator.Reset();
				while (Iterator.Next())
				{
					if (ValueToFind == Iterator.GetCurrent())
					{
						return true;
					}
				}
				return false;
			}

			TArray<T> ToArray()
			{
				TArray<T> Result;
				Iterator.Reset();
				while (Iterator.Next())
				{
					Result.Add(Iterator.GetCurrent());
				}
				return Result;
			}


			TSet<T> ToSet()
			{
				TSet<T> Result;
				Iterator.Reset();
				while (Iterator.Next())
				{
					Result.Add(Iterator.GetCurrent());
				}
				return Result;
			}

			template <typename K = decltype(std::declval<T>().Key), typename V = decltype(std::declval<T>().Value)>
			TMap<K, V> ToMap()
			{
				TMap<K, V> Result;
				Iterator.Reset();
				while (Iterator.Next())
				{
					const T& Pair = Iterator.GetCurrent();
					Result.Add(Pair.Key, Pair.Value);
				}
				return Result;
			}

			template <typename KeySel, typename K = std::invoke_result_t<KeySel, T>>
			TMap<K, T> ToMap(KeySel&& KeySelector)
			{
				TMap<K, T> Result;
				Iterator.Reset();
				while (Iterator.Next())
				{
					const T& Current = Iterator.GetCurrent();
					K Key = Invoke(KeySelector, Current);
					Result.Add(Key, Current);
				}
				return Result;
			}

			template <typename KeySel, typename ValSel,
			          typename K = std::invoke_result_t<KeySel, T>,
			          typename V = std::invoke_result_t<ValSel, T>>
			TMap<K, V> ToMap(KeySel&& KeySelector, ValSel&& ValueSelector)
			{
				TMap<K, V> Result;
				Iterator.Reset();
				while (Iterator.Next())
				{
					const T& Current = Iterator.GetCurrent();
					K Key = Invoke(KeySelector, Current);
					V Value = Invoke(ValueSelector, Current);
					Result.Add(Key, Value);
				}
				return Result;
			}


			void End()
			{
				Iterator.Reset();
				while (Iterator.Next())
				{
				}
			}
		};
	}

	// --- Entry Points ---


	template <typename ContainerType>
	auto Start(ContainerType&& Source)
	{
		using CleanContainerType = std::remove_cvref_t<ContainerType>;
		using ValueType = Private::Concept::TContainerValueType_t<CleanContainerType>;
		if constexpr (std::is_lvalue_reference_v<ContainerType>)
		{
			using FIteratorType = Private::TLValueSourceIterator<
				CleanContainerType, ValueType, decltype(std::declval<const CleanContainerType&>().begin())>;
			return Private::TLinqQuery<FIteratorType>(FIteratorType(&Source));
		}
		else
		{
			using FIteratorType = Private::TRValueSourceIterator<
				CleanContainerType, ValueType, decltype(Source.begin())>;
			return Private::TLinqQuery<FIteratorType>(FIteratorType(MoveTemp(Source)));
		}
	}
}
