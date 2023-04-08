/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLComputeCommandEncoder.h>
#include <Metal/MTLHeap.h>
#include <Metal/MTLResource.h>
#include <Metal/MTLSampler.h>
#include <Metal/MTLStageInputOutputDescriptor.h>
#include "compute_command_encoder.hpp"
#include "buffer.hpp"
#include "compute_pipeline.hpp"
#include "sampler.hpp"
#include "heap.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    void ComputeCommandEncoder::SetComputePipelineState(const ComputePipelineState& state)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setcomputepipelinestate(m_ptr, state.GetPtr());
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setComputePipelineState:(id<MTLComputePipelineState>)state.GetPtr()];
#endif
    }

    void ComputeCommandEncoder::SetBytes(const void* data, NSUInteger length, NSUInteger index)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setbyteslengthatindex(m_ptr, data, length, index);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setBytes:data length:length atIndex:index];
#endif
    }

    void ComputeCommandEncoder::SetBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
    {
        Validate();
        
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setbufferoffsetatindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset(), index);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setBuffer:(id<MTLBuffer>)buffer.GetPtr() offset:offset + buffer.GetOffset() atIndex:index];
#endif
    }

    void ComputeCommandEncoder::SetBufferOffset(NSUInteger offset, NSUInteger index)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 8_3)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetBufferOffsetatindex(m_ptr, offset, index);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setBufferOffset:offset atIndex:index];
#endif
#endif
    }

    void ComputeCommandEncoder::SetBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range)
    {
        Validate();
		id<MTLBuffer>* array = (id<MTLBuffer>*)alloca(range.Length * sizeof(id<MTLBuffer>));
		NSUInteger* theOffsets = (NSUInteger*)alloca(range.Length * sizeof(NSUInteger));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLBuffer>)buffers[i].GetPtr();
			theOffsets[i] = offsets[i] + (buffers[i] ? buffers[i].GetOffset() : 0);
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setbuffersoffsetswithrange(m_ptr, (id<MTLBuffer>*)array, (NSUInteger const*)theOffsets, NSMakeRange(range.Location, range.Length));

#else
        [(id<MTLComputeCommandEncoder>)m_ptr setBuffers:array
                                                         offsets:theOffsets
                                                       withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void ComputeCommandEncoder::SetTexture(const Texture& texture, NSUInteger index)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Settextureatindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setTexture:(id<MTLTexture>)texture.GetPtr() atIndex:index];
#endif
    }

    void ComputeCommandEncoder::SetTextures(const Texture* textures, const ns::Range& range)
    {
        Validate();
		id<MTLTexture>* array = (id<MTLTexture>*)alloca(range.Length * sizeof(id<MTLTexture>));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLTexture>)textures[i].GetPtr();
		}
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Settextureswithrange(m_ptr, (id<MTLTexture>*)array, NSMakeRange(range.Location, range.Length));

#else
        [(id<MTLComputeCommandEncoder>)m_ptr setTextures:array
                                                        withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void ComputeCommandEncoder::SetSamplerState(const SamplerState& sampler, NSUInteger index)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setsamplerstateatindex(m_ptr, sampler.GetPtr(), index);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setSamplerState:(id<MTLSamplerState>)sampler.GetPtr() atIndex:index];
#endif
    }

    void ComputeCommandEncoder::SetSamplerStates(const SamplerState::Type* samplers, const ns::Range& range)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setsamplerstateswithrange(m_ptr, samplers, NSMakeRange(range.Location, range.Length));

#else
        [(id<MTLComputeCommandEncoder>)m_ptr setSamplerStates:samplers
                                                             withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void ComputeCommandEncoder::SetSamplerState(const SamplerState& sampler, float lodMinClamp, float lodMaxClamp, NSUInteger index)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setsamplerstatelodminclamplodmaxclampatindex(m_ptr, (id<MTLSamplerState>)sampler.GetPtr(), lodMinClamp, lodMaxClamp, index);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setSamplerState:(id<MTLSamplerState>)sampler.GetPtr()
                                                          lodMinClamp:lodMinClamp
                                                          lodMaxClamp:lodMaxClamp
                                                              atIndex:index];
