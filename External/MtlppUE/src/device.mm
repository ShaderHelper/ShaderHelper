/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLComputePipeline.h>
#include <Metal/MTLDevice.h>
#include <Metal/MTLRenderPass.h>
#include <Metal/MTLRenderPipeline.h>
#include <Metal/MTLSampler.h>
#include <Metal/MTLStageInputOutputDescriptor.h>
#include <Foundation/NSError.h>
#include "device.hpp"
#include "buffer.hpp"
#include "command_queue.hpp"
#include "compute_pipeline.hpp"
#include "depth_stencil.hpp"
#include "render_pipeline.hpp"
#include "sampler.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "argument_encoder.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	ArgumentDescriptor::ArgumentDescriptor()
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
	: ns::Object<MTLArgumentDescriptor*>([MTLArgumentDescriptor new], ns::Ownership::Assign)
#endif
	{
	}
	
	ArgumentDescriptor::ArgumentDescriptor(ns::Ownership const retain)
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
	: ns::Object<MTLArgumentDescriptor*>(nil, retain)
#endif
	{
	}
	
	DataType ArgumentDescriptor::GetDataType() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (DataType)[(MTLArgumentDescriptor*)m_ptr dataType];
#else
		return 0;
#endif
	}
	
	NSUInteger ArgumentDescriptor::GetIndex() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLArgumentDescriptor*)m_ptr index];
#else
		return 0;
#endif
	}
	
	NSUInteger ArgumentDescriptor::GetArrayLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLArgumentDescriptor*)m_ptr arrayLength];
#else
		return 0;
#endif
	}
	
	ArgumentAccess ArgumentDescriptor::GetAccess() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (ArgumentAccess)[(MTLArgumentDescriptor*)m_ptr access];
#else
		return 0;
#endif
	}
	
	TextureType ArgumentDescriptor::GetTextureType() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (TextureType)[(MTLArgumentDescriptor*)m_ptr textureType];
#else
		return 0;
#endif
	}
	
	NSUInteger ArgumentDescriptor::GetConstantBlockAlignment() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return [(MTLArgumentDescriptor*)m_ptr constantBlockAlignment];
#else
		return 0;
#endif
	}
	
	void ArgumentDescriptor::SetDataType(DataType Type)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_ptr.dataType = (MTLDataType)Type;
#endif
	}
	
	void ArgumentDescriptor::SetIndex(NSUInteger Index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_ptr.index = Index;
#endif
	}
	
	void ArgumentDescriptor::SetArrayLength(NSUInteger Len)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_ptr.arrayLength = Len;
#endif
	}
	
	void ArgumentDescriptor::SetAccess(ArgumentAccess Access)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_ptr.access = (MTLArgumentAccess)Access;
#endif
	}
	
	void ArgumentDescriptor::SetTextureType(TextureType Type)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_ptr.textureType = (MTLTextureType)Type;
#endif
	}
	
	void ArgumentDescriptor::SetConstantBlockAlignment(NSUInteger Align)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		m_ptr.constantBlockAlignment = Align;
#endif
	}
	
    CompileOptions::CompileOptions() :
        ns::Object<MTLCompileOptions*>([[MTLCompileOptions alloc] init], ns::Ownership::Assign)
    {
    }

	ns::AutoReleased<ns::String> Device::GetWasAddedNotification()
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return ns::AutoReleased<ns::String>(MTLDeviceWasAddedNotification);
#else
		return ns::AutoReleased<ns::String>();
#endif
	}
	
	ns::AutoReleased<ns::String> Device::GetRemovalRequestedNotification()
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return ns::AutoReleased<ns::String>(MTLDeviceRemovalRequestedNotification);
#else
		return ns::AutoReleased<ns::String>();
#endif
	}
	
	ns::AutoReleased<ns::String> Device::GetWasRemovedNotification()
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return ns::AutoReleased<ns::String>(MTLDeviceWasRemovedNotification);
#else
		return ns::AutoReleased<ns::String>();
