// Copyright Epic Games, Inc. All Rights Reserved.

#include <Foundation/Foundation.h>

#include "imp_SelectorCache.hpp"

MTLPP_BEGIN

void RegisterSwizzleSelector(Class ObjcClass, SEL Select, SEL& Output)
{
	if (Output == nullptr)
	{
		static NSLock* Lock = [[NSLock alloc] init];
		
		[Lock lock];
		char const* Name = sel_getName(Select);
		size_t len = strlen(Name) + 3 + 1;
		char* tempBuffer = (char*)alloca(len);
		for (unsigned i = 0; Output == nullptr && i < 10; i++)
		{
			snprintf(tempBuffer, len, "_%d_%s", i, Name);
			SEL newSel = sel_registerName(tempBuffer);
			if (class_respondsToSelector(ObjcClass, newSel) == false)
			{
				Output = newSel;
			}
		}
		[Lock unlock];
	}
	assert(Output);
}

MTLPP_END
