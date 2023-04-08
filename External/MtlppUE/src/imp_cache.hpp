// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "declare.hpp"
#include "imp_SelectorCache.hpp"
#include "imp_Object.hpp"

MTLPP_BEGIN

namespace UE
{
	template<typename ObjC, typename Interpose>
	class MTLPP_EXPORT imp_cache
	{
		imp_cache() {}
		~imp_cache()
		{
			for(auto entry : Impls)
			{
				delete entry.second;
			}
		}
		
	public:
		typedef ITable<ObjC, Interpose> table;
		
		MTLPP_EXPORT static table* Register(ObjC Object)
		{
			static imp_cache* Self = new imp_cache();
			table* impTable = nullptr;
			if(Object)
			{
				Class c = object_getClass(Object);
				Self->Lock.lock();
				auto it = Self->Impls.find(c);
				if (it != Self->Impls.end())
				{
					impTable = it->second;
				}
				else
				{
					impTable = new table(c);
					Self->Impls.emplace(c, impTable);
				}
				Self->Lock.unlock();
			}
			return impTable;
		}
		
	private:
		std::mutex Lock;
		std::unordered_map<Class, table*> Impls;
	};
	
	template<typename T>
	static inline ITable<T, void>* CreateIMPTable(const T handle)
	{
		if (handle)
		{
			return imp_cache<T, void>::Register(handle);
		}
		else
		{
			return nullptr;
		}
	}
	
	template<typename Interpose>
	struct ITable<IOSurfaceRef, Interpose>
	{
		ITable()
		{
		}
		
		ITable(Class C)
		{
		}
		
		void RegisterInterpose(Class C) {}
		
		void Retain(IOSurfaceRef Surface) { CFRetain(Surface); }
		void Release(IOSurfaceRef Surface) { CFRelease(Surface); }
	};
	
	template<>
	inline ITable<IOSurfaceRef, void>* CreateIMPTable(const IOSurfaceRef handle)
	{
		static ITable<IOSurfaceRef, void> Table;
		return &Table;
	}
	
	template<>
	inline ITable<NSError*, void>* CreateIMPTable(NSError* handle)
	{
		static ITable<NSError*, void> Table(objc_getRequiredClass("NSError"));
		return &Table;
	}
	
	class MTLPP_EXPORT ITableCache
	{
	public:
		ITableCache()
		: Device(nullptr)
		, ArgumentEncoder(nullptr)
		, CommandQueue(nullptr)
		, Buffer(nullptr)
		, DepthStencilState(nullptr)
		, Function(nullptr)
		, Library(nullptr)
		, Texture(nullptr)
		, SamplerState(nullptr)
		, RenderPipelineState(nullptr)
		, ComputePipelineState(nullptr)
		, Heap(nullptr)
		, Fence(nullptr)
		, CommandBuffer(nullptr)
		, RenderCommandEncoder(nullptr)
		, BlitCommandEncoder(nullptr)
		, ComputeCommandEncoder(nullptr)
		, ParallelRenderCommandEncoder(nullptr)
		, CaptureScope(nullptr)
		{
		}
		
		ITable<id<MTLDevice>, void>* GetDevice(id<MTLDevice> Obj);
		ITable<id<MTLArgumentEncoder>, void>* GetArgumentEncoder(id<MTLArgumentEncoder> Obj);
		ITable<id<MTLCommandQueue>, void>* GetCommandQueue(id<MTLCommandQueue> Obj);
		ITable<id<MTLBuffer>, void>* GetBuffer(id<MTLBuffer> Obj);
		ITable<id<MTLDepthStencilState>, void>* GetDepthStencilState(id<MTLDepthStencilState> Obj);
		ITable<id<MTLFunction>, void>* GetFunction(id<MTLFunction> Obj);
		ITable<id<MTLLibrary>, void>* GetLibrary(id<MTLLibrary> Obj);
		ITable<id<MTLTexture>, void>* GetTexture(id<MTLTexture> Obj);
		ITable<id<MTLSamplerState>, void>* GetSamplerState(id<MTLSamplerState> Obj);
		ITable<id<MTLRenderPipelineState>, void>* GetRenderPipelineState(id<MTLRenderPipelineState> Obj);
		ITable<id<MTLComputePipelineState>, void>* GetComputePipelineState(id<MTLComputePipelineState> Obj);
		ITable<id<MTLHeap>, void>* GetHeap(id<MTLHeap> Obj);
		ITable<id<MTLFence>, void>* GetFence(id<MTLFence> Obj);
		ITable<id<MTLCommandBuffer>, void>* GetCommandBuffer(id<MTLCommandBuffer> Obj);
		ITable<id<MTLRenderCommandEncoder>, void>* GetRenderCommandEncoder(id<MTLRenderCommandEncoder> Obj);
		ITable<id<MTLBlitCommandEncoder>, void>* GetBlitCommandEncoder(id<MTLBlitCommandEncoder> Obj);
		ITable<id<MTLComputeCommandEncoder>, void>* GetComputeCommandEncoder(id<MTLComputeCommandEncoder> Obj);
		ITable<id<MTLParallelRenderCommandEncoder>, void>* GetParallelRenderCommandEncoder(id<MTLParallelRenderCommandEncoder> Obj);
		ITable<id<MTLCaptureScope>, void>* GetCaptureScope(id<MTLCaptureScope> Obj);
		
