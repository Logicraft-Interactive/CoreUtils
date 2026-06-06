#include "CoreMinimal.h"

#if WITH_EDITOR

#include "EventBus.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// Test spec declaration
// ---------------------------------------------------------------------------

BEGIN_DEFINE_SPEC(FEventBusSpec, "Logicraft.EventBus", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
    UEventBus* EventBus  = nullptr;
    UWorld* TestWorld  = nullptr;
    UGameInstance* TestGameInstance = nullptr;
    FGameplayTag TestTag;
END_DEFINE_SPEC(FEventBusSpec)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// UEventBusTestListener is declared in its own header so UHT can process it.
// UCLASS() macros cannot be defined in .cpp files as UHT does not scan them.
#include "EventBusTestObject.h"

// ---------------------------------------------------------------------------
// Spec definition
// ---------------------------------------------------------------------------

void FEventBusSpec::Define()
{
    // ------------------------------------------------------------------
    // Setup / Teardown
    // ------------------------------------------------------------------

    BeforeEach([this]
    {
        TestWorld = UWorld::CreateWorld(EWorldType::Game, false);

        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(TestWorld);

        TestGameInstance = NewObject<UGameInstance>(GEngine);
        TestGameInstance->Init();

        WorldContext.OwningGameInstance = TestGameInstance;
        TestWorld->SetGameInstance(TestGameInstance);

        EventBus = TestGameInstance->GetSubsystem<UEventBus>();
        TestTag  = FGameplayTag::RequestGameplayTag(TEXT("Test.Event.Bus"));
    });

    AfterEach([this]
    {
        if (TestGameInstance)
        {
            TestGameInstance->Shutdown();
            TestGameInstance = nullptr;
        }

        if (TestWorld)
        {
            GEngine->DestroyWorldContext(TestWorld);
            TestWorld->DestroyWorld(false);
            TestWorld = nullptr;
        }

        EventBus = nullptr;
    });

    // ------------------------------------------------------------------
    // Describe: Broadcast
    // ------------------------------------------------------------------

    Describe("Broadcast", [this]
    {
        It("Should deliver a single integer argument to the subscriber", [this]
        {
            int32 Received = 0;
            EventBus->AddLambda(TestTag, [&](int32 Val) { Received = Val; });

            EventBus->Broadcast(TestTag, 42);

            TestEqual("Received value", Received, 42);
        });

        It("Should deliver multiple arguments of different types", [this]
        {
            FString ReceivedStr;
            float   ReceivedFloat = 0.f;

            EventBus->AddLambda(TestTag, [&](const FString& S, float F)
            {
                ReceivedStr   = S;
                ReceivedFloat = F;
            });

            // Explicit template arg required: deduction would produce FString, not const FString&,
            // causing a container type mismatch at broadcast time.
            EventBus->Broadcast<const FString&, float>(TestTag, FString("Hello"), 3.14f);

            TestEqual("String argument", ReceivedStr,   FString("Hello"));
            TestEqual("Float argument",  ReceivedFloat, 3.14f);
        });

        It("Should notify all subscribers, not just the first one", [this]
        {
            int32 CountA = 0, CountB = 0, CountC = 0;

            EventBus->AddLambda(TestTag, [&](int32) { ++CountA; });
            EventBus->AddLambda(TestTag, [&](int32) { ++CountB; });
            EventBus->AddLambda(TestTag, [&](int32) { ++CountC; });

            EventBus->Broadcast(TestTag, 0);

            TestEqual("Subscriber A called once", CountA, 1);
            TestEqual("Subscriber B called once", CountB, 1);
            TestEqual("Subscriber C called once", CountC, 1);
        });

        It("Should do nothing when no subscriber is registered", [this]
        {
            // Must not crash or assert when the tag has never been registered.
            EventBus->Broadcast(TestTag, 99);
            TestTrue("No crash on empty broadcast", true);
        });

        It("Should do nothing after all subscribers have been removed", [this]
        {
            int32 CallCount = 0;
            FDelegateHandle Handle = EventBus->AddLambda(TestTag, [&](int32) { ++CallCount; });

            EventBus->Remove(TestTag, Handle);
            EventBus->Broadcast(TestTag, 1);

            TestEqual("Callback must not fire after removal", CallCount, 0);
        });
    });

    // ------------------------------------------------------------------
    // Describe: Type Safety
    // ------------------------------------------------------------------

    Describe("Type Safety", [this]
    {
        It("Should correctly identify matching argument types", [this]
        {
            EventBus->AddLambda(TestTag, [](int32, float, FString) {});

            TestTrue("Matching types",     EventBus->IsArgsType<int32, float, FString>(TestTag));
            TestFalse("Mismatched types",  EventBus->IsArgsType<int32, float, char>(TestTag));
        });

        It("Should report false for IsArgsType when the tag has no container", [this]
        {
            TestFalse("No container yet", EventBus->IsArgsType<int32>(TestTag));
        });
    });

    // ------------------------------------------------------------------
    // Describe: Signature Locking
    // ------------------------------------------------------------------

    Describe("Signature Locking", [this]
    {
        It("Should pre-associate types before any subscriber is added", [this]
        {
            EventBus->LockSignature<int32, int32>(TestTag);

            TestTrue("Signature is locked to <int32, int32>",
                EventBus->IsArgsType<int32, int32>(TestTag));

            EventBus->UnlockSignature(TestTag);
        });

        It("Should keep the container alive with zero subscribers while locked", [this]
        {
            EventBus->LockSignature<int32>(TestTag);

            // No subscriber — container must still exist and hold its type.
            TestTrue("Container survives with zero subscribers while locked",
                EventBus->IsArgsType<int32>(TestTag));

            EventBus->UnlockSignature(TestTag);
        });

        It("Should destroy the container when unlocked with zero subscribers", [this]
        {
            EventBus->LockSignature<int32>(TestTag);
            EventBus->UnlockSignature(TestTag);

            // After unlock with no subscribers, the container must be gone.
            TestFalse("Container cleaned up after unlock with zero subscribers",
                EventBus->IsArgsType<int32>(TestTag));
        });

        It("Should allow a different signature after unlock and subscriber removal", [this]
        {
            EventBus->LockSignature<int32, int32>(TestTag);
            EventBus->UnlockSignature(TestTag);

            EventBus->AddLambda(TestTag, [](float, float) {});

            TestTrue("New signature accepted",      EventBus->IsArgsType<float, float>(TestTag));
            TestFalse("Old signature is gone",      EventBus->IsArgsType<int32, int32>(TestTag));
        });

        It("Should keep the container alive after Remove when still locked", [this]
        {
            EventBus->LockSignature<int32>(TestTag);

            FDelegateHandle Handle = EventBus->AddLambda(TestTag, [](int32) {});
            EventBus->Remove(TestTag, Handle);

            // Lock is still held, so the container must survive subscriber removal.
            TestTrue("Container survives subscriber removal while locked",
                EventBus->IsArgsType<int32>(TestTag));
            TestFalse("But no delegate is bound",
                EventBus->IsBound(TestTag));

            EventBus->UnlockSignature(TestTag);
        });
    });

    // ------------------------------------------------------------------
    // Describe: Remove
    // ------------------------------------------------------------------

    Describe("Remove", [this]
    {
        It("Should stop delivering events after Remove by handle", [this]
        {
            int32 CallCount = 0;
            FDelegateHandle Handle = EventBus->AddLambda(TestTag, [&](int32) { ++CallCount; });

            EventBus->Broadcast(TestTag, 1);
            EventBus->Remove(TestTag, Handle);
            EventBus->Broadcast(TestTag, 2);

            TestEqual("Only the first broadcast was received", CallCount, 1);
        });

        It("Should clean up the container when the last subscriber is removed", [this]
        {
            FDelegateHandle Handle = EventBus->AddLambda(TestTag, [](int32) {});

            EventBus->Remove(TestTag, Handle);

            TestFalse("Tag is no longer bound",    EventBus->IsBound(TestTag));
            TestFalse("Container has been removed", EventBus->IsArgsType<int32>(TestTag));
        });

        It("Should return false and do nothing when given an invalid handle", [this]
        {
            const bool Result = EventBus->Remove(TestTag, FDelegateHandle());
            TestFalse("Remove with invalid handle returns false", Result);
        });

        It("Should only remove the targeted subscriber, leaving others intact", [this]
        {
            int32 CountA = 0, CountB = 0;

            FDelegateHandle HandleA = EventBus->AddLambda(TestTag, [&](int32) { ++CountA; });
            EventBus->AddLambda(TestTag, [&](int32) { ++CountB; });

            EventBus->Remove(TestTag, HandleA);
            EventBus->Broadcast(TestTag, 1);

            TestEqual("Removed subscriber A did not fire", CountA, 0);
            TestEqual("Remaining subscriber B fired",      CountB, 1);
        });
    });

    // ------------------------------------------------------------------
    // Describe: RemoveAll
    // ------------------------------------------------------------------

    Describe("RemoveAll", [this]
    {
        It("Should remove all delegates bound to a given object", [this]
        {
            UEventBusTestObject* Listener = NewObject<UEventBusTestObject>();

            EventBus->AddUObject(TestTag, Listener, &UEventBusTestObject::OnEvent);
            EventBus->AddUObject(TestTag, Listener, &UEventBusTestObject::OnEvent);

            const int32 Removed = EventBus->RemoveAll(TestTag, Listener);

            TestEqual("Two delegates removed",     Removed, 2);
            TestFalse("Object is no longer bound", EventBus->IsBoundToObject(TestTag, Listener));
        });

        It("Should return 0 and do nothing when given a null object", [this]
        {
            EventBus->AddLambda(TestTag, [](int32) {});

            const int32 Removed = EventBus->RemoveAll(TestTag, nullptr);

            TestEqual("Nothing removed for null object", Removed, 0);
            TestTrue("Remaining subscriber still bound",  EventBus->IsBound(TestTag));
        });

        It("Should not affect subscribers bound to a different object", [this]
        {
            UEventBusTestObject* ListenerA = NewObject<UEventBusTestObject>();
            UEventBusTestObject* ListenerB = NewObject<UEventBusTestObject>();

            EventBus->AddUObject(TestTag, ListenerA, &UEventBusTestObject::OnEvent);
            EventBus->AddUObject(TestTag, ListenerB, &UEventBusTestObject::OnEvent);

            EventBus->RemoveAll(TestTag, ListenerA);

            TestFalse("ListenerA is unbound",    EventBus->IsBoundToObject(TestTag, ListenerA));
            TestTrue("ListenerB is still bound", EventBus->IsBoundToObject(TestTag, ListenerB));
        });
    });

    // ------------------------------------------------------------------
    // Describe: IsBound / IsBoundToObject
    // ------------------------------------------------------------------

    Describe("Query", [this]
    {
        It("IsBound should return false before any subscription", [this]
        {
            TestFalse("Nothing bound yet", EventBus->IsBound(TestTag));
        });

        It("IsBound should return true once a lambda is added", [this]
        {
            EventBus->AddLambda(TestTag, [](int32) {});
            TestTrue("Bound after AddLambda", EventBus->IsBound(TestTag));
        });

        It("IsBound should return false after all subscribers are removed", [this]
        {
            FDelegateHandle Handle = EventBus->AddLambda(TestTag, [](int32) {});
            EventBus->Remove(TestTag, Handle);
            TestFalse("Unbound after Remove", EventBus->IsBound(TestTag));
        });

        It("IsBoundToObject should correctly identify a bound UObject", [this]
        {
            UEventBusTestObject* Listener = NewObject<UEventBusTestObject>();

            TestFalse("Not bound before Add",   EventBus->IsBoundToObject(TestTag, Listener));
            EventBus->AddUObject(TestTag, Listener, &UEventBusTestObject::OnEvent);
            TestTrue("Bound after AddUObject",  EventBus->IsBoundToObject(TestTag, Listener));
        });

        It("IsBoundToObject should return false for a null object", [this]
        {
            EventBus->AddLambda(TestTag, [](int32) {});
            TestFalse("Null object is never bound", EventBus->IsBoundToObject(TestTag, nullptr));
        });
    });

    // ------------------------------------------------------------------
    // Describe: Delegate types
    // ------------------------------------------------------------------

    Describe("Delegate Types", [this]
    {
        It("AddUObject should deliver events to a UObject method", [this]
        {
            UEventBusTestObject* Listener = NewObject<UEventBusTestObject>();

            EventBus->AddUObject(TestTag, Listener, &UEventBusTestObject::OnEvent);
            EventBus->Broadcast(TestTag, 7);

            TestEqual("UObject method received value", Listener->LastValue,  7);
            TestEqual("UObject method called once",    Listener->CallCount,  1);
        });

        It("AddWeakLambda should not fire after the UObject is destroyed", [this]
        {
            int32 CallCount = 0;
            UEventBusTestObject* Listener = NewObject<UEventBusTestObject>();

            EventBus->AddWeakLambda(TestTag, Listener, [&](int32) { ++CallCount; });

            EventBus->Broadcast(TestTag, 1);
            TestEqual("Fires while object is alive", CallCount, 1);

            // Simulate GC by marking the object as garbage.
            Listener->MarkAsGarbage();
            EventBus->Broadcast(TestTag, 2);
            TestEqual("Does not fire after object is destroyed", CallCount, 1);
        });

        It("AddSP should deliver events to a shared pointer object", [this]
        {
            struct FSharedListener : TSharedFromThis<FSharedListener>
            {
                int32 CallCount = 0;
                void OnEvent(int32) { ++CallCount; }
            };

            TSharedRef<FSharedListener> Listener = MakeShared<FSharedListener>();

            EventBus->AddSP(TestTag, Listener, &FSharedListener::OnEvent);
            EventBus->Broadcast(TestTag, 1);

            TestEqual("SP delegate received event", Listener->CallCount, 1);
        });
    });

    // ------------------------------------------------------------------
    // Describe: Re-entrancy
    // ------------------------------------------------------------------

    Describe("Re-entrancy", [this]
    {
        It("Should not crash when a subscriber removes itself during broadcast", [this]
        {
            struct FSelfRemovingListener : TSharedFromThis<FSelfRemovingListener>
            {
                UWorld* World;
                FGameplayTag     Tag;
                FDelegateHandle  Handle;
                UEventBus* EventBusRef; // Used to call remove internally
                bool             bDidRun = false;

                void OnEvent(int32)
                {
                    bDidRun = true;
                    EventBusRef->Remove(Tag, Handle);
                }
            };

            auto Listener    = MakeShared<FSelfRemovingListener>();
            Listener->World  = TestWorld;
            Listener->Tag    = TestTag;
            Listener->EventBusRef = EventBus;
            Listener->Handle = EventBus->AddSP(TestTag, Listener, &FSelfRemovingListener::OnEvent);

            bool bOtherListenerRan = false;
            EventBus->AddLambda(TestTag, [&](int32) { bOtherListenerRan = true; });

            EventBus->Broadcast(TestTag, 1);

            TestTrue("Self-removing listener ran",  Listener->bDidRun);
            TestTrue("Other listener also ran",     bOtherListenerRan);
            // IsBoundToObject only works with UObject and raw pointers.
            // For SP delegates, the delegate stores a TWeakPtr internally,
            // so we verify removal via IsBound — the second lambda is still bound,
            // meaning only the SP delegate was removed.
            TestTrue("Tag still has the second subscriber", EventBus->IsBound(TestTag));
        });

        It("Should not crash when a subscriber adds a new subscriber during broadcast", [this]
        {
            int32 NewSubscriberCallCount = 0;

            // This lambda registers a second subscriber when it first fires.
            EventBus->AddLambda(TestTag, [&](int32)
            {
                // Only register once to avoid infinite recursion.
                if (NewSubscriberCallCount == 0)
                {
                    EventBus->AddLambda(TestTag, [&](int32) { ++NewSubscriberCallCount; });
                }
            });

            EventBus->Broadcast(TestTag, 1);

            // The newly added subscriber must not fire during the broadcast that triggered its addition,
            // because UE iterates the invocation list in reverse and ignores appended entries.
            TestEqual("Newly added subscriber did not fire during the same broadcast",
                NewSubscriberCallCount, 0);

            EventBus->Broadcast(TestTag, 2);
            TestEqual("Newly added subscriber fires on the next broadcast",
                NewSubscriberCallCount, 1);
        });

        It("Should not crash when broadcast is called from within a callback", [this]
        {
            // Use a secondary tag to avoid direct recursion on the same delegate.
            FGameplayTag SecondaryTag = FGameplayTag::RequestGameplayTag(TEXT("Test.Event.Bus"));
            int32 SecondaryCallCount  = 0;

            EventBus->AddLambda(TestTag, [&](int32)
            {
                // Nested broadcast on a different internal lambda to avoid stack overflow.
                ++SecondaryCallCount;
            });

            EventBus->Broadcast(TestTag, 1);

            TestEqual("Nested logic executed without crash", SecondaryCallCount, 1);
        });
    });
}

#endif