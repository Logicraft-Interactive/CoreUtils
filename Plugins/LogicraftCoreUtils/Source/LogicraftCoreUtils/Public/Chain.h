// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <source_location>
#include <concepts>
#include <type_traits>
#include <functional>

/**
 * @brief Namespace containing utilities for safe method chaining on UObjects (Monadic pattern).
 * This pattern allows executing sequences of operations where validity checks (IsValid)
 * are handled automatically between steps, significantly reducing nested "if" statements and "arrow code".
 */
namespace Chain
{
    namespace Private
    {     
       // Helper to get the raw type without pointers, const, or volatile qualifiers.
       template<typename T>
       using CleanType = std::remove_pointer_t<std::remove_cvref_t<T>>;
       
       // Concept to ensure the template type is derived from UObject.
       template<typename T>
       concept IsUObject = std::is_base_of_v<UObject, CleanType<T>>;

       template<typename Func, typename T>
       concept IsCallable = std::invocable<Func, T>;
         
       // Internal logging function to handle error reporting (implemented in cpp).
       void LOGICRAFTCOREUTILS_API ChainLog(const FString& Msg);
       
       /**
        * @brief A wrapper class that holds a UObject pointer and manages safe execution flow.
        * Acts as a "Maybe Monad": if the object is invalid (null or pending kill), 
        * subsequent Execute/Transform calls are skipped and an optional warning is logged.
        *
        * @tparam T The type of UObject being wrapped.
        */
       template<IsUObject T>
       class TChain
       {
       private:
          /**
           * @brief The wrapped object. 
           * stored as a WeakPointer to be safe against Garbage Collection during the chain execution.
           */
          TWeakObjectPtr<T> Object = nullptr;

          /**
           * @brief Controls whether a log entry is generated when the chain breaks (object becomes null).
           */
          bool bThrowWarning = false;
    
          // Internal check to verify if the wrapped object is valid for operation.
          bool CanChain() const
          {
             return Object.IsValid();
          }

          /**
           * @brief Logs a warning with specific file and line information where the chain failed.
           */
          void LogWarning(const std::source_location& SourceLocation)
          {
             if (bThrowWarning)
             {
                ChainLog(FString::Printf(TEXT("Object class %s from file %hs, line %d wasn't valid !"),
                   *T::StaticClass()->GetName(),SourceLocation.file_name(), SourceLocation.line()));  
             }  
          }

       public:
          // Default constructors and assignment operators.
          TChain() {};
          TChain(const TChain&) = default;
          TChain(TChain&&) = default;
          TChain& operator=(const TChain&) = default;
          TChain& operator=(TChain&&) = default; 
          
          // Constructors initializing the chain with a reference or pointer.
          TChain(T& InObject, bool bInThrowWarning) : Object(&InObject), bThrowWarning(bInThrowWarning) {}
          TChain(T* InObject, bool bInThrowWarning) : Object(InObject), bThrowWarning(bInThrowWarning) {}
                
          /**
           * @brief explicit accessor to the underlying raw pointer.
           * @return The raw pointer (may be nullptr).
           */
          T* Get() { return Object.Get(); }
          T* Get() const { return Object.Get(); }

          /**
           * @brief Arrow operator to allow accessing members of T directly.
           * @warning Use with caution! If the internal object is null, this acts like a standard pointer
           * and will cause a crash. Prefer using Execute() for safe access.
           */
          T* operator->() { return Get(); }
          T* operator->() const { return Get(); }
          
          /**
           * @brief Dereference operator.
           * @warning Unsafe if object is null.
           */
          T& operator*() const { return *Object; }
          T& operator*() { return *Object; }

          /**
           * @brief Implicit conversion to T*. Allows passing the chain to functions expecting a raw pointer.
           */
          operator T*() const { return Get(); }
          operator T*() { return Get(); }
          
          /**
           * @brief Invokes a callable (void function) on the wrapped object ONLY if it is valid.
           * Uses std::invoke to support Lambdas, Member function pointers, and Functors.
           *
           * @tparam Func The type of the callable.
           * @param Function The callable to execute. Must accept T* as an argument.
           * @param SourceLocation Automatically captures the file and line number for logging.
           * @return TChain& Returns a reference to self to allow further method chaining.
           */
          template<IsCallable<T*> Func>
          TChain Execute(Func&& Function, const std::source_location& SourceLocation = std::source_location::current())
          {
             if (!CanChain())
             {
                LogWarning(SourceLocation);           
                return TChain(nullptr, bThrowWarning);
             }
        
             std::invoke(Forward<Func>(Function), Object.Get());
             return *this;
          }

          /**
           * @brief Transforms the chain into a chain of a DIFFERENT type/object.
           * Useful for navigating object hierarchies (e.g., Actor -> GetComponent -> GetChild).
           * If the current object is invalid, it returns an invalid chain of the new type immediately.
           *
           * @tparam Func The type of the callable.
           * @param Function A callable that returns a pointer to a new object.
           * @param SourceLocation Automatically captures the file and line number.
           * @return TChain<NewType> A new chain wrapping the result of the function.
           */
          template<IsCallable<T*> Func>
          auto Transform(Func&& Function, const std::source_location& SourceLocation = std::source_location::current())
          -> TChain<std::remove_pointer_t<std::invoke_result_t<Func, T*>>>
          {  
             // Deduce the return type of the function to determine the new Chain type. 
             using NewType = std::remove_pointer_t<std::invoke_result_t<Func, T*>>;
             
             if (!CanChain())
             {
                LogWarning(SourceLocation);                  
                return TChain<NewType>(nullptr, bThrowWarning);
             }
        
             auto NewObject = std::invoke(Forward<Func>(Function), Object.Get());
             return TChain<NewType>(NewObject, bThrowWarning);
          }