#endif
	}
	
	ns::Array<Device> Device::CopyAllDevicesWithObserver(ns::Object<id <NSObject>>& observer, DeviceHandler handler)
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		return ns::Array<Device>(MTLCopyAllDevicesWithObserver(observer.GetInnerPtr(), ^(id<MTLDevice>  _Nonnull device, MTLDeviceNotificationName  _Nonnull notifyName)
			{
			handler(Device(device), ns::String(notifyName));
		}), ns::Ownership::Assign);
#else
		return ns::Array<Device>();
#endif
	}
	
	void Device::RemoveDeviceObserver(ns::Object<id <NSObject>> observer)
	{
#if MTLPP_IS_AVAILABLE_MAC(10_13)
		if (observer)
		{
			MTLRemoveDeviceObserver((id<NSObject>)observer.GetPtr());
		}
#endif
	}
	
    Device Device::CreateSystemDefaultDevice()
    {
        return Device(MTLCreateSystemDefaultDevice(), ns::Ownership::Assign);
    }

    ns::Array<Device> Device::CopyAllDevices()
    {
#if MTLPP_IS_AVAILABLE_MAC(10_11)
        return ns::Array<Device>(MTLCopyAllDevices(), ns::Ownership::Assign);
#else
		return [NSArray arrayWithObject:[MTLCreateSystemDefaultDevice() autorelease]];
#endif
    }

    ns::AutoReleased<ns::String> Device::GetName() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->name(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLDevice>)m_ptr name]);
#endif
    }

    Size Device::GetMaxThreadsPerThreadgroup() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		MTLPPSize MTLPPSize =  m_table->maxThreadsPerThreadgroup(m_ptr);
        return Size(NSUInteger(MTLPPSize.width), NSUInteger(MTLPPSize.height), NSUInteger(MTLPPSize.depth));
#else
        MTLSize mtlSize = [(id<MTLDevice>)m_ptr maxThreadsPerThreadgroup];
        return Size(NSUInteger(mtlSize.width), NSUInteger(mtlSize.height), NSUInteger(mtlSize.depth));
#endif
#else
        return Size(0, 0, 0);
#endif
    }

    bool Device::IsLowPower() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->isLowPower(m_ptr);
#else
        return [(id<MTLDevice>)m_ptr isLowPower];
#endif
#else
        return false;
#endif
    }

    bool Device::IsHeadless() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->isHeadless(m_ptr);
#else
        return [(id<MTLDevice>)m_ptr isHeadless];
#endif
#else
        return false;
#endif
    }
	
	bool Device::IsRemovable() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_13)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->isRemovable(m_ptr);
#else
		return [(id<MTLDevice>)m_ptr isRemovable];
#endif
#else
		return false;
#endif
	}

    uint64_t Device::GetRecommendedMaxWorkingSetSize() const
    {
#if MTLPP_IS_AVAILABLE_MAC(10_12)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->recommendedMaxWorkingSetSize(m_ptr);
#else
		if(@available(macOS 10.12, *))
			return [(id<MTLDevice>)m_ptr recommendedMaxWorkingSetSize];
		else
			return 0;
#endif
#else
		return 0;
#endif
    }

    bool Device::IsDepth24Stencil8PixelFormatSupported() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->isDepth24Stencil8PixelFormatSupported(m_ptr);
#else
        return [(id<MTLDevice>)m_ptr isDepth24Stencil8PixelFormatSupported];
#endif
#else
        return true;
#endif
    }
	
	uint64_t Device::GetRegistryID() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->registryID(m_ptr);
#else
		return [(id<MTLDevice>)m_ptr registryID];
#endif
#else
		return 0;
#endif
	}
	
	ReadWriteTextureTier Device::GetReadWriteTextureSupport() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return (ReadWriteTextureTier)m_table->readWriteTextureSupport(m_ptr);
#else
		return (ReadWriteTextureTier)[(id<MTLDevice>)m_ptr readWriteTextureSupport];
#endif
#else
		return 0;
#endif
	}
	
	ArgumentBuffersTier Device::GetArgumentsBufferSupport() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return (ArgumentBuffersTier)m_table->argumentBuffersSupport(m_ptr);
#else
		return (ArgumentBuffersTier)[(id<MTLDevice>)m_ptr argumentBuffersSupport];
#endif
#else
		return 0;
#endif
	}
	
	bool Device::AreRasterOrderGroupsSupported() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->areRasterOrderGroupsSupported(m_ptr);
