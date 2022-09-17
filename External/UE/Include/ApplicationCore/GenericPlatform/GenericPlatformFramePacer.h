// Copyright Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformFramePacer.h: Generic platform frame pacer classes
==============================================================================================*/


#pragma once

#include "CoreTypes.h"
#include "CoreFwd.h"

/**
 * Generic implementation for most platforms
 **/
struct APPLICATIONCORE_API FGenericPlatformRHIFramePacer
{
	/**
	 * The pace we are running at (30 = 30fps, 0 = unpaced)
	 * The generic implementation returns a result based on rhi.SyncInterval and FPlatformMisc::GetMaxRefreshRate().
	 */
	static inline int32 GetFramePace() 
	{
		return GetFramePaceFromSyncInterval();
	}

	/**
	 * Sets the pace we would like to running at (30 = 30fps, 0 = unpaced).
	 * The generic implementation sets the value for rhi.SyncInterval according to FPlatformMisc::GetMaxRefreshRate().
	 *
	 * @return the pace we will run at.
	 */
	static inline int32 SetFramePace(int32 FramePace)
	{
		return SetFramePaceToSyncInterval(FramePace);
	}

	/**
	 * Returns whether the hardware is able to frame pace at the specified frame rate
	 */
	static bool SupportsFramePace(int32 QueryFramePace);

protected:
	/**
	 * The generic implementation returns a result based on rhi.SyncInterval and FPlatformMisc::GetMaxRefreshRate().
	 */
	static int32 GetFramePaceFromSyncInterval();

	/**
	 * The generic implementation sets rhi.SyncInterval based on FPlatformMisc::GetMaxRefreshRate().
	 *
	 * @return the pace we will run at.
	 */
	static int32 SetFramePaceToSyncInterval(int32 FramePace);
};