	private:
		ITable<id<MTLDevice>, void>* Device;
		ITable<id<MTLArgumentEncoder>, void>* ArgumentEncoder;
		ITable<id<MTLCommandQueue>, void>* CommandQueue;
		ITable<id<MTLBuffer>, void>* Buffer;
		ITable<id<MTLDepthStencilState>, void>* DepthStencilState;
		ITable<id<MTLFunction>, void>* Function;
		ITable<id<MTLLibrary>, void>* Library;
		ITable<id<MTLTexture>, void>* Texture;
		ITable<id<MTLSamplerState>, void>* SamplerState;
		ITable<id<MTLRenderPipelineState>, void>* RenderPipelineState;
		ITable<id<MTLComputePipelineState>, void>* ComputePipelineState;
		ITable<id<MTLHeap>, void>* Heap;
		ITable<id<MTLFence>, void>* Fence;
		ITable<id<MTLCommandBuffer>, void>* CommandBuffer;
		ITable<id<MTLRenderCommandEncoder>, void>* RenderCommandEncoder;
		ITable<id<MTLBlitCommandEncoder>, void>* BlitCommandEncoder;
		ITable<id<MTLComputeCommandEncoder>, void>* ComputeCommandEncoder;
		ITable<id<MTLParallelRenderCommandEncoder>, void>* ParallelRenderCommandEncoder;
		ITable<id<MTLCaptureScope>, void>* CaptureScope;
	};
	
	struct MTLPP_EXPORT ITableCacheRef
	{
		ITableCacheRef()
		: TableCache(nullptr)
		{
		}
		
		ITableCacheRef(ITableCache* _TableCache)
		: TableCache(_TableCache)
		{
		}
		
		ITable<id<MTLDevice>, void>* GetDevice(id<MTLDevice> Obj);
		ITable<id<MTLArgumentEncoder>, void>* GetArgumentEncoder(id<MTLArgumentEncoder> Obj);
		ITable<id<MTLCommandQueue>, void>* GetCommandQueue(id<MTLCommandQueue> Obj);
		ITable<id<MTLBuffer>, void>* GetBuffer(id<MTLBuffer> Obj);
		ITable<id<MTLDepthStencilState>, void>* GetDepthStencilState(id<MTLDepthStencilState> Obj);
		ITable<id<MTLFunction>, void>* GetFunction(id<MTLFunction> Obj);
		ITable<id<MTLLibrary>, void>* GetLibrary(id<MTLLibrary> Obj);
		ITable<id<MTLTexture>, void>* GetTexture(id<MTLTexture> Obj);
		ITable<id<MTLSamplerState>, void>* GetSamplerState(id<MTLSamplerState> Obj);
		ITable<id<MTLRenderPipelineState>, void>* GetRenderPipelineState(id<MTLRenderPipelineState> Obj);
		ITable<id<MTLComputePipelineState>, void>* GetComputePipelineState(id<MTLComputePipelineState> Obj);
		ITable<id<MTLHeap>, void>* GetHeap(id<MTLHeap> Obj);
		ITable<id<MTLFence>, void>* GetFence(id<MTLFence> Obj);
		ITable<id<MTLCommandBuffer>, void>* GetCommandBuffer(id<MTLCommandBuffer> Obj);
		ITable<id<MTLRenderCommandEncoder>, void>* GetRenderCommandEncoder(id<MTLRenderCommandEncoder> Obj);
		ITable<id<MTLBlitCommandEncoder>, void>* GetBlitCommandEncoder(id<MTLBlitCommandEncoder> Obj);
		ITable<id<MTLComputeCommandEncoder>, void>* GetComputeCommandEncoder(id<MTLComputeCommandEncoder> Obj);
		ITable<id<MTLParallelRenderCommandEncoder>, void>* GetParallelRenderCommandEncoder(id<MTLParallelRenderCommandEncoder> Obj);
		ITable<id<MTLCaptureScope>, void>* GetCaptureScope(id<MTLCaptureScope> Obj);
		
		ITableCache* TableCache;
	};
}

MTLPP_END