#else
		return [(id<MTLDevice>)m_ptr areRasterOrderGroupsSupported];
#endif
#else
		return false;
#endif
	}
	
	uint64_t Device::GetCurrentAllocatedSize() const
	{
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->currentAllocatedSize(m_ptr);
#else
		return [(id<MTLDevice>)m_ptr currentAllocatedSize];
#endif
#else
		return 0;
#endif
	}

    CommandQueue Device::NewCommandQueue()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return CommandQueue(m_table->NewCommandQueue(m_ptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return CommandQueue([(id<MTLDevice>)m_ptr newCommandQueue], nullptr, ns::Ownership::Assign);
#endif
    }

    CommandQueue Device::NewCommandQueue(NSUInteger maxCommandBufferCount)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return CommandQueue(m_table->NewCommandQueueWithMaxCommandBufferCount(m_ptr, maxCommandBufferCount), m_table->TableCache, ns::Ownership::Assign);
#else
        return CommandQueue([(id<MTLDevice>)m_ptr newCommandQueueWithMaxCommandBufferCount:maxCommandBufferCount], nullptr, ns::Ownership::Assign);
#endif
    }

    SizeAndAlign Device::HeapTextureSizeAndAlign(const TextureDescriptor& desc)
    {
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		MTLPPSizeAndAlign MTLPPSizeAndAlign = m_table->heapTextureSizeAndAlignWithDescriptor(m_ptr, desc.GetPtr());
		return SizeAndAlign{ NSUInteger(MTLPPSizeAndAlign.size), NSUInteger(MTLPPSizeAndAlign.align) };
#else
		if (@available(macOS 10.13, iOS 10.0, *))
		{
			MTLSizeAndAlign mtlSizeAndAlign = [(id<MTLDevice>)m_ptr heapTextureSizeAndAlignWithDescriptor:(MTLTextureDescriptor*)desc.GetPtr()];
			return SizeAndAlign{ NSUInteger(mtlSizeAndAlign.size), NSUInteger(mtlSizeAndAlign.align) };
		}
		return SizeAndAlign{0, 0};
#endif
#else
        return SizeAndAlign{0, 0};
#endif
    }

    SizeAndAlign Device::HeapBufferSizeAndAlign(NSUInteger length, ResourceOptions options)
    {
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		MTLPPSizeAndAlign MTLPPSizeAndAlign = m_table->heapBufferSizeAndAlignWithLengthoptions(m_ptr, length, MTLResourceOptions(options));
		return SizeAndAlign{ NSUInteger(MTLPPSizeAndAlign.size), NSUInteger(MTLPPSizeAndAlign.align) };
#else
		if (@available(macOS 10.13, iOS 10.0, *))
		{
			MTLSizeAndAlign mtlSizeAndAlign = [(id<MTLDevice>)m_ptr heapBufferSizeAndAlignWithLength:length options:MTLResourceOptions(options)];
			return SizeAndAlign{ NSUInteger(mtlSizeAndAlign.size), NSUInteger(mtlSizeAndAlign.align) };
		}
		return SizeAndAlign{0, 0};
#endif
#else
        return SizeAndAlign{0, 0};
#endif
    }

    Heap Device::NewHeap(const HeapDescriptor& descriptor)
    {
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return Heap(m_table->NewHeapWithDescriptor(m_ptr, descriptor.GetPtr()), m_table->TableCache, ns::Ownership::Assign);
#else
		if (@available(macOS 10.13, iOS 10.0, *))
		{
			return Heap([(id<MTLDevice>)m_ptr newHeapWithDescriptor:(MTLHeapDescriptor*)descriptor.GetPtr()], nullptr, ns::Ownership::Assign);
		}
		return nullptr;
#endif
#else
		return nullptr;
#endif
    }

    Buffer Device::NewBuffer(NSUInteger length, ResourceOptions options)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Buffer(m_table->NewBufferWithLength(m_ptr, length, MTLResourceOptions(options)), m_table->TableCache, ns::Ownership::Assign);
#else
        return Buffer([(id<MTLDevice>)m_ptr newBufferWithLength:length options:MTLResourceOptions(options)], nullptr, ns::Ownership::Assign);
