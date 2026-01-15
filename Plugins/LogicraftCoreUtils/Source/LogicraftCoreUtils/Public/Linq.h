// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

namespace Linq
{
	namespace Private
	{
		namespace Concept
		{
			// C++20 Concepts to constrain template types used in the library.

			// Checks if a type is specifically a TArray.
			template <typename Collection, typename T>
			concept IsCollection = std::is_same_v<Collection, TArray<T>>;

			// Checks if a type inherits from UObject (Unreal Engine Object).
			template <typename Ty>
			concept DerivedFromObject = std::derived_from<Ty, UObject>;

			// Helper to deduce the value type contained within a TArray-like structure.
			template <TIsArray T>
			struct GetArrayValueType
			{
				using Type = std::remove_cvref_t<decltype(*std::declval<T>().begin())>;
			};
		}

		// Traits helper to determine how to store a value (as a pointer/reference or a direct value).
		// This optimization prevents unnecessary copying of complex objects during iteration.
		template <typename T>
		struct TStorageTraits
		{ 
			// If T is a reference, store it as a pointer. Otherwise, store the value directly.
			static constexpr bool bIsReference = std::is_reference_v<T>;
			using CleanType = std::remove_cvref_t<T>;
			using StorageType = std::conditional_t<bIsReference,const CleanType*, T>;
		private:
			StorageType Value;
		public:

			// Sets the stored value handling l-values and r-values appropriately.
			template<typename Ty = T>
			requires std::is_same_v<std::remove_cvref_t<Ty>, std::remove_cvref_t<T>>
			void Set(Ty&& InValue)
			{
				if constexpr (bIsReference)
				{
					Value = &InValue; 
				}
				else
				{
					Value = Forward<T>(InValue);
				}
			}

			// Retrieves the value, dereferencing the pointer if strictly necessary for the storage type.
			const CleanType& Get()
			{
				if constexpr (bIsReference)
				{
					return *Value; 
				}
				else
				{
					return Value;
				}
			}
		};

		template<typename ArrayType, typename ValueType = typename std::remove_cvref_t<ArrayType>::ElementType>
		class ILinqCollection
		{
			using CleanArrayType = std::remove_cvref_t<ArrayType>;
		public:
			virtual ~ILinqCollection() = default;
			
			virtual void Reset() = 0;

			virtual bool Next() = 0;

			virtual const ValueType& GetCurrent() = 0;

			virtual const CleanArrayType& GetArray() = 0;
 		};
 
		template<typename ArrayType, typename TrueType, typename ValueType = typename std::remove_cvref_t<ArrayType>::ElementType>
		requires std::is_same_v<ArrayType, TArray<ValueType>>
		class TLinqTArray : public ILinqCollection<ArrayType>
		{			
			TStorageTraits<TrueType> Collection = {};
			int CurrentIndex = -1;

		public:
			explicit TLinqTArray(const ArrayType& InCollection)
			{
				Collection.Set(InCollection);
			}

			explicit TLinqTArray(ArrayType&& InCollection)
			{
				Collection.Set(MoveTemp(InCollection)); 
			}
			
			virtual void Reset() override
			{
				CurrentIndex = -1;
			}

			virtual bool Next() override
			{
				return ++CurrentIndex < Collection.Get().Num();
			}

			virtual const ValueType& GetCurrent() override
			{
				return Collection.Get()[CurrentIndex];
			}

			virtual const ArrayType& GetArray() override
			{
				return Collection.Get();
			}
		};


		// Base interface for all LINQ iterators.
		// Follows a pull-based iterator pattern (Next -> GetCurrent).
		template <typename T>
		class ILinqIterator
		{
		public:
			virtual ~ILinqIterator() = default;

			// Advances to the next element. Returns true if successful, false if end of collection.
			virtual bool Next() = 0;

			// Resets the iterator to the beginning.
			virtual void Reset() = 0;

			// Gets the current element in the iteration.
			virtual const T& GetCurrent() = 0;
		};

		
		template <typename T, typename ArrayType>
		class TSourceIterator : public ILinqIterator<T>
		{
			TUniquePtr<ILinqCollection<ArrayType>> Collection;

		public:
			explicit TSourceIterator(TUniquePtr<ILinqCollection<ArrayType>>&& InSource)
				: Collection(MoveTemp(InSource))
			{
			}


			virtual bool Next() override
			{
				return Collection->Next();
			}

			virtual void Reset() override
			{
				Collection->Reset();
			}

			virtual const T& GetCurrent() override
			{
				return Collection->GetCurrent();
			}
		};
		