#endif
    }

    void ComputeCommandEncoder::SetSamplerStates(const SamplerState::Type* samplers, const float* lodMinClamps, const float* lodMaxClamps, const ns::Range& range)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setsamplerstateslodminclampslodmaxclampswithrange(m_ptr, samplers, lodMinClamps, lodMaxClamps, NSMakeRange(range.Location, range.Length));

#else
        [(id<MTLComputeCommandEncoder>)m_ptr setSamplerStates:samplers
                                                          lodMinClamps:lodMinClamps
                                                          lodMaxClamps:lodMaxClamps
                                                             withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void ComputeCommandEncoder::SetThreadgroupMemory(NSUInteger length, NSUInteger index)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setthreadgroupmemorylengthatindex(m_ptr, length, index);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setThreadgroupMemoryLength:length atIndex:index];
#endif
    }
	
	void ComputeCommandEncoder::SetImageblock(NSUInteger width, NSUInteger height)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetImageblockWidthHeight(m_ptr, width, height);
#else
		[m_ptr setImageblockWidth:width height:height];
#endif
#endif
	}

    void ComputeCommandEncoder::SetStageInRegion(const Region& region)
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setstageinregion(m_ptr, region);
#else
        [(id<MTLComputeCommandEncoder>)m_ptr setStageInRegion:MTLRegionMake3D(region.origin.x, region.origin.y, region.origin.z, region.size.width, region.size.height, region.size.depth)];
#endif
#endif
    }

    void ComputeCommandEncoder::DispatchThreadgroups(const Size& threadgroupsPerGrid, const Size& threadsPerThreadgroup)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Dispatchthreadgroupsthreadsperthreadgroup(m_ptr, threadgroupsPerGrid, threadsPerThreadgroup);
#else
        MTLSize mtlThreadgroupsPerGrid = MTLSizeMake(threadgroupsPerGrid.width, threadgroupsPerGrid.height, threadgroupsPerGrid.depth);
        MTLSize mtlThreadsPerThreadgroup = MTLSizeMake(threadsPerThreadgroup.width, threadsPerThreadgroup.height, threadsPerThreadgroup.depth);
        [(id<MTLComputeCommandEncoder>)m_ptr dispatchThreadgroups:mtlThreadgroupsPerGrid threadsPerThreadgroup:mtlThreadsPerThreadgroup];
#endif
    }

    void ComputeCommandEncoder::DispatchThreadgroupsWithIndirectBuffer(const Buffer& indirectBuffer, NSUInteger indirectBufferOffset, const Size& threadsPerThreadgroup)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Dispatchthreadgroupswithindirectbufferindirectbufferoffsetthreadsperthreadgroup(m_ptr, (id<MTLBuffer>)indirectBuffer.GetPtr(), indirectBufferOffset + indirectBuffer.GetOffset(), threadsPerThreadgroup);
#else
        MTLSize mtlThreadsPerThreadgroup = MTLSizeMake(threadsPerThreadgroup.width, threadsPerThreadgroup.height, threadsPerThreadgroup.depth);
        [(id<MTLComputeCommandEncoder>)m_ptr dispatchThreadgroupsWithIndirectBuffer:(id<MTLBuffer>)indirectBuffer.GetPtr()
                                                                        indirectBufferOffset:indirectBufferOffset + indirectBuffer.GetOffset()
                                                                       threadsPerThreadgroup:mtlThreadsPerThreadgroup];
#endif
    }
	
	void ComputeCommandEncoder::DispatchThreads(const Size& threadsPerGrid, const Size& threadsPerThreadgroup)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0) && !MTLPP_PLATFORM_TVOS
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Dispatchthreadsthreadsperthreadgroup(m_ptr, threadsPerGrid, threadsPerThreadgroup);
#else
		MTLSize mtlThreadsPerGrid = MTLSizeMake(threadsPerGrid.width, threadsPerGrid.height, threadsPerGrid.depth);
		MTLSize mtlThreadsPerThreadgroup = MTLSizeMake(threadsPerThreadgroup.width, threadsPerThreadgroup.height, threadsPerThreadgroup.depth);
		[(id<MTLComputeCommandEncoder>)m_ptr dispatchThreads:mtlThreadsPerGrid threadsPerThreadgroup:mtlThreadsPerThreadgroup];
