// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformAffinity.h"

/**
 * Helper functions for processes that fork in order to share memory pages.
 *
 * About multithreading:
 * When a process gets forked, any existing threads will not exist on the new forked process.
 * To solve this we use forkable threads that are notified when the fork occurs and will automatically convert themselves into real runnable threads.
 * On the master process, these forkable threads will be fake threads that are executed on the main thread and will block the critical path.
 *
 * Currently the game code is responsible for calling Fork on itself than calling FForkProcessHelper::OnForkingOccured to transform the forkable threads.
 * Ideally the fork point is done right after the game has loaded all the assets it wants to share so it can maximize the shared memory pool.
 * From the fork point any memory page that gets written into by a forked process will be transferred into a unique page for this process.
 * 
 */
class CORE_API FForkProcessHelper 
{
public:

	/**
	 * Returns true if the server process was launched with the intention to fork.
	 * This could be a process on a fork-supported platform that will launch real child processes. (-WaitAndFork is set)
	 * Or it could be a process that will simulate forking by tranforming itself into a child process via fake forking (-FakeForking is set)
	 */
	static bool IsForkRequested();

	/**
	 * Are we a forked process that supports multithreading
	 * This only becomes true after its safe to be multithread.
	 * Since a process can be forked mid-tick, there is a period of time where IsForkedChildProcess is true but IsForkedMultithreadInstance will be false
	 */
	static bool IsForkedMultithreadInstance();

	/**
	 * Is this a process that was forked
	 */
	static bool IsForkedChildProcess();

	/**
	 * Sets the forked child process flag 
	 */
	static void SetIsForkedChildProcess();

	/**
	 * Event triggered when a fork occurred on the child process and its safe to create real threads
	 */
	static void OnForkingOccured();

	/**
	 * Tells if we allow multithreading on forked processes.
	 * Default is set to false but can be configured to always be true via DEFAULT_MULTITHREAD_FORKED_PROCESSES
	 * Enabled via -PostForkThreading
	 * Disabled via -DisablePostForkThreading
	 */
	static bool SupportsMultithreadingPostFork();

	/**
	 * Creates a thread according to the environment it's in:
	 *	In environments with SupportsMultithreading: create a real thread that will tick the runnable object itself
	 *	In environments without multithreading: create a fake thread that is ticked by the main thread.
	 *  In environments without multithreading but that allows multithreading post-fork: 
	 *		If called on the original master process: will create a forkable thread that is ticked in the main thread pre-fork but becomes a real thread post-fork
	 *      If called on a forked child process: will create a real thread immediately
	 */
	static FRunnableThread* CreateForkableThread(
		class FRunnable* InRunnable,
		const TCHAR* InThreadName,
		uint32 InStackSize = 0,
		EThreadPriority InThreadPri = TPri_Normal,
		uint64 InThreadAffinityMask = FPlatformAffinity::GetNoAffinityMask(),
		EThreadCreateFlags InCreateFlags = EThreadCreateFlags::None
	);
};



