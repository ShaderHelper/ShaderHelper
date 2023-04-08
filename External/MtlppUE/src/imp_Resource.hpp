// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Resource_hpp
#define imp_Resource_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<typename ObjC>
struct IMPTableResource : public IMPTableBase<ObjC>
{
	IMPTableResource()
	{
	}
	
	IMPTableResource(Class C)
	: IMPTableBase<ObjC>(C)
	, INTERPOSE_CONSTRUCTOR(Heap, C)
	, INTERPOSE_CONSTRUCTOR(StorageMode, C)
	, INTERPOSE_CONSTRUCTOR(CpuCacheMode, C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(SetPurgeableState, C)
	, INTERPOSE_CONSTRUCTOR(AllocatedSize, C)
	, INTERPOSE_CONSTRUCTOR(MakeAliasable, C)
	, INTERPOSE_CONSTRUCTOR(IsAliasable, C)
	{
	}
	
	template<typename InterposeClass>
	void RegisterInterpose(Class C)
	{
		IMPTableBase<ObjC>::template RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(SetPurgeableState, C);
		INTERPOSE_REGISTRATION(MakeAliasable, C);
	}
	
	INTERPOSE_SELECTOR(id<MTLResource>, heap, Heap, id<MTLHeap>);
	INTERPOSE_SELECTOR(id<MTLResource>, storageMode, StorageMode, MTLStorageMode);
	INTERPOSE_SELECTOR(id<MTLResource>, cpuCacheMode, CpuCacheMode, MTLCPUCacheMode);
	INTERPOSE_SELECTOR(id<MTLResource>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLResource>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLResource>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLResource>, setPurgeableState:, SetPurgeableState, MTLPurgeableState, MTLPurgeableState);
	INTERPOSE_SELECTOR(id<MTLResource>, allocatedSize, AllocatedSize, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLResource>, makeAliasable, MakeAliasable, void);
	INTERPOSE_SELECTOR(id<MTLResource>, isAliasable, IsAliasable, BOOL);
};

MTLPP_END

#endif /* imp_Resource_hpp */
