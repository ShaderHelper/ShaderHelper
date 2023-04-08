// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Heap_hpp
#define imp_Heap_hpp

#include "imp_Object.hpp"

MTLPP_BEGIN

template<>
struct MTLPP_EXPORT IMPTable<MTLHeapDescriptor*, void> : public IMPTableBase<MTLHeapDescriptor*>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<MTLHeapDescriptor*>(C)
	, INTERPOSE_CONSTRUCTOR(StorageMode, C)
	, INTERPOSE_CONSTRUCTOR(CpuCacheMode, C)
	, INTERPOSE_CONSTRUCTOR(Size, C)
	, INTERPOSE_CONSTRUCTOR(setStorageMode, C)
	, INTERPOSE_CONSTRUCTOR(setCpuCacheMode, C)
	, INTERPOSE_CONSTRUCTOR(setSize, C)
	{
	}
	
	INTERPOSE_SELECTOR(MTLHeapDescriptor*, storageMode, StorageMode, MTLStorageMode);
	INTERPOSE_SELECTOR(MTLHeapDescriptor*, cpuCacheMode, CpuCacheMode, MTLCPUCacheMode);
	INTERPOSE_SELECTOR(MTLHeapDescriptor*, size, Size, NSUInteger);
	INTERPOSE_SELECTOR(MTLHeapDescriptor*, setStorageMode:, setStorageMode, void, MTLStorageMode);
	INTERPOSE_SELECTOR(MTLHeapDescriptor*, setCpuCacheMode:, setCpuCacheMode, void, MTLCPUCacheMode);
	INTERPOSE_SELECTOR(MTLHeapDescriptor*, setSize:, setSize, void, NSUInteger);
};

template<>
struct MTLPP_EXPORT IMPTable<id<MTLHeap>, void> : public IMPTableBase<id<MTLHeap>>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<id<MTLHeap>>(C)
	, INTERPOSE_CONSTRUCTOR(Device, C)
	, INTERPOSE_CONSTRUCTOR(Label, C)
	, INTERPOSE_CONSTRUCTOR(SetLabel, C)
	, INTERPOSE_CONSTRUCTOR(StorageMode, C)
	, INTERPOSE_CONSTRUCTOR(CpuCacheMode, C)
	, INTERPOSE_CONSTRUCTOR(SetPurgeableState, C)
	, INTERPOSE_CONSTRUCTOR(Size, C)
	, INTERPOSE_CONSTRUCTOR(UsedSize, C)
	, INTERPOSE_CONSTRUCTOR(CurrentAllocatedSize, C)
	, INTERPOSE_CONSTRUCTOR(MaxAvailableSizeWithAlignment, C)
	, INTERPOSE_CONSTRUCTOR(NewBufferWithLength, C)
	, INTERPOSE_CONSTRUCTOR(NewTextureWithDescriptor, C)
	{
	}
	
	INTERPOSE_SELECTOR(id<MTLHeap>, device, Device, id<MTLDevice>);
	INTERPOSE_SELECTOR(id<MTLHeap>, label, Label, NSString*);
	INTERPOSE_SELECTOR(id<MTLHeap>, setLabel:, SetLabel, void, NSString*);
	INTERPOSE_SELECTOR(id<MTLHeap>, storageMode, StorageMode, MTLStorageMode);
	INTERPOSE_SELECTOR(id<MTLHeap>, cpuCacheMode, CpuCacheMode, MTLCPUCacheMode);
	INTERPOSE_SELECTOR(id<MTLHeap>, setPurgeableState:, SetPurgeableState, MTLPurgeableState, MTLPurgeableState);
	INTERPOSE_SELECTOR(id<MTLHeap>, size, Size, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLHeap>, usedSize, UsedSize, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLHeap>, currentAllocatedSize, CurrentAllocatedSize, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLHeap>, maxAvailableSizeWithAlignment:, MaxAvailableSizeWithAlignment, NSUInteger, NSUInteger);
	INTERPOSE_SELECTOR(id<MTLHeap>, newBufferWithLength:options:, NewBufferWithLength, id<MTLBuffer>, NSUInteger, MTLResourceOptions);
	INTERPOSE_SELECTOR(id<MTLHeap>, newTextureWithDescriptor:, NewTextureWithDescriptor, id<MTLTexture>, MTLTextureDescriptor*);

};

template<typename InterposeClass>
struct MTLPP_EXPORT IMPTable<id<MTLHeap>, InterposeClass> : public IMPTable<id<MTLHeap>, void>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTable<id<MTLHeap>, void>(C)
	{
		RegisterInterpose(C);
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<id<MTLHeap>>::RegisterInterpose<InterposeClass>(C);
		
		INTERPOSE_REGISTRATION(SetLabel, C);
		INTERPOSE_REGISTRATION(SetPurgeableState, C);
		INTERPOSE_REGISTRATION(MaxAvailableSizeWithAlignment, C);
		INTERPOSE_REGISTRATION(NewBufferWithLength, C);
		INTERPOSE_REGISTRATION(NewTextureWithDescriptor, C);
	}
};

MTLPP_END

#endif /* imp_Heap_hpp */
