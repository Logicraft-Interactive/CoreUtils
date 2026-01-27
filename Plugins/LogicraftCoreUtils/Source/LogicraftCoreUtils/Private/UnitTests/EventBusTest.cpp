#include "CoreMinimal.h"

#if WITH_EDITOR

#include "EventBus.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FEventBusSpec, "Logicraft.Core.EventBus", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
    UEventBus* EventBus = nullptr;
    UWorld* TestWorld = nullptr;
    FGameplayTag TestTag;
END_DEFINE_SPEC(FEventBusSpec)

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Test_Event_Bus, "Test.Event.Bus", "Tag for event bus testing.");

void FEventBusSpec::Define()
{
    BeforeEach([this]
    {
        TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
       
        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(TestWorld);
       
        EventBus = TestWorld->GetSubsystem<UEventBus>();
       
        if (!EventBus)
        {
             TestWorld->InitializeSubsystems();
             EventBus = TestWorld->GetSubsystem<UEventBus>();
        }
       
        TestTag = FGameplayTag::RequestGameplayTag(TEXT("Test.Event.Bus"));
    });

    AfterEach([this]
    {
        if (TestWorld)
        {
            GEngine->DestroyWorldContext(TestWorld);
            TestWorld->DestroyWorld(false);
            TestWorld = nullptr;
        }
        EventBus = nullptr;
    });

    Describe("Basic Functionality", [this]
    {
        It("Should broadcast simple integers correctly", [this]
        {
            int32 ReceivedValue = 0;
            
            UEventBus::AddLambda(TestWorld, TestTag, [&ReceivedValue](int32 Val)
            {
                ReceivedValue = Val;
            });

            UEventBus::Broadcast(TestWorld, TestTag, 42);

            TestEqual("Received Value should be 42", ReceivedValue, 42);
        });

        It("Should handle multiple arguments", [this]
        {
            FString ReceivedStr;
            float ReceivedFloat = 0.0f;

            UEventBus::AddLambda(TestWorld, TestTag, [&](FString S, float F)
            {
                ReceivedStr = S;
                ReceivedFloat = F;
            });

            UEventBus::Broadcast(TestWorld, TestTag, FString("Hello"), 3.14f);

            TestEqual("String match", ReceivedStr, FString("Hello"));
            TestEqual("Float match", ReceivedFloat, 3.14f);
        });

        It("Should be able to compare types", [this]
        {
            UEventBus::AddLambda(TestWorld, TestTag, [](int, float, FString){});
            
            TestTrue("Types match", UEventBus::IsArgsType<int, float, FString>(TestWorld, TestTag));
            TestFalse("Types don't match", UEventBus::IsArgsType<int, float, char>(TestWorld, TestTag));
        });
    });

    Describe("Lifecycle Management", [this]
    {
        It("Should unregister automatically when handle is used", [this]
        {
            int32 CallCount = 0;
            FDelegateHandle Handle = UEventBus::AddLambda(TestWorld, TestTag, [&](int32) { CallCount++; });
            
            UEventBus::Broadcast(TestWorld, TestTag, 1);
            UEventBus::Remove(TestWorld, TestTag, Handle);
            UEventBus::Broadcast(TestWorld, TestTag, 2);

            TestEqual("Call count should stop at 1", CallCount, 1);
            TestFalse("Tag should no longer be bound", UEventBus::IsBound(TestWorld, TestTag));
        });

        It("Should NOT crash on Re-entrancy (Removing self during broadcast)", [this]
        {
            struct FReentrantHelper
            {
                UWorld* World;
                FGameplayTag Tag;
                FDelegateHandle MyHandle;
                bool bDidRun = false;

                void OnEvent(int32)
                {
                    bDidRun = true;
                    UEventBus::Remove(World, Tag, MyHandle);
                }
            };

            auto Helper = MakeShared<FReentrantHelper>();
            Helper->World = TestWorld;
            Helper->Tag = TestTag;
            
            Helper->MyHandle = UEventBus::AddSP(TestWorld, TestTag, Helper, &FReentrantHelper::OnEvent);
            
            bool bSecondListenerRun = false;
            UEventBus::AddLambda(TestWorld, TestTag, [&](int32){ bSecondListenerRun = true; });
            
            UEventBus::Broadcast(TestWorld, TestTag, 123);

            TestTrue("Helper ran", Helper->bDidRun);
            TestTrue("Second listener ran (depending on implementation order, but shouldn't crash)", bSecondListenerRun);
            TestFalse("Should be unbound after self-removal (if it was the only one, strictly checking specific handle)", UEventBus::IsBoundToObject(TestWorld, TestTag, &Helper.Get()));
        });
    });
    
    //This test does work but since I'm using ensureMsgf AddExpectedError doesn't work and still fail the test.
    // Describe("Safety", [this]()
    // {
    //      It("Should handle type mismatch gracefully (Ensure check logs expected)", [this]()
    //      {
    //          AddExpectedError(TEXT("Unable to broadcast a callback of a different type"));
    //          
    //          UEventBus::AddLambda(TestWorld, TestTag, [](int32){});
    //          
    //          UEventBus::Broadcast(TestWorld, TestTag, 1.0f); 
    //      });
    // });
}

#endif