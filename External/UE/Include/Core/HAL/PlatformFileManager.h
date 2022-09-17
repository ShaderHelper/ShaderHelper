// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "CoreFwd.h"
#include "Templates/Atomic.h"

#ifndef USE_ATOMIC_PLATFORM_FILE
	#define USE_ATOMIC_PLATFORM_FILE (WITH_EDITOR)
#endif

/**
* Platform File chain manager.
**/
class CORE_API FPlatformFileManager
{
#if USE_ATOMIC_PLATFORM_FILE
	/** Currently used platform file. */
	TAtomic<class IPlatformFile*> TopmostPlatformFile;
#else
	/** Currently used platform file. */
	class IPlatformFile* TopmostPlatformFile;
#endif

public:

	/** Constructor. */
	FPlatformFileManager( );

	/**
	 * Gets the currently used platform file.
	 *
	 * @return Reference to the currently used platform file.
	 */
	IPlatformFile& GetPlatformFile( );

	/**
	 * Sets the current platform file.
	 *
	 * @param NewTopmostPlatformFile Platform file to be used.
	 */
	void SetPlatformFile( IPlatformFile& NewTopmostPlatformFile );

	/**
	 * Finds a platform file in the chain of active platform files.
	 *
	 * @param Name of the platform file.
	 * @return Pointer to the active platform file or nullptr if the platform file was not found.
	 */
	IPlatformFile* FindPlatformFile( const TCHAR* Name );

	/**
	 * Creates a new platform file instance.
	 *
	 * @param Name of the platform file to create.
	 * @return Platform file instance of the platform file type was found, nullptr otherwise.
	 */
	IPlatformFile* GetPlatformFile( const TCHAR* Name );

	/**
	 * calls Tick on the platform files in the TopmostPlatformFile chain
	 */
	void TickActivePlatformFile();

	/**
	* Performs additional initialization when the new async IO is enabled.
	*/
	void InitializeNewAsyncIO();

	/**
	 * Gets FPlatformFileManager Singleton.
	 */
	static FPlatformFileManager& Get( );

	/**
	* Removes the specified file wrapper from the platform file wrapper chain.
	*
	* THIS IS EXTREMELY DANGEROUS AFTER THE ENGINE HAS BEEN INITIALIZED AS WE MAY BE MODIFYING
	* THE WRAPPER CHAIN WHILE THINGS ARE BEING LOADED
	*
	* @param Name of the platform file to create.
	* @return Platform file instance of the platform file type was found, nullptr otherwise.
	*/
	void RemovePlatformFile(IPlatformFile* PlatformFileToRemove);

};
