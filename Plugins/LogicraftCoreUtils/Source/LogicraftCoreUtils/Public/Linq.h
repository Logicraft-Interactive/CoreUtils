// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <concepts>
#include <algorithm> // Required for Algo::Sort
#include "Meta/LCUTypeTraits.h"
#include "Meta/LCUConcepts.h"

/**
 * @namespace Linq
 * @brief Provides a set of LINQ-like extension methods for Unreal Engine containers and standard arrays.
 * Allows for fluent querying, filtering, sorting, and transformation of data.
 */
namespace Linq
{
	namespace Private
	{
		/**
		 * @struct TStorageTraits
		 * @brief Helper struct to determine how to store intermediate values in iterators.
		 * Handles the distinction between storing raw values vs pointers to references.
		 */
		template <typename T>
		struct TStorageTraits
		{
			using StorageType = std::conditional_t<std::is_reference_v<T>, std::remove_reference_t<T>*, T>;

		private:
			StorageType Value;

		public:
			/**
			 * @brief Sets the internal value, handling reference semantics.
			 */
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

			/**
			 * @brief Retrieves the value or dereferences the stored pointer.
			 */
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

		/**
		 * @class ILinqIterator
		 * @brief Base CRTP interface for all LINQ iterators.
		 * Defines the contract for Next(), Reset(), and GetCurrent().
		 */
		template <typename Derived, typename T>
		class ILinqIterator
		{
		public:
			/**
			 * @brief Advances the iterator to the next element.
			 * @return True if an element exists, False if the end is reached.
			 */
			bool Next()
			{
				return static_cast<Derived*>(this)->Next_Implementation();
			}

			/**
			 * @brief Resets the iterator to the beginning.
			 */
			void Reset()
			{
				static_cast<Derived*>(this)->Reset_Implementation();
			}

			/**
			 * @brief Gets the current element pointed to by the iterator.
			 */
			const T& GetCurrent()
			{
				return static_cast<Derived*>(this)->GetCurrent_Implementation();
			}

			using ValueType = T;
		};

		/**
		 * @class TLValueSourceIterator
		 * @brief Iterator for L-Value collections (references).
		 * Borrows a pointer to the source collection.
		 */
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

		/**
		 * @class TRValueSourceIterator
		 * @brief Iterator for R-Value collections (temporaries).
		 * Takes ownership of the collection via MoveTemp to ensure memory safety.
		 */
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

		/**
		 * @class TWhereIterator
		 * @brief Filters elements based on a predicate.
		 */
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

		/**
		 * @class TSelectIterator
		 * @brief Projects/Transforms elements into a new form (Map operation).
		 */
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

		/**
		 * @class TApplyIterator
		 * @brief Applies a modifier function to elements and returns the modified element.
		 * Useful for side-effects that change the state of the object.
		 */
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

		/**
		 * @class TExecuteIterator
		 * @brief Executes a function on elements without modifying the return flow.
		 * Pure side-effect iterator (like a 'ForEach' that continues the chain).
		 */
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

		/**
		 * @class TCastIterator
		 * @brief Attempts to Cast UObject pointers to a specific type.
		 * Only returns elements where the cast succeeds (effectively filters nulls).
		 */
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

		/**
		 * @class TIsValidIterator
		 * @brief Filters UObject pointers, returning only those that satisfy IsValid().
		 */
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

		/**
		 * @class TTakeIterator
		 * @brief Returns a specified number of contiguous elements from the start of the sequence.
		 */
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

		/**
		 * @class TSkipIterator
		 * @brief Bypasses a specified number of elements and then returns the remaining elements.
		 */
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

		/**
		 * @class TOrderByBaseIterator
		 * @brief Base class for sorting iterators.
		 * Unlike filtering iterators, this buffers the entire collection to sort it before iterating.
		 */
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

		/**
		 * @class TSimpleOrderByIterator
		 * @brief Sorts elements using their default operator<.
		 */
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

		/**
		 * @class TSelectorOrderByIterator
		 * @brief Sorts elements based on a key projected by a selector function.
		 */
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

		/**
		 * @class TComparatorOrderByIterator
		 * @brief Sorts elements using a custom comparator function (binary predicate).
		 */
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

		/**
		 * @class TLinqQuery
		 * @brief The main wrapper class exposed to the user via fluent API.
		 * Wraps an iterator and provides methods to chain subsequent operations.
		 */
		template <typename InIterator, typename T = InIterator::ValueType>
		class TLinqQuery
		{
			InIterator Iterator;

		public:
			explicit TLinqQuery(InIterator&& InSource) : Iterator(MoveTemp(InSource))
			{
			}

			// --- Intermediate Operations (Lazy Evaluation) ---

			/**
			 * @brief Filters the sequence based on a predicate.
			 */
			template <typename Pred>
			auto Where(Pred&& Predicate)
			{
				using NewIterType = TWhereIterator<Pred, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Pred>(Predicate)));
			}

