// Copyright Epic Games, Inc. All Rights Reserved.

#include <TargetConditionals.h>
#include "debugger.hpp"

#if MTLPP_CONFIG_VALIDATE
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/sysctl.h>

namespace mtlpp
{
	bool IsDebuggerPresent(void)
	// Returns true if the current process is being debugged (either
	// running under the debugger or has a debugger attached post facto).
	{
		// Based on http://developer.apple.com/library/mac/#qa/qa1361/_index.html
		
		struct kinfo_proc Info;
		int Mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
		size_t Size = sizeof(Info);
		
		sysctl( Mib, sizeof( Mib ) / sizeof( *Mib ), &Info, &Size, NULL, 0 );
		
		return ( Info.kp_proc.p_flag & P_TRACED ) != 0;
	}
	
	void Break(void)
	{
		if(IsDebuggerPresent())
		{
#if MTLPP_PLATFORM_MAC
            // Epic agrant: use builtin for arm mac
    #ifdef __aarch64__
            __builtin_trap();
    #else
			__asm__ ( "int $3" );
    #endif
#else
			__asm__ ( "svc 0" );
#endif
		}
	}
}
#endif