#endif
    }

    Buffer Device::NewBuffer(const void* pointer, NSUInteger length, ResourceOptions options)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Buffer(m_table->NewBufferWithBytes(m_ptr, pointer, length, MTLResourceOptions(options)), m_table->TableCache, ns::Ownership::Assign);
#else
        return Buffer([(id<MTLDevice>)m_ptr newBufferWithBytes:pointer length:length options:MTLResourceOptions(options)], nullptr, ns::Ownership::Assign);
#endif
    }


    Buffer Device::NewBuffer(void* pointer, NSUInteger length, ResourceOptions options, BufferDeallocHandler deallocator)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Buffer(m_table->NewBufferWithBytesNoCopy(m_ptr, pointer, length, MTLResourceOptions(options), ^(void* pointer, NSUInteger length) { deallocator(pointer, NSUInteger(length)); }), m_table->TableCache, ns::Ownership::Assign);
#else
        return Buffer([(id<MTLDevice>)m_ptr newBufferWithBytesNoCopy:pointer
                                                                             length:length
                                                                            options:MTLResourceOptions(options)
                                                                        deallocator:^(void* pointer, NSUInteger length) { deallocator(pointer, uint32_t(length)); }], nullptr, ns::Ownership::Assign);
#endif
    }

    DepthStencilState Device::NewDepthStencilState(const DepthStencilDescriptor& descriptor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return DepthStencilState(m_table->NewDepthStencilStateWithDescriptor(m_ptr, descriptor.GetPtr()), m_table->TableCache, ns::Ownership::Assign);
#else
        return DepthStencilState([(id<MTLDevice>)m_ptr newDepthStencilStateWithDescriptor:(MTLDepthStencilDescriptor*)descriptor.GetPtr()], nullptr, ns::Ownership::Assign);
#endif
    }

    Texture Device::NewTexture(const TextureDescriptor& descriptor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Texture(m_table->NewTextureWithDescriptor(m_ptr, descriptor.GetPtr()), m_table->TableCache, ns::Ownership::Assign);
#else
        return Texture([(id<MTLDevice>)m_ptr newTextureWithDescriptor:(MTLTextureDescriptor*)descriptor.GetPtr()], nullptr, ns::Ownership::Assign);
#endif
    }
	
	Texture Device::NewTextureWithDescriptor(const TextureDescriptor& descriptor, ns::IOSurface& iosurface, NSUInteger plane)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_11)
#if MTLPP_CONFIG_IMP_CACHE
		return 	Texture(m_table->NewTextureWithDescriptorIOSurface(m_ptr, descriptor.GetPtr(), iosurface.GetPtr(), plane), m_table->TableCache, ns::Ownership::Assign);
#else
		return Texture([(id<MTLDevice>)m_ptr newTextureWithDescriptor:(MTLTextureDescriptor*)descriptor.GetPtr() iosurface:(IOSurfaceRef)iosurface.GetPtr() plane:plane], nullptr, ns::Ownership::Assign);
#endif
#else
		return Texture(nullptr, nullptr);
#endif
	}

    SamplerState Device::NewSamplerState(const SamplerDescriptor& descriptor)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return SamplerState(m_table->NewSamplerStateWithDescriptor(m_ptr, descriptor.GetPtr()), m_table->TableCache, ns::Ownership::Assign);
#else
        return SamplerState([(id<MTLDevice>)m_ptr newSamplerStateWithDescriptor:(MTLSamplerDescriptor*)descriptor.GetPtr()], nullptr, ns::Ownership::Assign);