#endif
#endif
	}

    void ComputeCommandEncoder::UpdateFence(const Fence& fence)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
			m_table->Updatefence(m_ptr, fence.GetPtr());
#else
		if (@available(macOS 10.13, iOS 10.0, *))
		[(id<MTLComputeCommandEncoder>)m_ptr updateFence:(id<MTLFence>)fence.GetPtr()];
#endif
#endif
    }

    void ComputeCommandEncoder::WaitForFence(const Fence& fence)
    {
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Waitforfence(m_ptr, fence.GetPtr());
#else
		if (@available(macOS 10.13, iOS 10.0, *))
			[(id<MTLComputeCommandEncoder>)m_ptr waitForFence:(id<MTLFence>)fence.GetPtr()];
#endif
#endif
    }
	
	void ComputeCommandEncoder::UseResource(const Resource& resource, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Useresourceusage(m_ptr, resource.GetPtr(), MTLResourceUsage(usage));
#else
		[(id<MTLComputeCommandEncoder>)m_ptr useResource:(id<MTLResource>)resource.GetPtr() usage:(MTLResourceUsage)usage];
#endif
#endif
	}
	
	void ComputeCommandEncoder::UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
        id<MTLResource>* array = (id<MTLResource>*)alloca(count * sizeof(id<MTLBuffer>));
        for (NSUInteger i = 0; i < count; i++)
        {
            array[i] = (id<MTLResource>)resource[i].GetPtr();
        }
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Useresourcescountusage(m_ptr, array, count, MTLResourceUsage(usage));
#else
		[(id<MTLComputeCommandEncoder>)m_ptr useResources:array count:count usage:(MTLResourceUsage)usage];
#endif
#endif
	}
	
	void ComputeCommandEncoder::UseHeap(const Heap& heap)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Useheap(m_ptr, heap.GetPtr());
#else
		[(id<MTLComputeCommandEncoder>)m_ptr useHeap:(id<MTLHeap>)heap.GetPtr()];
#endif
#endif
	}
	
	void ComputeCommandEncoder::UseHeaps(const Heap::Type* heap, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Useheapscount(m_ptr, heap, count);
#else
		[(id<MTLComputeCommandEncoder>)m_ptr useHeaps:heap count:count];
#endif
#endif
	}
	
#if MTLPP_CONFIG_VALIDATE
	void ValidatedComputeCommandEncoder::UseResource(const Resource& resource, ResourceUsage usage)
	{
		Validator.UseResource(resource.GetPtr(), usage);
		
		if (class_conformsToProtocol(object_getClass(resource.GetPtr()), @protocol(MTLBuffer)) && ((const Buffer&)resource).GetParentBuffer())
		{
			Validator.UseResource(((const Buffer&)resource).GetPtr(), ns::Range(((const Buffer&)resource).GetOffset(), ((const Buffer&)resource).GetLength()), usage);
		}
		
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		if (@available(macOS 10.13, iOS 11.0, *))
		{
			ComputeCommandEncoder::UseResource(resource, usage);
		}
#endif
	}
	
	void ValidatedComputeCommandEncoder::UseResources(const Resource* resource, NSUInteger count, ResourceUsage usage)
	{
		for (NSUInteger i = 0; i < count; ++i)
		{
			Validator.UseResource(resource[i].GetPtr(), usage);
			if (class_conformsToProtocol(object_getClass(resource[i].GetPtr()), @protocol(MTLBuffer)) && ((const Buffer&)resource[i]).GetParentBuffer())
			{
				Validator.UseResource(((const Buffer&)resource[i]).GetPtr(), ns::Range(((const Buffer&)resource[i]).GetOffset(), ((const Buffer&)resource[i]).GetLength()), usage);
			}
		}
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		if (@available(macOS 10.13, iOS 11.0, *))
		{
			ComputeCommandEncoder::UseResources(resource, count, usage);
		}
#endif
	}
#endif
}

MTLPP_END
