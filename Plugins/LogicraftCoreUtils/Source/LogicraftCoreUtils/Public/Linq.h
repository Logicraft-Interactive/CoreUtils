// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h" 

namespace Linq
{
	namespace Private
	{
		namespace Concept
		{						
			template<typename T>
			concept TIsArray = requires(T a)
			{
				a.begin();
				a.end();
				a.Num();
			};

		    template<TIsArray T>
		    struct GetArrayValueType
		    {
		        using Type =  std::remove_cvref_t<decltype(*std::declval<T>().begin())>; 
		    };
		}


		template<typename T>
		class ILinqIterator
		{
		public:
			virtual ~ILinqIterator() = default;

			virtual bool Next() = 0;
    
		    virtual void Reset() = 0;
 
			virtual T GetCurrent() = 0;
		};

        template<typename T>
	    class TSourceIterator : public ILinqIterator<T>
        {
            const TArray<T>* Collection = nullptr;
            int Current = -1;            
        public:
        	explicit TSourceIterator(const TArray<T>* InSource)
			: Collection(InSource) {}
        	
            virtual bool Next() override
            {
                return ++Current < Collection->Num();
            }
            
            virtual void Reset() override
            {
                Current = -1;
            }
            
            virtual T GetCurrent() override
            {
                return (*Collection)[Current];
            }
        };

	    template<typename T, typename Pred>
	    class TWhereIterator : public ILinqIterator<T>
	    {
	    	TUniquePtr<ILinqIterator<T>> Source = nullptr;
	        T CurrentValue;
			Pred Predicate;
	    	
	    public:
	    	template<typename P = Pred>
	    	TWhereIterator(TUniquePtr<ILinqIterator<T>>&& InSource, P&& InPredicate)
	    	: Source(MoveTemp(InSource)), Predicate(Forward<Pred>(InPredicate))
	    	{}
	    	
	        virtual bool Next() override
	        {
	        	while (Source->Next())
	        	{
	        		CurrentValue = Source->GetCurrent();
	        		return Predicate(CurrentValue);
	        	}
	        	return false;
	        }
            
	        virtual void Reset() override
	        {
	        	Source->Reset();
	        }
            
	        virtual T GetCurrent() override
	        {
	        	return CurrentValue;
	        }
	    };

	    
		template<typename T>
		class TLinqQuery
		{
			TUniquePtr<ILinqIterator<T>> Iterator;

		public:
			explicit TLinqQuery(TUniquePtr<ILinqIterator<T>>&& InSource) : Iterator(MoveTemp(InSource))
			{}

			template<typename Pred>
			TLinqQuery<T> Where(Pred&& Predicate)
			{
				return TLinqQuery<T>(MakeUnique<TWhereIterator<T, Pred>>(
					MoveTemp(Iterator), Forward<Pred>(Predicate)));
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
		};
		
	}

	template<typename T>
	Private::TLinqQuery<T> Linq(TArray<T>& Source)
	{
		return Private::TLinqQuery<T>(MakeUnique<Private::TSourceIterator<T>>(&Source));
	}
}

#include "CoreMinimal.h"
#include "Algo/Transform.h"
#include "Algo/Accumulate.h"

// Base enumerator interface
template<typename T>
class IEnumerator
{
public:
    virtual ~IEnumerator() = default;
    virtual bool MoveNext() = 0;
    virtual T Current() const = 0;
    virtual void Reset() = 0;
};

// Source enumerator - wraps a TArray
template<typename T>
class TSourceEnumerator : public IEnumerator<T>
{
private:
    const TArray<T>* Source;
    int32 Index;

public:
    explicit TSourceEnumerator(const TArray<T>* InSource)
        : Source(InSource), Index(-1) {}

    virtual bool MoveNext() override
    {
        ++Index;
        return Index < Source->Num();
    }

    virtual T Current() const override
    {
        return (*Source)[Index];
    }

    virtual void Reset() override
    {
        Index = -1;
    }
};

// Where enumerator - filters lazily
template<typename T, typename PredicateType>
class TWhereEnumerator : public IEnumerator<T>
{
private:
    TUniquePtr<IEnumerator<T>> Source;
    PredicateType Predicate;
    T CurrentValue;

public:
    TWhereEnumerator(TUniquePtr<IEnumerator<T>>&& InSource, PredicateType InPredicate)
        : Source(MoveTemp(InSource)), Predicate(InPredicate) {}

    virtual bool MoveNext() override
    {
        while (Source->MoveNext())
        {
            CurrentValue = Source->Current();
            if (Predicate(CurrentValue))
            {
                return true;
            }
        }
        return false;
    }

    virtual T Current() const override
    {
        return CurrentValue;
    }

    virtual void Reset() override
    {
        Source->Reset();
    }
};

// Select enumerator - transforms lazily
template<typename TIn, typename TOut, typename SelectorType>
class TSelectEnumerator : public IEnumerator<TOut>
{
private:
    TUniquePtr<IEnumerator<TIn>> Source;
    SelectorType Selector;
    TOut CurrentValue;

public:
    TSelectEnumerator(TUniquePtr<IEnumerator<TIn>>&& InSource, SelectorType InSelector)
        : Source(MoveTemp(InSource)), Selector(InSelector) {}

    virtual bool MoveNext() override
    {
        if (Source->MoveNext())
        {
            CurrentValue = Selector(Source->Current());
            return true;
        }
        return false;
    }

    virtual TOut Current() const override
    {
        return CurrentValue;
    }

    virtual void Reset() override
    {
        Source->Reset();
    }
};

// Take enumerator - limits count lazily
template<typename T>
class TTakeEnumerator : public IEnumerator<T>
{
private:
    TUniquePtr<IEnumerator<T>> Source;
    int32 Count;
    int32 CurrentCount;

public:
    TTakeEnumerator(TUniquePtr<IEnumerator<T>>&& InSource, int32 InCount)
        : Source(MoveTemp(InSource)), Count(InCount), CurrentCount(0) {}

    virtual bool MoveNext() override
    {
        if (CurrentCount >= Count)
        {
            return false;
        }
        
        if (Source->MoveNext())
        {
            ++CurrentCount;
            return true;
        }
        return false;
    }

    virtual T Current() const override
    {
        return Source->Current();
    }

    virtual void Reset() override
    {
        Source->Reset();
        CurrentCount = 0;
    }
};