		// Iterator for iterating over an existing TArray pointer (L-Value source).
		// Does not own the memory of the array.
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

		// Iterator for iterating over a temporary TArray (R-Value source).
		// Owns the memory of the array via MoveTemp.
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

		// Filters elements based on a predicate (Where clause).
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

		// Projects/Transforms elements from type 'In' to type 'Out' (Select clause).
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
					// Apply the selector function to transform the data.
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

		// Applies a modification function to elements and returns the modified element.
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

		// Executes a function on elements purely for side effects (like foreach), but maintains the chain.
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

		// Special iterator for casting Unreal Engine Objects safely.
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
					// Uses Unreal's Cast<> and only returns successfully cast objects (implicitly filters nulls).
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

		// Filters out invalid Unreal Objects (checks IsValid).
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

		// Limits the number of elements returned.
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

		// Bypasses a specified number of elements and returns the rest.
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
		
		// Base class for sorting iterators. 
		// NOTE: Sorting breaks the pure lazy evaluation pipeline because all data must be fetched and sorted before the first element can be returned.
		template<typename T>
		class TOrderByBaseIterator : public ILinqIterator<T>
		{
			TUniquePtr<ILinqIterator<T>> Source = nullptr;
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
				SortMethod();
			}
		protected:
			TArray<T> NewCollection = {};
			virtual void SortMethod() = 0;
			
		public:
			TOrderByBaseIterator(TUniquePtr<ILinqIterator<T>>&& InSource)
				: Source(MoveTemp(InSource))
			{
			}

			virtual bool Next() override
			{	
				BuildNewCollection();

				if (++CurrentIndex < NewCollection.Num())
				{
					return true;
				}			
				
				return false;
			}

			virtual void Reset() override
			{
				Source->Reset();
				NewCollection.Empty();
				bHasReset = true;
				CurrentIndex = -1;
			}

			virtual const T& GetCurrent() override
			{
				return NewCollection[CurrentIndex];
			}
		};

		// Standard default sorting.
		template<typename T>
		requires std::totally_ordered<T>
		class TSimpleOrderByIterator : public TOrderByBaseIterator<T>
		{
		protected:
			virtual void SortMethod() override
			{
				Algo::Sort(this->NewCollection);
			}
			
		public:
			explicit TSimpleOrderByIterator(TUniquePtr<ILinqIterator<T>>&& InSource)
				: TOrderByBaseIterator<T>(MoveTemp(InSource))
			{
			}
		};

		// Sorting by a specific property selector.
		template<typename T, typename Sel>
		requires std::is_convertible_v<std::invoke_result_t<Sel, T>,bool>
		class TSelectorOrderByIterator : public TOrderByBaseIterator<T>
		{
			Sel Selector;
		protected:
			virtual void SortMethod() override
			{
				Algo::SortBy(this->NewCollection, Selector);
			}
			
		public:
			template<typename S = Sel>
			requires std::is_same_v<S, Sel>
			explicit TSelectorOrderByIterator(TUniquePtr<ILinqIterator<T>>&& InSource, Sel&& InSelector)
				: TOrderByBaseIterator<T>(MoveTemp(InSource)), Selector(Forward<Sel>(InSelector))
			{
			}
		};

		// Sorting using a custom comparator.
		template<typename T, typename Comp>
		requires requires(T a, T b){ {a.operator<(b)} -> std::convertible_to<bool>; } || std::totally_ordered<T>
		class TComparatorOrderByIterator : public TOrderByBaseIterator<T>
		{
			Comp Selector;
		protected:
			virtual void SortMethod() override
			{
				Algo::Sort(this->NewCollection, Selector);
			}
			
		public:
			template<typename C = Comp>
			requires std::is_same_v<C, Comp>
			explicit TComparatorOrderByIterator(TUniquePtr<ILinqIterator<T>>&& InSource, C&& InSelector)
				: TOrderByBaseIterator<T>(MoveTemp(InSource)), Selector(Forward<C>(InSelector))
			{
			}
		};

		// The main wrapper class exposed to the user. 
		// It holds the current iterator chain and provides the fluent API methods.
		template <typename T>
		class TLinqQuery
		{
			TUniquePtr<ILinqIterator<T>> Iterator;

		public:
			explicit TLinqQuery(TUniquePtr<ILinqIterator<T>>&& InSource) : Iterator(MoveTemp(InSource))
			{
			}

			// --- Intermediate Operations (Return TLinqQuery) ---

