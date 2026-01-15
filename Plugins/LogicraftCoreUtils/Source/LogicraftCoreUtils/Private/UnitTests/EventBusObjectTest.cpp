// Copyright (c) Logicraft Interactive. All Rights Reserved.


#include "EventBusObjectTest.h"

#include "LogCategory.h"

void UEventBusObjectTest::EventCallback()
{
	UE_LOG(LogEventBus, Warning, TEXT("UEventBusObjectTest: Logging from EventCallback"))
}

void UEventBusObjectTest::EventCallback_Int(int Arg)
{
	UE_LOG(LogEventBus, Warning, TEXT("UEventBusObjectTest: Logging from EventCallback_Int [Arg : %d]"), Arg)
}

void UEventBusObjectTest::EventCallback_Float(float Arg)
{
	UE_LOG(LogEventBus, Warning, TEXT("UEventBusObjectTest: Logging from EventCallback_Float [Arg : %.4f]"), Arg)
}
