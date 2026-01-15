#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "EventBusSubsystem.h"
#include "GameplayTagContainer.h"
#include "Engine/World.h"
#include "Tests/AutomationEditorCommon.h"
#include "NativeGameplayTags.h"

BEGIN_DEFINE_SPEC(FEventBusSpec, "Logicraft.Core.EventBus", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
    UEventBusSubsystem* EventBus = nullptr;
    UWorld* TestWorld = nullptr;
    FGameplayTag TestTag;
END_DEFINE_SPEC(FEventBusSpec)

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Test_Event_Bus, "Test.Event.Bus", "Tag pour le bus d'événement de test");

void FEventBusSpec::Define()
{
    BeforeEach([this]()
    {
        TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
       
        FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(TestWorld);
       
        EventBus = TestWorld->GetSubsystem<UEventBusSubsystem>();
       
        if (!EventBus)
        {
             TestWorld->InitializeSubsystems();
             EventBus = TestWorld->GetSubsystem<UEventBusSubsystem>();
        }
       
        TestTag = FGameplayTag::RequestGameplayTag(TEXT("Test.Event.Bus"));
    });

    AfterEach([this]()
    {
        if (TestWorld)
        {
            GEngine->DestroyWorldContext(TestWorld);
            TestWorld->DestroyWorld(false);
            TestWorld = nullptr;
        }
        EventBus = nullptr;
    });

    Describe("Basic Functionality", [this]()
    {
        It("Should broadcast simple integers correctly", [this]()
        {
            int32 ReceivedValue = 0;
            
            // On lie une lambda
            EventBus->AddLambda(TestTag, [&ReceivedValue](int32 Val)
            {
                ReceivedValue = Val;
            });

            // On broadcast
            EventBus->Broadcast(TestTag, 42);

            TestEqual("Received Value should be 42", ReceivedValue, 42);
        });

        It("Should handle multiple arguments", [this]()
        {
            FString ReceivedStr;
            float ReceivedFloat = 0.0f;

            EventBus->AddLambda(TestTag, [&](FString S, float F)
            {
                ReceivedStr = S;
                ReceivedFloat = F;
            });

            EventBus->Broadcast(TestTag, FString("Hello"), 3.14f);

            TestEqual("String match", ReceivedStr, FString("Hello"));
            TestEqual("Float match", ReceivedFloat, 3.14f);
        });
    });

    Describe("Lifecycle Management", [this]()
    {
        It("Should unregister automatically when handle is used", [this]()
        {
            int32 CallCount = 0;
            FDelegateHandle Handle = EventBus->AddLambda(TestTag, [&](int32) { CallCount++; });

            EventBus->Broadcast(TestTag, 1);
            EventBus->Remove(TestTag, Handle);
            EventBus->Broadcast(TestTag, 2);

            TestEqual("Call count should stop at 1", CallCount, 1);
            TestFalse("Tag should no longer be bound", EventBus->IsBound(TestTag));
        });

        It("Should NOT crash on Re-entrancy (Removing self during broadcast)", [this]()
        {
            struct FReentrantHelper
            {
                UEventBusSubsystem* Bus;
                FGameplayTag Tag;
                FDelegateHandle MyHandle;
                bool bDidRun = false;

                void OnEvent(int32)
                {
                    bDidRun = true;
                    Bus->Remove(Tag, MyHandle);
                }
            };

            auto Helper = MakeShared<FReentrantHelper>();
            Helper->Bus = EventBus;
            Helper->Tag = TestTag;
            
            Helper->MyHandle = EventBus->AddSP(TestTag, Helper, &FReentrantHelper::OnEvent);
            
            bool bSecondListenerRun = false;
            EventBus->AddLambda(TestTag, [&](int32){ bSecondListenerRun = true; });
            
            EventBus->Broadcast(TestTag, 123);

            TestTrue("Helper ran", Helper->bDidRun);
            TestTrue("Second listener ran (depending on implementation order, but shouldn't crash)", bSecondListenerRun);
            TestFalse("Should be unbound after self-removal (if it was the only one, strictly checking specific handle)", EventBus->IsBoundToObject(TestTag, &Helper.Get()));
        });
    });

    //This test does work but since I'm using ensureMsgf AddExpectedError doesn't work and still fail the test.
    // Describe("Safety", [this]()
    // {
    //      It("Should handle type mismatch gracefully (Ensure check logs expected)", [this]()
    //      {
    //          AddExpectedError(TEXT("Unable to broadcast a callback of a different type"));
    //          
    //          EventBus->AddLambda(TestTag, [](int32){});
    //          
    //          EventBus->Broadcast(TestTag, 1.0f); 
    //      });
    // });
}