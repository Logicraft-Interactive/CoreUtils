// Copyright (c) 2026 Logicraft Interactive. All Rights Reserved.

#include "PoolSystem/Poolable.h"

// Add default functionality here for any IPoolable functions that are not pure virtual.
int IPoolable::Internal_GetIndex() const
{
	return Index;
}


void IPoolable::Internal_SetIndex(const int NewIndex)
{
	Index = NewIndex;
}