			/**
			 * @brief Projects each element into a new form.
			 */
			template <typename Sel, typename Out = std::invoke_result_t<Sel, T>>
			auto Select(Sel&& Selector)
			{
				using NewIterType = TSelectIterator<Out, Sel, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Sel>(Selector)));
			}

			/**
			 * @brief Applies a modification to each element.
			 */
			template <typename Func>
			auto Apply(Func&& Modifier)
			{
				using NewIterType = TApplyIterator<Func, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			/**
			 * @brief Executes a side-effect function for each element without modifying the stream.
			 */
			template <typename Func>
			auto Execute(Func&& Modifier)
			{
				using NewIterType = TExecuteIterator<Func, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			/**
			 * @brief Sorts the sequence using a custom comparator.
			 */
			template <typename Comp>
				requires std::predicate<Comp, T, T>
			auto OrderBy(Comp&& Comparator)
			{
				using NewIterType = TComparatorOrderByIterator<Comp, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Comp>(Comparator)));
			}

			/**
			 * @brief Sorts the sequence using the default operator<.
			 */
			auto OrderBy()
			{
				using NewIterType = TSimpleOrderByIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator)));
			}

			/**
			 * @brief Sorts the sequence by a selected key.
			 */
			template <typename Sel>
			auto OrderBy(Sel&& Selector)
			{
				using NewIterType = TSelectorOrderByIterator<Sel, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Forward<Sel>(Selector)));
			}

			/**
			 * @brief Casts elements to a derived type. Filters out failed casts.
			 */
			template <typename NewType>
			auto Cast()
			{
				using NewIterType = TCastIterator<NewType*, InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator)));
			}

			/**
			 * @brief Filters out invalid UObjects.
			 */
			auto IsValid()
			{
				using NewIterType = TIsValidIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator)));
			}

			/**
			 * @brief Skips the first N elements.
			 */
			auto Skip(const int Number)
			{
				using NewIterType = TSkipIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Number));
			}

			/**
			 * @brief Takes only the first N elements.
			 */
			auto Take(const int Number)
			{
				using NewIterType = TTakeIterator<InIterator>;
				return TLinqQuery<NewIterType>(NewIterType(MoveTemp(Iterator), Number));
			}

			// --- Terminal Operations (Immediate Execution) ---

			/**
			 * @brief Returns the first element satisfying the predicate, or NullOpt if none found.
			 */
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

			/**
			 * @brief Returns the first element satisfying the predicate, or a default value.
			 */
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

			/**
			 * @brief Returns the last element satisfying the predicate, or NullOpt.
			 */
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

			/**
			 * @brief Returns the last element satisfying the predicate, or a default value.
			 */
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

			/**
			 * @brief Returns the single element satisfying the predicate. Asserts if 0 or >1 found.
			 */
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

			/**
			 * @brief Counts elements satisfying the predicate.
			 */
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

			/**
			 * @brief Sums the values returned by a selector function.
			 */
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

			/**
			 * @brief Sums the elements (must be arithmetic types).
			 */
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

			/**
			 * @brief Finds the minimum element based on a comparator.
			 */
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

			/**
			 * @brief Finds the minimum element using operator<.
			 */
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

			/**
			 * @brief Finds the maximum element based on a comparator.
			 */
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

			/**
			 * @brief Finds the maximum element using operator>.
			 */
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

			/**
			 * @brief Checks if any element satisfies the predicate.
			 */
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

			/**
			 * @brief Checks if all elements satisfy the predicate.
			 */
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

			/**
			 * @brief Checks if the sequence contains a specific value using a custom comparator.
			 */
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

			/**
			 * @brief Checks if the sequence contains a specific value using operator==.
			 */
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

			/**
			 * @brief Materializes the sequence into a TArray.
			 */
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

			/**
			 * @brief Materializes the sequence into a TSet.
			 */
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

			/**
			 * @brief Materializes the sequence into a TMap (assuming elements are Pairs).
			 */
			template <typename K = decltype(std::declval<T>().Key), typename V = decltype(std::declval<T>().Value)>
				requires TypeTrait::bIsInstantiation_V<TTuple, T>
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

			/**
			 * @brief Materializes the sequence into a TMap using a Key Selector.
			 */
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

			/**
			 * @brief Materializes the sequence into a TMap using Key and Value Selectors.
			 */
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

			/**
			 * @brief Consumes the iterator until the end (useful for executing side-effects).
			 */
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

	/**
	 * @brief Starts a LINQ query from a container (TArray, TSet, TMap, etc.).
	 * @param Source The container to query. Can be an L-Value (reference) or R-Value (temporary).
	 * @return A TLinqQuery wrapper initialized with the appropriate iterator.
	 */
	template <typename ContainerType>
	auto Start(ContainerType&& Source)
	{
		using CleanContainerType = std::remove_cvref_t<ContainerType>;
		using ValueType = TypeTrait::TContainerValueType_T<CleanContainerType>;
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