          /**
           * @brief Terminates the chain and returns a value derived from the object.
           *
           * @tparam Func The callable type.
           * @tparam ReturnType The return type of the callable.
           * @param Function The callable to execute to get the value.
           * @param DefaultValue The value to return if the chain is broken/object is invalid.
           * @param SourceLocation Context for logging.
           * @return ReturnType The result of the function or the DefaultValue.
           */
          template<IsCallable<T*> Func, typename ReturnType = std::invoke_result_t<Func, T*>>
          TOptional<ReturnType> GetValue(Func&& Function, const std::source_location& SourceLocation = std::source_location::current())
          {
             if (!CanChain())
             {              
                LogWarning(SourceLocation);
                return NullOpt;
             }
             return TOptional<ReturnType>(std::invoke(Forward<Func>(Function), Object.Get()));
          }

          /**
           * @brief Safely attempts to Cast the internal object to a specific UObject subclass.
           * If the cast fails, the chain becomes invalid (broken).
           *
           * @tparam NewType The UObject derived class to cast to.
           * @return TChain<NewType> A new chain wrapping the casted object (or nullptr if failed).
           */
          template<IsUObject NewType>
          TChain<NewType> Cast(const std::source_location& SourceLocation = std::source_location::current())
          {
             if (!CanChain())
             {              
                LogWarning(SourceLocation);
                return TChain<NewType>(nullptr, bThrowWarning);
             }

             // Attempt Unreal Engine dynamic cast
             if (NewType* NewObject = ::Cast<NewType>(Object.Get()))
             {
                return TChain<NewType>(NewObject, bThrowWarning);
             }
             
             // Log if cast failed (chain breaks here)
             LogWarning(SourceLocation);
             return TChain<NewType>(nullptr, bThrowWarning);
          }

          /**
           * @brief Provides a fallback object if the current chain is invalid.
           *
           * @param DefaultObject The object to use if the current one is null.
           * @return TChain A new chain (either the current valid one or the default one).
           */
          TChain Or(T* DefaultObject)
          {
             return CanChain() ? *this : TChain(DefaultObject, bThrowWarning);
          }

          /**
           * @brief Executes a fallback logic/function if the chain is invalid.
           * Useful for error handling logic at the end of a chain.
           *
           * @param Function A void callable taking no arguments.
           */
          template<IsCallable<T*> Func>
          void Else(Func&& Function)
          {
             if (!CanChain())
             {
                std::invoke(Forward<Func>(Function));
             }
          }
       };
    }
    
    /**
     * @brief Entry point: Starts a new chain with a UObject pointer.
     * @param Object The starting object.
     * @param bThrowWarning Whether to log warnings if the object becomes invalid during the chain.
     * @return Private::TChain<T> A new chain instance.
     */
    template<Private::IsUObject T> 
    Private::TChain<T> StartChain(T* Object, bool bThrowWarning = true)
    {
       return Private::TChain<T>(Object, bThrowWarning);
    }

    /**
     * @brief Entry point: Starts a new chain with a UObject reference.
     * @param Object The starting object reference.
     * @param bThrowWarning Whether to log warnings if the object becomes invalid.
     * @return Private::TChain<T> A new chain instance.
     */
    template<Private::IsUObject T> 
    Private::TChain<T> StartChain(T& Object, bool bThrowWarning = true)
    {
       return Private::TChain<T>(Object, bThrowWarning);
    }

    /**
     * @brief Helper to immediately execute a single function on an object with safety checks.
     * Use this if you don't need to chain multiple calls but want the logging/validity safety
     * without writing an explicit "if (IsValid)".
     *
     * @param Object The object to operate on.
     * @param Function The callable to execute.
     * @param bThrowWarning Whether to log warnings if the object becomes invalid.
     * @param SourceLocation Automatically captures the file and line number.
     */
    template<Private::IsUObject T, Private::IsCallable<T*> Func>
    void Execute(T* Object, Func&& Function, bool bThrowWarning = true, const std::source_location& SourceLocation = std::source_location::current())
    {
       StartChain(Object, bThrowWarning).Execute(Forward<Func>(Function), SourceLocation);
    }

    /**
     * @brief Helper to immediately transform an object with safety checks.
     * Short-hand for StartChain(Obj).Transform(...)
     *
     * @param Object The object to operate on.
     * @param Function The callable that returns a new object.
     * @param bThrowWarning Whether to log warnings if the object becomes invalid.
     * @param SourceLocation Automatically captures the file and line number.
     * @return Private::TChain<NewType> A chain wrapping the result.
     */
   template<Private::IsUObject T, Private::IsCallable<T*> Func>
     auto Transform(T* Object, Func&& Function, bool bThrowWarning = true, const std::source_location& SourceLocation = std::source_location::current())
     -> Private::TChain<std::remove_pointer_t<std::invoke_result_t<Func, T*>>>
    {
       return StartChain(Object, bThrowWarning).Transform(Forward<Func>(Function), SourceLocation);
    }
}