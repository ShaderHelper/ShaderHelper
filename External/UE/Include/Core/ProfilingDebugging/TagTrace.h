// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreTypes.h"
#include "Trace/Config.h"
#include "Trace/Detail/LogScope.h"

////////////////////////////////////////////////////////////////////////////////
// Fwd declare ELLMTag
enum class ELLMTag : uint8;
// Fwd declare LLM private tag data
namespace UE {
	namespace LLMPrivate {
		class FTagData;
	}
}

////////////////////////////////////////////////////////////////////////////////
CORE_API int32	MemoryTrace_AnnounceCustomTag(int32 Tag, int32 ParentTag, const TCHAR* Display);
CORE_API int32	MemoryTrace_AnnounceFNameTag(const class FName& TagName);
CORE_API int32	MemoryTrace_GetActiveTag();

////////////////////////////////////////////////////////////////////////////////
#if !defined(UE_MEMORY_TAGS_TRACE_ENABLED)
	#define UE_MEMORY_TAGS_TRACE_ENABLED 0
#endif

#if UE_MEMORY_TAGS_TRACE_ENABLED && UE_TRACE_ENABLED

////////////////////////////////////////////////////////////////////////////////

/**
  * Used to associate any allocation within this scope to a given tag.
  *
  * We need to be able to convert the three types of inputs to LLM scopes:
  * - ELLMTag, an uint8 with fixed categories. There are three sub ranges
	  Generic tags, platform and project tags.
  * - FName, free form string, for example a specific asset.
  * - TagData, an opaque pointer from LLM.
  *
  */
class FMemScope
{
public:
	CORE_API FMemScope(int32 InTag);
	CORE_API FMemScope(ELLMTag InTag);
	CORE_API FMemScope(const class FName& InName);
	CORE_API FMemScope(const UE::LLMPrivate::FTagData* TagData);
	CORE_API ~FMemScope();
private:
	void ActivateScope(int32 InTag);
	UE::Trace::Private::FScopedLogScope Inner;
	int32 PrevTag;
};

/**
 * Used order to keep the tag for memory that is being reallocated.
 */
class FMemScopePtr
{
public:
	CORE_API FMemScopePtr(uint64 InPtr);
	CORE_API ~FMemScopePtr();
private:
	UE::Trace::Private::FScopedLogScope Inner;
};

////////////////////////////////////////////////////////////////////////////////
constexpr int32 TRACE_TAG = 257;

////////////////////////////////////////////////////////////////////////////////
#define UE_MEMSCOPE(InTag)				FMemScope PREPROCESSOR_JOIN(MemScope,__LINE__)(InTag);
#define UE_MEMSCOPE_PTR(InPtr)			FMemScopePtr PREPROCESSOR_JOIN(MemPtrScope,__LINE__)((uint64)InPtr);

#else // UE_MEMORY_TAGS_TRACE_ENABLED

////////////////////////////////////////////////////////////////////////////////
#define UE_MEMSCOPE(...)
#define UE_MEMSCOPE_PTR(...)

#endif // UE_MEMORY_TAGS_TRACE_ENABLED

