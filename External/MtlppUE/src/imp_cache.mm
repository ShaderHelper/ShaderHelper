// Copyright Epic Games, Inc. All Rights Reserved.

#include "imp_cache.hpp"
#include "device.hpp"
#include "argument_encoder.hpp"
#include "command_queue.hpp"
#include "buffer.hpp"
#include "depth_stencil.hpp"
#include "library.hpp"
#include "texture.hpp"
#include "sampler.hpp"
#include "render_pipeline.hpp"
#include "compute_pipeline.hpp"
#include "heap.hpp"
#include "fence.hpp"
#include "command_buffer.hpp"
#include "render_command_encoder.hpp"
#include "blit_command_encoder.hpp"
#include "compute_command_encoder.hpp"
#include "parallel_render_command_encoder.hpp"
#include "capture_scope.hpp"

namespace UE
{
	template<typename T>
	ITable<T, void>* GetITable(ITableCache* self, T Obj, ITable<T, void>** Cache)
	{
		assert(Obj);
		assert(Cache);
		if (*Cache == nullptr)
		{
			*Cache = UE::CreateIMPTable(Obj);
			if (!(*Cache)->TableCache)
			{
				(*Cache)->TableCache = self;
			}
		}
		assert(*Cache);
		return *Cache;
	}
	
	ITable<id<MTLArgumentEncoder>, void>* ITableCache::GetArgumentEncoder(id<MTLArgumentEncoder> Obj) { return GetITable(this, Obj, &ArgumentEncoder); }
	ITable<id<MTLBlitCommandEncoder>, void>* ITableCache::GetBlitCommandEncoder(id<MTLBlitCommandEncoder> Obj)  { return GetITable(this, Obj, &BlitCommandEncoder); }
	ITable<id<MTLBuffer>, void>* ITableCache::GetBuffer(id<MTLBuffer> Obj)  { return GetITable(this, Obj, &Buffer); }
	ITable<id<MTLCommandQueue>, void>* ITableCache::GetCommandQueue(id<MTLCommandQueue> Obj)  { return GetITable(this, Obj, &CommandQueue); }
	ITable<id<MTLCommandBuffer>, void>* ITableCache::GetCommandBuffer(id<MTLCommandBuffer> Obj)  { return GetITable(this, Obj, &CommandBuffer); }
	ITable<id<MTLComputeCommandEncoder>, void>* ITableCache::GetComputeCommandEncoder(id<MTLComputeCommandEncoder> Obj)  { return GetITable(this, Obj, &ComputeCommandEncoder); }
	ITable<id<MTLComputePipelineState>, void>* ITableCache::GetComputePipelineState(id<MTLComputePipelineState> Obj)  { return GetITable(this, Obj, &ComputePipelineState); }
	ITable<id<MTLDepthStencilState>, void>* ITableCache::GetDepthStencilState(id<MTLDepthStencilState> Obj)  { return GetITable(this, Obj, &DepthStencilState); }
	ITable<id<MTLDevice>, void>* ITableCache::GetDevice(id<MTLDevice> Obj) { return GetITable(this, Obj, &Device); }
	ITable<id<MTLFence>, void>* ITableCache::GetFence(id<MTLFence> Obj)  { return GetITable(this, Obj, &Fence); }
	ITable<id<MTLFunction>, void>* ITableCache::GetFunction(id<MTLFunction> Obj)  { return GetITable(this, Obj, &Function); }
	ITable<id<MTLHeap>, void>* ITableCache::GetHeap(id<MTLHeap> Obj)  { return GetITable(this, Obj, &Heap); }
	ITable<id<MTLLibrary>, void>* ITableCache::GetLibrary(id<MTLLibrary> Obj)  { return GetITable(this, Obj, &Library); }
	ITable<id<MTLParallelRenderCommandEncoder>, void>* ITableCache::GetParallelRenderCommandEncoder(id<MTLParallelRenderCommandEncoder> Obj)  { return GetITable(this, Obj, &ParallelRenderCommandEncoder); }
	ITable<id<MTLRenderCommandEncoder>, void>* ITableCache::GetRenderCommandEncoder(id<MTLRenderCommandEncoder> Obj)  { return GetITable(this, Obj, &RenderCommandEncoder); }
	ITable<id<MTLRenderPipelineState>, void>* ITableCache::GetRenderPipelineState(id<MTLRenderPipelineState> Obj)  { return GetITable(this, Obj, &RenderPipelineState); }
	ITable<id<MTLSamplerState>, void>* ITableCache::GetSamplerState(id<MTLSamplerState> Obj)  { return GetITable(this, Obj, &SamplerState); }
	ITable<id<MTLTexture>, void>* ITableCache::GetTexture(id<MTLTexture> Obj)  { return GetITable(this, Obj, &Texture); }
	ITable<id<MTLCaptureScope>, void>* ITableCache::GetCaptureScope(id<MTLCaptureScope> Obj)  { return GetITable(this, Obj, &CaptureScope); }
	