#endif
    }

    Library Device::NewDefaultLibrary()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Library(m_table->NewDefaultLibrary(m_ptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return Library([(id<MTLDevice>)m_ptr newDefaultLibrary], nullptr, ns::Ownership::Assign);
#endif
    }

    Library Device::NewLibrary(const ns::String& filepath, ns::AutoReleasedError* error)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Library(m_table->NewLibraryWithFile(m_ptr, filepath.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return Library([(id<MTLDevice>)m_ptr newLibraryWithFile:(NSString*)filepath.GetPtr() error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
    }
	
	Library Device::NewLibrary(dispatch_data_t data, ns::AutoReleasedError* error)
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Library(m_table->NewLibraryWithData(m_ptr, data, error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
		return Library([(id<MTLDevice>)m_ptr newLibraryWithData:data error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
	}
	
	Library Device::NewDefaultLibraryWithBundle(const ns::Bundle& bundle, ns::AutoReleasedError* error)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return Library(m_table->NewDefaultLibraryWithBundle(m_ptr, bundle.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return Library([(id<MTLDevice>)m_ptr newDefaultLibraryWithBundle:(NSBundle*)bundle.GetPtr() error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
#else
		return Library();
#endif
	}
	
	Library Device::NewLibrary(ns::String source, const CompileOptions& options, ns::AutoReleasedError* error)
	{
		Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Library(m_table->NewLibraryWithSourceOptionsError(m_ptr, source, options.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
		return Library([(id<MTLDevice>)m_ptr newLibraryWithSource:source options:options error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
	}
	
	Library Device::NewLibrary(ns::URL const& url, ns::AutoReleasedError* error)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
        return Library(m_table->NewLibraryWithURL(m_ptr, url.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return Library([(id<MTLDevice>)m_ptr newLibraryWithURL:(NSURL*)url.GetPtr() error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
#else
		return Library();
#endif
	}

    void Device::NewLibrary(ns::String source, const CompileOptions& options, LibraryHandler completionHandler)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->NewLibraryWithSourceOptionsCompletionHandler(m_ptr, source, options.GetPtr(), ^(id <MTLLibrary> library, NSError * error) {
			completionHandler(
							  Library(library, cache, ns::Ownership::Assign),
							  ns::AutoReleasedError(error));
		});
#else
        [(id<MTLDevice>)m_ptr newLibraryWithSource:source
                                                    options:(MTLCompileOptions*)options.GetPtr()
                                          completionHandler:^(id <MTLLibrary> library, NSError * error) {
                                                completionHandler(
													Library(library, nullptr, ns::Ownership::Assign),
                                                    ns::AutoReleasedError(error));
                                          }];
#endif
    }

    RenderPipelineState Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, ns::AutoReleasedError* error)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return RenderPipelineState(m_table->NewRenderPipelineStateWithDescriptorError(m_ptr, descriptor.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return RenderPipelineState([(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                                                          error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
    }

    RenderPipelineState Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, AutoReleasedRenderPipelineReflection* outReflection, ns::AutoReleasedError* error)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return RenderPipelineState(m_table->NewRenderPipelineStateWithDescriptorOptionsReflectionError(m_ptr, descriptor.GetPtr(), MTLPipelineOption(options), outReflection ? outReflection->GetInnerPtr() : nil, error ? (NSError**)error->GetInnerPtr() : nullptr ), m_table->TableCache, ns::Ownership::Assign);
#else
        return RenderPipelineState([(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                                                        options:MTLPipelineOption(options)
                                                                                     reflection:outReflection ? outReflection->GetInnerPtr() : nil
                                                                                          error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
    }

    void Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, RenderPipelineStateHandler completionHandler)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->NewRenderPipelineStateWithDescriptorCompletionHandler(m_ptr, descriptor.GetPtr(), ^(id <MTLRenderPipelineState> renderPipelineState, NSError * error) {
			completionHandler(
							  RenderPipelineState(renderPipelineState, cache, ns::Ownership::Assign),
							  ns::AutoReleasedError(error)
							  );
		});
#else
        [(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                          completionHandler:^(id <MTLRenderPipelineState> renderPipelineState, NSError * error) {
                                                              completionHandler(
																  RenderPipelineState(renderPipelineState, nullptr, ns::Ownership::Assign),
                                                                  ns::AutoReleasedError(error)
                                                              );
                                                          }];
#endif
    }

    void Device::NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineStateReflectionHandler completionHandler)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->NewRenderPipelineStateWithDescriptorOptionsCompletionHandler(m_ptr, descriptor.GetPtr(), MTLPipelineOption(options), ^(id <MTLRenderPipelineState> renderPipelineState, MTLRenderPipelineReflection * reflection, NSError * error) {
			completionHandler(
							  RenderPipelineState(renderPipelineState, cache, ns::Ownership::Assign),
							  AutoReleasedRenderPipelineReflection(reflection),
							  ns::AutoReleasedError(error)
							  );
		});
#else
        [(id<MTLDevice>)m_ptr newRenderPipelineStateWithDescriptor:(MTLRenderPipelineDescriptor*)descriptor.GetPtr()
                                                                    options:MTLPipelineOption(options)
                                                          completionHandler:^(id <MTLRenderPipelineState> renderPipelineState, MTLRenderPipelineReflection * reflection, NSError * error) {
                                                              completionHandler(
																  RenderPipelineState(renderPipelineState, nullptr, ns::Ownership::Assign),
                                                                  AutoReleasedRenderPipelineReflection(reflection),
                                                                  ns::AutoReleasedError(error)
                                                              );
                                                          }];
#endif
    }

    ComputePipelineState Device::NewComputePipelineState(const Function& computeFunction, ns::AutoReleasedError* error)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ComputePipelineState(m_table->NewComputePipelineStateWithFunctionError(m_ptr, computeFunction.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return ComputePipelineState([(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
                                                                                         error:error ? (NSError**)error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
    }

    ComputePipelineState Device::NewComputePipelineState(const Function& computeFunction, PipelineOption options, AutoReleasedComputePipelineReflection* outReflection, ns::AutoReleasedError* error)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ComputePipelineState(m_table->NewComputePipelineStateWithFunctionOptionsReflectionError(m_ptr, computeFunction.GetPtr(), MTLPipelineOption(options), outReflection ? outReflection->GetInnerPtr() : nil, error ? error->GetInnerPtr() : nil), m_table->TableCache, ns::Ownership::Assign);
#else
		return ComputePipelineState([(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
																  options:MTLPipelineOption(options)
															   reflection:outReflection ? outReflection.GetInnerPtr() : nil
																	error:error ? error->GetInnerPtr() : nil], nullptr, ns::Ownership::Assign);
#endif
    }

    void Device::NewComputePipelineState(const Function& computeFunction, ComputePipelineStateHandler completionHandler)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->NewComputePipelineStateWithFunctionCompletionHandler(m_ptr, computeFunction.GetPtr(), ^(id <MTLComputePipelineState> computePipelineState, NSError * error) {
			completionHandler(
							  ComputePipelineState(computePipelineState, cache, ns::Ownership::Assign),
							  ns::AutoReleasedError(error)
							  );
		});
#else
        [(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
                                                         completionHandler:^(id <MTLComputePipelineState> computePipelineState, NSError * error) {
                                                             completionHandler(
																 ComputePipelineState(computePipelineState, nullptr, ns::Ownership::Assign),
                                                                 ns::AutoReleasedError(error)
                                                             );
                                                         }];
#endif
    }

    void Device::NewComputePipelineState(const Function& computeFunction, PipelineOption options, ComputePipelineStateReflectionHandler completionHandler)
    {
        Validate();
		
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->NewComputePipelineStateWithFunctionOptionsCompletionHandler(m_ptr, computeFunction.GetPtr(), MTLPipelineOption(options), ^(id <MTLComputePipelineState> computePipelineState, MTLComputePipelineReflection * reflection, NSError * error) {
			completionHandler(
							  ComputePipelineState(computePipelineState, cache, ns::Ownership::Assign),
							  AutoReleasedComputePipelineReflection(reflection),
							  ns::AutoReleasedError(error)
							  );
		});
#else
        [(id<MTLDevice>)m_ptr newComputePipelineStateWithFunction:(id<MTLFunction>)computeFunction.GetPtr()
                                                                   options:MTLPipelineOption(options)
                                                         completionHandler:^(id <MTLComputePipelineState> computePipelineState, MTLComputePipelineReflection * reflection, NSError * error) {
                                                             completionHandler(
																 ComputePipelineState(computePipelineState, nullptr, ns::Ownership::Assign),
                                                                 AutoReleasedComputePipelineReflection(reflection),
                                                                 ns::AutoReleasedError(error)
                                                             );
                                                         }];
#endif
    }

    ComputePipelineState Device::NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, AutoReleasedComputePipelineReflection* outReflection, ns::AutoReleasedError* error)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ComputePipelineState(m_table->NewComputePipelineStateWithDescriptorOptionsReflectionError(m_ptr, descriptor.GetPtr(), MTLPipelineOption(options), outReflection ? outReflection->GetInnerPtr() : nullptr, error ? error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
        return ComputePipelineState([(id<MTLDevice>)m_ptr newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor*)descriptor.GetPtr()
                                                                                         options:MTLPipelineOption(options)
                                                                                      reflection:outReflection ? outReflection->GetInnerPtr() : nullptr
                                                                                           error:error ? error->GetInnerPtr() : nullptr], nullptr, ns::Ownership::Assign);
#endif
#else
        return nullptr;
#endif
    }

    void Device::NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, ComputePipelineStateReflectionHandler completionHandler)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->NewComputePipelineStateWithDescriptorOptionsCompletionHandler(m_ptr,descriptor.GetPtr(), MTLPipelineOption(options), ^(id <MTLComputePipelineState> computePipelineState, MTLComputePipelineReflection * reflection, NSError * error)
																			   {
																				   completionHandler(
																									 ComputePipelineState(computePipelineState, cache, ns::Ownership::Assign),
																									 AutoReleasedComputePipelineReflection(reflection),
																									 ns::AutoReleasedError(error));
																			   });
#else
        [(id<MTLDevice>)m_ptr newComputePipelineStateWithDescriptor:(MTLComputePipelineDescriptor*)descriptor.GetPtr()
                                                                     options:MTLPipelineOption(options)
                                                         completionHandler:^(id <MTLComputePipelineState> computePipelineState, MTLComputePipelineReflection * reflection, NSError * error)
                                                                    {
                                                                        completionHandler(
																			ComputePipelineState(computePipelineState, nullptr, ns::Ownership::Assign),
                                                                            AutoReleasedComputePipelineReflection(reflection),
                                                                            ns::AutoReleasedError(error));
                                                                    }];
#endif
#endif
    }

    Fence Device::NewFence() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_13, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return Fence(m_table->NewFence(m_ptr), m_table->TableCache, ns::Ownership::Assign);
#else
		if (@available(macOS 10.13, iOS 10.0, *))
			return Fence([(id<MTLDevice>)m_ptr newFence], nullptr, ns::Ownership::Assign);
		else
			return nullptr;
#endif
#else
		return nullptr;
#endif
    }

    bool Device::SupportsFeatureSet(FeatureSet featureSet) const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->supportsFeatureSet(m_ptr, MTLFeatureSet(featureSet));
#else
        return [(id<MTLDevice>)m_ptr supportsFeatureSet:MTLFeatureSet(featureSet)];
#endif
    }

    bool Device::SupportsTextureSampleCount(NSUInteger sampleCount) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->supportsTextureSampleCount(m_ptr, sampleCount);
#else
        return [(id<MTLDevice>)m_ptr supportsTextureSampleCount:sampleCount];
#endif
#else
        return true;
#endif
    }
	
	NSUInteger Device::GetMinimumLinearTextureAlignmentForPixelFormat(PixelFormat format) const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->minimumLinearTextureAlignmentForPixelFormat(m_ptr, (MTLPixelFormat)format);
#else
		return [(id<MTLDevice>)m_ptr minimumLinearTextureAlignmentForPixelFormat:(MTLPixelFormat)format];
#endif
#else
		return 0;
#endif
	}
	
	NSUInteger Device::GetMaxThreadgroupMemoryLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->maxThreadgroupMemoryLength(m_ptr);
#else
		return [(id<MTLDevice>)m_ptr maxThreadgroupMemoryLength];
#endif
#else
		return 0;
#endif
	}
	
	bool Device::AreProgrammableSamplePositionsSupported() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->areProgrammableSamplePositionsSupported(m_ptr);
#else
		return [(id<MTLDevice>)m_ptr areProgrammableSamplePositionsSupported];
#endif
#else
		return false;
#endif
	}
	
	void Device::GetDefaultSamplePositions(SamplePosition* positions, NSUInteger count)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->getDefaultSamplePositionscount(m_ptr, positions, count);
#else
		[(id<MTLDevice>)m_ptr getDefaultSamplePositions:(MTLSamplePosition *)positions count:count];
#endif
#endif
	}
	
	ArgumentEncoder Device::NewArgumentEncoderWithArguments(ns::Array<ArgumentDescriptor> const& arguments)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ArgumentEncoder(m_table->NewArgumentEncoderWithArguments(m_ptr, (NSArray<MTLArgumentDescriptor*>*)arguments.GetPtr()), m_table->TableCache, ns::Ownership::Assign);
#else
		return ArgumentEncoder([(id<MTLDevice>)m_ptr newArgumentEncoderWithArguments:(NSArray<MTLArgumentDescriptor *> *)arguments.GetPtr()], nullptr, ns::Ownership::Assign);
#endif
#else
		return ArgumentEncoder();
#endif
	}
	
	RenderPipelineState Device::NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, AutoReleasedRenderPipelineReflection* outReflection, ns::AutoReleasedError* error)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return RenderPipelineState(m_table->newRenderPipelineStateWithTileDescriptoroptionsreflectionerror(m_ptr, descriptor.GetPtr(), (MTLPipelineOption)options, outReflection ? outReflection->GetInnerPtr() : nullptr, error ? error->GetInnerPtr() : nullptr), m_table->TableCache, ns::Ownership::Assign);
#else
		return [m_ptr newRenderPipelineStateWithTileDescriptor:descriptor.GetPtr() options:(MTLPipelineOption)options reflection:outReflection ? outReflection->GetInnerPtr() : nullptr error:error ? error->GetInnerPtr() : nullptr];
#endif
#else
		return RenderPipelineState();
#endif
	}
	
	void Device::NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineStateReflectionHandler completionHandler)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_IOS(11_0)
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->newRenderPipelineStateWithTileDescriptoroptionscompletionHandler(m_ptr, descriptor.GetPtr(), (MTLPipelineOption)options, ^(id <MTLRenderPipelineState> renderPipelineState, MTLRenderPipelineReflection * reflection, NSError * error) {
			completionHandler(
							  RenderPipelineState(renderPipelineState, cache, ns::Ownership::Assign),
							  AutoReleasedRenderPipelineReflection(reflection),
							  ns::AutoReleasedError(error)
							  );
		});
#else
		[(id<MTLDevice>)m_ptr newRenderPipelineStateWithTileDescriptor: descriptor.GetPtr()
														   options:MTLPipelineOption(options)
												 completionHandler:^(id <MTLRenderPipelineState> renderPipelineState, MTLRenderPipelineReflection * reflection, NSError * error) {
													 completionHandler(
																	   RenderPipelineState(renderPipelineState, nullptr, ns::Ownership::Assign),
																	   AutoReleasedRenderPipelineReflection(reflection),
																	   ns::AutoReleasedError(error)
																	   );
												 }];
#endif
#endif
	}
	
#if MTLPP_CONFIG_VALIDATE
	Buffer ValidatedDevice::NewBuffer(NSUInteger length, ResourceOptions options)
	{
		Buffer Buf = Device::NewBuffer(length, options);
		ValidatedBuffer::Register(Buf);
		return Buf;
	}
	
	Buffer ValidatedDevice::NewBuffer(const void* pointer, NSUInteger length, ResourceOptions options)
	{
		Buffer Buf = Device::NewBuffer(pointer, length, options);
		ValidatedBuffer::Register(Buf);
		return Buf;
	}
	Buffer ValidatedDevice::NewBuffer(void* pointer, NSUInteger length, ResourceOptions options, BufferDeallocHandler deallocator)
	{
		Buffer Buf = Device::NewBuffer(pointer, length, options, deallocator);
		ValidatedBuffer::Register(Buf);
		return Buf;
	}
	
	Texture ValidatedDevice::NewTexture(const TextureDescriptor& descriptor)
	{
		Texture Tex = Device::NewTexture(descriptor);
		ValidatedTexture::Register(Tex);
		return Tex;
	}

	Texture ValidatedDevice::NewTextureWithDescriptor(const TextureDescriptor& descriptor, ns::IOSurface& iosurface, NSUInteger plane)
	{
		Texture Tex = Device::NewTextureWithDescriptor(descriptor, iosurface, plane);
		ValidatedTexture::Register(Tex);
		return Tex;
	}
#endif
}

MTLPP_END