			// Filters the sequence based on a predicate.
			template <typename Pred>
			TLinqQuery<T> Where(Pred&& Predicate)
			{
				return TLinqQuery<T>(MakeUnique<TWhereIterator<T, Pred>>(
					MoveTemp(Iterator), Forward<Pred>(Predicate)));
			}

			// Projects each element of the sequence into a new form.
			template <typename Sel, typename Out = std::invoke_result_t<Sel, T>>
			TLinqQuery<Out> Select(Sel&& Selector)
			{
				return TLinqQuery<Out>(MakeUnique<TSelectIterator<T, Out, Sel>>(
					MoveTemp(Iterator), Forward<Sel>(Selector)));
			}

			// Applies a function to modify elements in place (returns modified sequence).
			template <typename Func>
			TLinqQuery<T> Apply(Func&& Modifier)
			{
				return TLinqQuery<T>(MakeUnique<TApplyIterator<T, Func>>(
					MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			// Executes a function on each element (side effect) without altering the stream type.
			template <typename Func>
			TLinqQuery<T> Execute(Func&& Modifier)
			{
				return TLinqQuery<T>(MakeUnique<TExecuteIterator<T, Func>>(
					MoveTemp(Iterator), Forward<Func>(Modifier)));
			}

			// Sorts the elements using a custom comparator.
			template <typename Comp>
			requires std::predicate<Comp, T, T>
			TLinqQuery<T> OrderBy(Comp&& Comparator)
			{
				return TLinqQuery<T>(MakeUnique<TComparatorOrderByIterator<T, Comp>>(
					MoveTemp(Iterator), Forward<Comp>(Comparator)));
			}
 
			// Sorts the elements using default comparison.
			TLinqQuery  OrderBy()
			{
				return TLinqQuery (MakeUnique<TSimpleOrderByIterator<T>>(
					MoveTemp(Iterator)));
			}

			// Sorts the elements based on a key returned by the selector.
			template <typename Sel>
			requires std::predicate<Sel, T>
			TLinqQuery<T> OrderBy(Sel&& Selector)
			{
				return TLinqQuery<T>(MakeUnique<TSelectorOrderByIterator<T, Sel>>(
					MoveTemp(Iterator), Forward<Sel>(Selector)));
			}

			// Casts elements to a specific UObject derived type (filters out failed casts).
			template <typename NewType>
			TLinqQuery<NewType*> Cast()
			{
				return TLinqQuery<NewType*>(MakeUnique<TCastIterator<T, NewType*>>(
					MoveTemp(Iterator)));
			}

			// Filters out invalid objects (Isvalid(Obj) == false).
			template <typename NewType = T>
			TLinqQuery IsValid()
			{
				return TLinqQuery<NewType>(MakeUnique<TIsValidIterator<T>>(
					MoveTemp(Iterator)));
			}

			// Bypasses a specified number of elements.
			TLinqQuery Skip(const int Number)
			{
				return TLinqQuery(MakeUnique<TSkipIterator<T>>(
					MoveTemp(Iterator), Number));
			}
 
			// Returns a specified number of contiguous elements from the start.
			TLinqQuery Take(const int Number)
			{
				return TLinqQuery(MakeUnique<TTakeIterator<T>>(
					MoveTemp(Iterator), Number));
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
			requires std::is_same_v<T, DefValue>
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
				return Forward<T>(DefaultValue);
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
			requires std::is_same_v<T, DefValue>
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
				return Forward<T>(DefaultValue);
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

	template<typename ArrayType, typename ValueType = std::remove_cvref_t<ArrayType>::ElementType>
	requires std::is_same_v<std::remove_cvref_t<ArrayType>, TArray<ValueType>>
	Private::TLinqQuery<ValueType> StartCustom(ArrayType&& Source)
	{
		return Private::TLinqQuery<ValueType>(MakeUnique<Private::TSourceIterator<ValueType, std::remove_cvref_t<ArrayType>>>(
			MakeUnique<Private::TLinqTArray<std::remove_cvref_t<ArrayType>, decltype(Source)>>(Forward<ArrayType>(Source))));
	}

	// Starts a Linq query from an existing TArray variable (L-Value).
	template <typename T>
	Private::TLinqQuery<T> Start(TArray<T>& Source)
	{
		return Private::TLinqQuery<T>(MakeUnique<Private::TLValueSourceIterator<T>>(&Source));
	}

	// Starts a Linq query from a temporary TArray (R-Value), taking ownership.
	template <typename T>
	Private::TLinqQuery<T> Start(TArray<T>&& Source)
	{
		return Private::TLinqQuery<T>(MakeUnique<Private::TRValueSourceIterator<T>>(MoveTemp(Source)));
	}
}