	ITable<id<MTLArgumentEncoder>, void>* ITableCacheRef::GetArgumentEncoder(id<MTLArgumentEncoder> Obj) { return (Obj && TableCache) ? TableCache->GetArgumentEncoder(Obj) : nullptr; }
	ITable<id<MTLBlitCommandEncoder>, void>* ITableCacheRef::GetBlitCommandEncoder(id<MTLBlitCommandEncoder> Obj)  { return (Obj && TableCache) ? TableCache->GetBlitCommandEncoder(Obj) : nullptr; }
	ITable<id<MTLBuffer>, void>* ITableCacheRef::GetBuffer(id<MTLBuffer> Obj)  { return (Obj && TableCache) ? TableCache->GetBuffer(Obj) : nullptr; }
	ITable<id<MTLCommandQueue>, void>* ITableCacheRef::GetCommandQueue(id<MTLCommandQueue> Obj)  { return (Obj && TableCache) ? TableCache->GetCommandQueue(Obj) : nullptr; }
	ITable<id<MTLCommandBuffer>, void>* ITableCacheRef::GetCommandBuffer(id<MTLCommandBuffer> Obj)  { return (Obj && TableCache) ? TableCache->GetCommandBuffer(Obj) : nullptr; }
	ITable<id<MTLComputeCommandEncoder>, void>* ITableCacheRef::GetComputeCommandEncoder(id<MTLComputeCommandEncoder> Obj)  { return (Obj && TableCache) ? TableCache->GetComputeCommandEncoder(Obj) : nullptr; }
	ITable<id<MTLComputePipelineState>, void>* ITableCacheRef::GetComputePipelineState(id<MTLComputePipelineState> Obj) { return (Obj && TableCache) ? TableCache->GetComputePipelineState(Obj) : nullptr; }
	ITable<id<MTLDepthStencilState>, void>* ITableCacheRef::GetDepthStencilState(id<MTLDepthStencilState> Obj)  { return (Obj && TableCache) ? TableCache->GetDepthStencilState(Obj) : nullptr; }
	ITable<id<MTLDevice>, void>* ITableCacheRef::GetDevice(id<MTLDevice> Obj) { return (Obj && TableCache) ? TableCache->GetDevice(Obj) : nullptr; }
	ITable<id<MTLFence>, void>* ITableCacheRef::GetFence(id<MTLFence> Obj)  { return (Obj && TableCache) ? TableCache->GetFence(Obj) : nullptr; }
	ITable<id<MTLFunction>, void>* ITableCacheRef::GetFunction(id<MTLFunction> Obj)  { return (Obj && TableCache) ? TableCache->GetFunction(Obj) : nullptr; }
	ITable<id<MTLHeap>, void>* ITableCacheRef::GetHeap(id<MTLHeap> Obj)  { return (Obj && TableCache) ? TableCache->GetHeap(Obj) : nullptr; }
	ITable<id<MTLLibrary>, void>* ITableCacheRef::GetLibrary(id<MTLLibrary> Obj)  { return (Obj && TableCache) ? TableCache->GetLibrary(Obj) : nullptr; }
	ITable<id<MTLParallelRenderCommandEncoder>, void>* ITableCacheRef::GetParallelRenderCommandEncoder(id<MTLParallelRenderCommandEncoder> Obj) { return (Obj && TableCache) ? TableCache->GetParallelRenderCommandEncoder(Obj) : nullptr; }
	ITable<id<MTLRenderCommandEncoder>, void>* ITableCacheRef::GetRenderCommandEncoder(id<MTLRenderCommandEncoder> Obj)  { return (Obj && TableCache) ? TableCache->GetRenderCommandEncoder(Obj) : nullptr; }
	ITable<id<MTLRenderPipelineState>, void>* ITableCacheRef::GetRenderPipelineState(id<MTLRenderPipelineState> Obj)  { return (Obj && TableCache) ? TableCache->GetRenderPipelineState(Obj) : nullptr; }
	ITable<id<MTLSamplerState>, void>* ITableCacheRef::GetSamplerState(id<MTLSamplerState> Obj)  { return (Obj && TableCache) ? TableCache->GetSamplerState(Obj) : nullptr; }
	ITable<id<MTLTexture>, void>* ITableCacheRef::GetTexture(id<MTLTexture> Obj)  { return (Obj && TableCache) ? TableCache->GetTexture(Obj) : nullptr; }
	ITable<id<MTLCaptureScope>, void>* ITableCacheRef::GetCaptureScope(id<MTLCaptureScope> Obj)  { return (Obj && TableCache) ? TableCache->GetCaptureScope(Obj) : nullptr; }
}
