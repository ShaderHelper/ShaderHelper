// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef debugger_hpp
#define debugger_hpp

#include "declare.hpp"

#if MTLPP_CONFIG_VALIDATE
namespace mtlpp
{
	bool IsDebuggerPresent(void);
	void Break(void);
}
#endif

#endif /* debugger_hpp */
