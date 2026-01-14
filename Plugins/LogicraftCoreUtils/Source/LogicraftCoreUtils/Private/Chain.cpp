// Copyright (c) Logicraft Interactive. All Rights Reserved.

#include "Chain.h"
#include "LogCategory.h"

void Chain::Private::ChainLog(const FString& Msg)
{
	UE_LOG(LogLCU, Warning, TEXT("%s"), *Msg);
}
