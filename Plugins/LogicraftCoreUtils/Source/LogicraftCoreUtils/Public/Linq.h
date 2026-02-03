// Copyright (c) Logicraft Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Linq
{
	namespace Private
	{
		namespace Concept
		{
			// C++20 Concepts to constrain template types used in the library.
			// Checks if a type inherits from UObject (Unreal Engine Object).
			template <typename Ty>
			concept DerivedFromObject = std::derived_from<Ty, UObject>;
		}

		// Traits helper to determine how to store a value (as a pointer/reference or a direct value).
		// This optimization prevents unnecessary copying of complex objects during iteration.
		template <typename T>
		struct TStorageTraits
		{ 
			// If T is a reference, store it as a pointer. Otherwise, store the value directly.
			using StorageType = std::conditional_t<std::is_reference_v<T>, std::remove_reference_t<T>*, T>;
		private:
			StorageType Value;
		public:

			// Sets the stored value handling l-values and r-values appropriately.
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

			// Retrieves the value, dereferencing the pointer if strictly necessary for the storage type.
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


		// Base interface for all LINQ iterators.
		// Follows a pull-based iterator pattern (Next -> GetCurrent).
		template <typename Derived ,typename T>
		class ILinqIterator
		{ 
		public:
			virtual ~ILinqIterator() = default;

			// Advances to the next element. Returns true if successful, false if end of collection.
			bool Next()
			{
				return static_cast<Derived*>(this)->Next_Implementation();
			}

			// Resets the iterator to the beginning.
			void Reset()
			{
				static_cast<Derived*>(this)->Reset_Implementation();
			}

			// Gets the current element in the iteration.
			const T& GetCurrent()
			{
				return static_cast<Derived*>(this)->GetCurrent_Implementation();
			}
		};

		template<typename T>
		class TLinqConcept
		{
		public:
			virtual ~TLinqConcept() = default;
			
			virtual bool Next() = 0;

			virtual void Reset() = 0;

			virtual const T&  GetCurrent() = 0;
		};

		template<typename TIterator, typename T>
		requires std::is_base_of_v<ILinqIterator<TIterator, T>, TIterator>
		class TLinqModel : public TLinqConcept<T>
		{
		private:
			static constexpr bool bIsOnStack = sizeof(TIterator) < 64; 
			using Type = std::conditional_t<bIsOnStack, TIterator, TUniquePtr<TIterator>>;
			Type Iterator{};


			template<typename It>
			static Type MakeIterator(It&& InIterator)
			{
				if constexpr (bIsOnStack)
				{
					return Forward<It>(InIterator);          // move direct dans le Type (= TIterator)
				}
				else
				{
					return MakeUnique<TIterator>(Forward<It>(InIterator)); // wrap dans un UniquePtr
				}
			}
			
		public:
			template<typename It = TIterator>
			explicit TLinqModel(It&& InIterator)
			: Iterator(MakeIterator(Forward<It>(InIterator)))   // ← move-construct, no default ctor needed
			{
			}
			
			virtual ~TLinqModel() override = default;

			virtual bool Next() override
			{
				if constexpr (bIsOnStack)
				{
					return Iterator.Next();
				}
				else
				{
					return Iterator->Next(); 
				}
			}

			virtual void Reset() override
			{
				if constexpr (bIsOnStack)
				{
					Iterator.Reset();
				}
				else
				{
					Iterator->Reset(); 
				}
			}

			virtual const T& GetCurrent() override
			{
				if constexpr (bIsOnStack)
				{
					return Iterator.GetCurrent();
				}
				else
				{
					return Iterator->GetCurrent(); 
				}
			}
		};
		
		
		// Iterator for iterating over an existing TArray pointer (L-Value source).
		// Does not own the memory of the array.
		template <typename T>
		class TLValueSourceIterator : public ILinqIterator<TLValueSourceIterator<T>,T>
		{
			using FCollectionType = const TArray<T>*;
			FCollectionType Collection = nullptr;
			int Current = -1;

		public:
			explicit TLValueSourceIterator(FCollectionType InSource)
				: Collection(InSource)
			{
			}

			bool Next_Implementation()
			{
				return ++Current < Collection->Num();
			}

			void Reset_Implementation()
			{
				Current = -1;
			}

			const T& GetCurrent_Implementation()
			{
				return (*Collection)[Current];
			}
		};

		// Iterator for iterating over a temporary TArray (R-Value source).
		// Owns the memory of the array via MoveTemp.
		template <typename T>
		class TRValueSourceIterator : public ILinqIterator<TRValueSourceIterator<T>,T>
		{
			using FCollectionType = TArray<T> ;
			FCollectionType Collection{};
			int Current = -1;

		public:
			explicit TRValueSourceIterator(FCollectionType&& InSource)
				: Collection(MoveTemp(InSource))
			{
			}

			bool Next_Implementation()
			{
				return ++Current < Collection.Num();
			}

			void Reset_Implementation()
			{
				Current = -1;
			}

			const T& GetCurrent_Implementation()
			{
				return Collection[Current];
			}
		};

		// Filters elements based on a predicate (Where clause).
		template <typename T, typename Pred>
		class TWhereIterator : public ILinqIterator<TWhereIterator<T, Pred> ,T>
		{
			TUniquePtr<TLinqConcept<T>> Source = nullptr;
			const T* CurrentValue = {};
			Pred Predicate;

		public:
			template <typename P = Pred>
			requires std::is_same_v<std::remove_cvref_t<P>, std::remove_cvref_t<Pred>>
			TWhereIterator(TUniquePtr<TLinqConcept<T>>&& InSource, P&& InPredicate)
				: Source(MoveTemp(InSource)), Predicate(Forward<P>(InPredicate))
			{
			}

			bool Next_Implementation()
			{
				// Keep pulling from source until predicate is met or source is exhausted.
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

			 void Reset_Implementation()
			{
				Source->Reset();
				CurrentValue = {};
			}

			 const T& GetCurrent_Implementation()
			{
				return *CurrentValue;
			}
		};

		// Projects/Transforms elements from type 'In' to type 'Out' (Select clause).
		template <typename In, typename Out, typename Sel>
		class TSelectIterator : public ILinqIterator<TSelectIterator<In, Out, Sel>, std::remove_reference_t<Out>>
		{
			TUniquePtr<TLinqConcept<In>> Source = nullptr; 
			TStorageTraits<Out> CurrentValue = {};
			Sel Selector;
		public:
			template <typename S = Sel>
			requires std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<Sel>>
			TSelectIterator(TUniquePtr<TLinqConcept<In>>&& InSource, S&& InSelector)
				: Source(MoveTemp(InSource)), Selector(Forward<S>(InSelector))
			{
			}

			 bool Next_Implementation()
			{
				while (Source->Next())
				{
					// Apply the selector function to transform the data.
					CurrentValue.Set(Invoke(Selector, Source->GetCurrent()));
					return true;
				}
				return false;
			}

			 void Reset_Implementation()
			{
				Source->Reset();
				CurrentValue.Set({});
			}

			 const std::remove_reference_t<Out>& GetCurrent_Implementation()
			{
				return CurrentValue.Get();
			}
		};

		// Applies a modification function to elements and returns the modified element.
		template <typename T, typename Func>
		class TApplyIterator : public ILinqIterator<TApplyIterator<T, Func>, std::remove_reference_t<T>>
		{
			TUniquePtr<TLinqConcept<T>> Source = nullptr;
			TStorageTraits<T> CurrentValue;
			Func Modifier;

		public:
			template <typename F = Func>
			requires std::is_same_v<std::remove_cvref_t<F>, std::remove_cvref_t<Func>>
			TApplyIterator(TUniquePtr<TLinqConcept<T>>&& InSource, F&& InModifier)
				: Source(MoveTemp(InSource)), Modifier(Forward<F>(InModifier))
			{
			}

			 bool Next_Implementation()
			{
				while (Source->Next())
				{
					CurrentValue.Set(Invoke(Modifier, Source->GetCurrent()));
					return true;
				}
				return false;
			}

			 void Reset_Implementation()
			{
				Source->Reset();
				CurrentValue.Set({});
			}

			 const std::remove_reference_t<T>& GetCurrent_Implementation()
			{
				return CurrentValue.Get();
			}
		};

		// Executes a function on elements purely for side effects (like foreach), but maintains the chain.
		template <typename T, typename Func>
		class TExecuteIterator : public ILinqIterator<TExecuteIterator<T, Func>, T>
		{
			TUniquePtr<TLinqConcept<T>> Source = nullptr;
			Func Modifier;

		public:
			template <typename F = Func>
			requires std::is_same_v<std::remove_cvref_t<F>, std::remove_cvref_t<Func>>
			TExecuteIterator(TUniquePtr<TLinqConcept<T>>&& InSource, F&& InAction)
				: Source(MoveTemp(InSource)), Modifier(Forward<F>(InAction))
			{
			}

			 bool Next_Implementation()
			{
				while (Source->Next())
				{
					Invoke(Modifier, Source->GetCurrent());
					return true;
				}
				return false;
			}

			 void Reset_Implementation()
			{
				Source->Reset();
			}

			 const T& GetCurrent_Implementation()
			{
				return Source->GetCurrent();
			}
		};

		// Special iterator for casting Unreal Engine Objects safely.
		template <typename In, typename Out>
		requires Concept::DerivedFromObject<std::remove_pointer_t<In>>
		&& Concept::DerivedFromObject<std::remove_pointer_t<Out>>
		class TCastIterator : public ILinqIterator<TCastIterator<In, Out>, Out>
		{
			TUniquePtr<TLinqConcept<In>> Source = nullptr;
			Out CurrentValue = {};

		public:
			TCastIterator(TUniquePtr<TLinqConcept<In>>&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			 bool Next_Implementation()
			{
				while (Source->Next())
				{
					// Uses Unreal's Cast<> and only returns successfully cast objects (implicitly filters nulls).
					if (CurrentValue = Cast<std::remove_pointer_t<Out>>(Source->GetCurrent()); CurrentValue)
					{
						return true;
					}
				}
				return false;
			}

			 void Reset_Implementation()
			{
				Source->Reset();
				CurrentValue = {};
			}

			 const Out& GetCurrent_Implementation()
			{
				return CurrentValue;
			}
		};

		// Filters out invalid Unreal Objects (checks IsValid).
		template <typename T>
		requires Concept::DerivedFromObject<std::remove_pointer_t<T>>
		class TIsValidIterator : public ILinqIterator<TIsValidIterator<T>, T>
		{
			TUniquePtr<TLinqConcept<T>> Source = nullptr;

		public:
			TIsValidIterator(TUniquePtr<TLinqConcept<T>>&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			 bool Next_Implementation()
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

			 void Reset_Implementation()
			{
				Source->Reset();
			}

			 const T& GetCurrent_Implementation()
			{
				return Source->GetCurrent();
			}
		};

		// Limits the number of elements returned.
		template<typename T>
		class TTakeIterator : public ILinqIterator<TTakeIterator<T>, T>
		{
			TUniquePtr<TLinqConcept<T>> Source = nullptr;
			int MaxElement = 0;
			int Counter = 0;
		public:
			TTakeIterator(TUniquePtr<TLinqConcept<T>>&& InSource, const int InNumber)
				: Source(MoveTemp(InSource)), MaxElement(InNumber)
			{
			}

			 bool Next_Implementation()
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

			 void Reset_Implementation()
			{
				Source->Reset();
				Counter = 0;
			}

			 const T& GetCurrent_Implementation()
			{
				return Source->GetCurrent();
			}
		};

		// Bypasses a specified number of elements and returns the rest.
		template<typename T>
		class TSkipIterator : public ILinqIterator<TSkipIterator<T>, T>
		{
			TUniquePtr<TLinqConcept<T>> Source = nullptr;
			int MaxElement = 0;
			int Counter = 0;
		public:
			TSkipIterator(TUniquePtr<TLinqConcept<T>>&& InSource, const int InNumber)
				: Source(MoveTemp(InSource)), MaxElement(InNumber)
			{
			}

			bool Next_Implementation()
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

			void Reset_Implementation()
			{
				Source->Reset();
				Counter = 0;
			}

			const T& GetCurrent_Implementation()
			{
				return Source->GetCurrent();
			}
		};
		
		// Base class for sorting iterators. 
		// NOTE: Sorting breaks the pure lazy evaluation pipeline because all data must be fetched and sorted before the first element can be returned.
		template<typename T, typename Derived>
		class TOrderByBaseIterator : public ILinqIterator<Derived, T>
		{
			TUniquePtr<TLinqConcept<T>> Source = nullptr;
			bool bHasReset = true;
			int CurrentIndex = -1;

			void BuildNewCollection()
			{
				if (!bHasReset)
				{
					return;
				}
				bHasReset = false;
				// Materialize the collection entirely.
				while (Source->Next())
				{
					NewCollection.Add(Source->GetCurrent());
				}
				// Perform the sort.
				static_cast<Derived*>(this)->SortMethod();
			}
		protected:
			TArray<T> NewCollection = {};
			
		public:
			TOrderByBaseIterator(TUniquePtr<TLinqConcept<T>>&& InSource)
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
				Source->Reset();
				NewCollection.Empty();
				bHasReset = true;
				CurrentIndex = -1;
			}

			 const T& GetCurrent_Implementation()
			{
				return NewCollection[CurrentIndex];
			}
		};

		// Standard default sorting.
		template<typename T>
		requires std::totally_ordered<T> ||
			requires (T Result){ Result < std::declval<decltype(Result)>(); }
		class TSimpleOrderByIterator : public TOrderByBaseIterator<T, TSimpleOrderByIterator<T>>
		{
		protected:
			friend TOrderByBaseIterator<T, TSimpleOrderByIterator<T>>;
			
			void SortMethod()
			{
				Algo::Sort(this->NewCollection);
			}
			
		public:
			explicit TSimpleOrderByIterator(TUniquePtr<TLinqConcept<T>>&& InSource)
				: TOrderByBaseIterator<T, TSimpleOrderByIterator<T>>(MoveTemp(InSource))
			{
			}
		};

		// Sorting by a specific property selector.
		template<typename T, typename Sel>
		requires std::totally_ordered<std::invoke_result_t<Sel, T>> ||
			requires (std::invoke_result_t<Sel, T> Result){ Result <std::declval<decltype(Result)>(); }
		class TSelectorOrderByIterator : public TOrderByBaseIterator<T, TSelectorOrderByIterator<T, Sel>>
		{
			Sel Selector;
			friend TOrderByBaseIterator<T, TSelectorOrderByIterator<T, Sel>>;
		protected:
			void SortMethod()
			{
				Algo::SortBy(this->NewCollection, Selector);
			}
			
		public:
			template<typename S = Sel>
			requires std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<Sel>>
			explicit TSelectorOrderByIterator(TUniquePtr<TLinqConcept<T>>&& InSource, Sel&& InSelector)
				: TOrderByBaseIterator<T, TSelectorOrderByIterator<T, Sel>>(MoveTemp(InSource)), Selector(Forward<S>(InSelector))
			{
			}
		};

		// Sorting using a custom comparator.
		template<typename T, typename Comp>
		requires requires(T a, T b){ {a < b} -> std::convertible_to<bool>; } || std::totally_ordered<T>
		class TComparatorOrderByIterator : public TOrderByBaseIterator<T, TComparatorOrderByIterator<T, Comp>>
		{
			Comp Selector;
			friend TOrderByBaseIterator<T, TComparatorOrderByIterator<T, Comp>>;
		protected:
			void SortMethod()
			{
				Algo::Sort(this->NewCollection, Selector);
			}
			
		public:
			template<typename C = Comp>
			requires std::is_same_v<std::remove_cvref_t<C>, std::remove_cvref_t<Comp>>
			explicit TComparatorOrderByIterator(TUniquePtr<TLinqConcept<T>>&& InSource, C&& InSelector)
				: TOrderByBaseIterator<T, TComparatorOrderByIterator<T, Comp>>(MoveTemp(InSource)), Selector(Forward<C>(InSelector))
			{
			}
		};

		// The main wrapper class exposed to the user. 
		// It holds the current iterator chain and provides the fluent API methods.
		template <typename T>
		class TLinqQuery
		{
			TUniquePtr<TLinqConcept<T>> Iterator;

		public:
			explicit TLinqQuery(TUniquePtr<TLinqConcept<T>>&& InSource) : Iterator(MoveTemp(InSource))
			{
			}

			// --- Intermediate Operations (Return TLinqQuery) ---

			// Filters the sequence based on a predicate.
			template <typename Pred>
			TLinqQuery<T> Where(Pred&& Predicate)
			{
				using IteratorType = TWhereIterator<T, Pred>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Forward<Pred>(Predicate))));
			}

			// Projects each element of the sequence into a new form.
			template <typename Sel, typename Out = std::invoke_result_t<Sel, T>>
			TLinqQuery<Out> Select(Sel&& Selector)
			{
				using IteratorType = TSelectIterator<T, Out, Sel>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Forward<Sel>(Selector))));
			}

			// Applies a function to modify elements in place (returns modified sequence).
			template <typename Func>
			TLinqQuery<T> Apply(Func&& Modifier)
			{
				using IteratorType = TApplyIterator<T, Func>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Forward<Func>(Modifier))));
			}

			// Executes a function on each element (side effect) without altering the stream type.
			template <typename Func>
			TLinqQuery<T> Execute(Func&& Modifier)
			{				
				using IteratorType = TExecuteIterator<T, Func>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Forward<Func>(Modifier))));
			}

			// Sorts the elements using a custom comparator.
			template <typename Comp>
			requires std::predicate<Comp, T, T>
			TLinqQuery<T> OrderBy(Comp&& Comparator)
			{
				using IteratorType = TComparatorOrderByIterator<T, Comp>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Forward<Comp>(Comparator))));
				
			}
 
			// Sorts the elements using default comparison.
			TLinqQuery  OrderBy()
			{
				using IteratorType = TSimpleOrderByIterator<T>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator))));
			}

			// Sorts the elements based on a key returned by the selector.
			template <typename Sel>
			TLinqQuery<T> OrderBy(Sel&& Selector)
			{
				using IteratorType = TSelectorOrderByIterator<T, Sel>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Forward<Sel>(Selector))));
			}

			// Casts elements to a specific UObject derived type (filters out failed casts).
			template <typename NewType>
			TLinqQuery<NewType*> Cast()
			{
				using IteratorType = TCastIterator<T, NewType*>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator))));
			}

			// Filters out invalid objects (Isvalid(Obj) == false).
			template <typename NewType = T>
			TLinqQuery IsValid()
			{
				using IteratorType = TIsValidIterator<T>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator))));
			}

			// Bypasses a specified number of elements.
			TLinqQuery Skip(const int Number)
			{
				using IteratorType = TSkipIterator<T>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Number)));
				
			}
 
			// Returns a specified number of contiguous elements from the start.
			TLinqQuery Take(const int Number)
			{
				using IteratorType = TTakeIterator<T>;
				return TLinqQuery<T>(MakeUnique<TLinqModel<IteratorType, T>>(IteratorType(
					MoveTemp(Iterator), Number)));
			}

			// --- Terminal Operations (Execute the Query) ---

			// Returns the first element satisfying the predicate. Crashes if not found.
			template <typename Pred>
			T First(Pred&& Predicate)
			{
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Predicate, Iterator->GetCurrent()))
					{
						return Iterator->GetCurrent();
					}
				}
				ensureMsgf(false, TEXT("Unable to find object that satisfies predicate expression!"));
				return {};
			}

			// Returns the first element satisfying the predicate, or a default value if not found.
			template <typename Pred, typename DefValue = T>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<DefValue>>
			T FirstOrDefault(Pred&& Predicate, DefValue&& DefaultValue)
			{
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Predicate, Iterator->GetCurrent()))
					{
						return Iterator->GetCurrent();
					}
				}
				return Forward<DefValue>(DefaultValue);
			}

			// Returns the last element satisfying the predicate. Crashes if not found.
			template <typename Pred>
			T Last(Pred&& Predicate)
			{
				Iterator->Reset();
				TOptional<T> LastFound;
				while (Iterator->Next())
				{
					if (Invoke(Predicate, Iterator->GetCurrent()))
					{
						LastFound = Iterator->GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				ensureMsgf(false, TEXT("Unable to find object that satisfies predicate expression!"));
				return {};
			}

			// Returns the last element satisfying the predicate, or a default value.
			template <typename Pred, typename DefValue = T>
			requires std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<DefValue>>
			T LastOrDefault(Pred&& Predicate, DefValue&& DefaultValue)
			{
				Iterator->Reset();
				TOptional<T> LastFound;
				while (Iterator->Next())
				{
					if (Invoke(Predicate, Iterator->GetCurrent()))
					{
						LastFound = Iterator->GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				return Forward<DefValue>(DefaultValue);
			}

			// Returns the only element satisfying the predicate. Crashes if none or more than one exist.
			template <typename Pred>
			T Single(Pred&& Predicate)
			{
				Iterator->Reset();
				TOptional<T> LastFound;
				while (Iterator->Next())
				{
					if (Invoke(Predicate, Iterator->GetCurrent()))
					{
						if (LastFound)
						{
							ensureMsgf(false, TEXT("Two objects found when only one was expected!"));
							return {};
						}
						LastFound = Iterator->GetCurrent();
					}
				}
				if (LastFound)
				{
					return *LastFound;
				}
				ensureMsgf(false, TEXT("No objects found when only one was expected!"));
				return {};
			}

			// Returns the number of elements satisfying the predicate.
			template <typename Pred>
			int Count(Pred&& Predicate)
			{
				int Counter = 0;
				Iterator->Reset();
				while (Iterator->Next())
				{
					if (Invoke(Predicate, Iterator->GetCurrent()))
					{
						++Counter;
					}
				}

				return Counter;
			}

			// Computes the sum of the sequence transformed by a selector.
			template <typename Sel, typename ReturnType = std::invoke_result_t<Sel, T>>
			requires std::is_arithmetic_v<ReturnType>
			ReturnType Sum(Sel&& Selector)
			{
				ReturnType Result{};
				Iterator->Reset();
				while (Iterator->Next())
				{
					Result += Invoke(Selector, Iterator->GetCurrent());
				}
				return Result;
			}

			// Computes the sum of the sequence (arithmetic types only).
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

			// Finds the minimum value using a comparator.
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
 
			// Finds the minimum value (using operator <).
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

			// Finds the maximum value using a comparator.
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
 
			// Finds the maximum value (using operator >).
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

			// Determines whether any element of a sequence exists or satisfies a condition.
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

			// Determines whether all elements of a sequence satisfy a condition.
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

			// Determines whether the sequence contains a specific value using a comparator.
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
 
			// Determines whether the sequence contains a specific value using equality.
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

			// Materializes the query results into a TArray.
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

			// Iterates through the collection without returning a value (useful for side effects in previous Execute calls).
			void End()
			{
				Iterator->Reset();
				while (Iterator->Next())
				{}
			}
			
		};
	}

	// --- Entry Points ---

	// Starts a Linq query from an existing TArray variable (L-Value).
	template <typename T>
	Private::TLinqQuery<T> Start(const TArray<T>& Source)
	{
		using IteratorType = Private::TLValueSourceIterator<T>;
		return Private::TLinqQuery<T>(MakeUnique<Private::TLinqModel<IteratorType, T>>(IteratorType(&Source)));
	}

	// Starts a Linq query from a temporary TArray (R-Value), taking ownership.
	template <typename T>
	Private::TLinqQuery<T> Start(TArray<T>&& Source)
	{
		using IteratorType = Private::TRValueSourceIterator<T>;
		return Private::TLinqQuery<T>(MakeUnique<Private::TLinqModel<IteratorType, T>>(IteratorType(MoveTemp(Source))));
	}
}