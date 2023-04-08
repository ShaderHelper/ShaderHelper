/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Device.hpp"
#include "types.hpp"
#include "pixel_format.hpp"
#include "resource.hpp"
#include "library.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLDevice>, void> : public IMPTable<id<MTLDevice>, void>
	{
		ITable()
		: TableCache(new ITableCache)
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLDevice>, void>(C)
		, TableCache(new ITableCache)
		{
		}
		
		~ITable()
		{
			if (TableCache)
				delete TableCache;
		}
		
		ITableCache* TableCache;
	};
	
	template<>
	inline ITable<MTLArgumentDescriptor*, void>* CreateIMPTable(MTLArgumentDescriptor* handle)
	{
		static ITable<MTLArgumentDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
	class ArgumentEncoder;
    class CommandQueue;
    class Device;
    class Buffer;
    class DepthStencilState;
    class Function;
    class Library;
    class Texture;
    class SamplerState;
    class RenderPipelineState;
    class ComputePipelineState;
    class Heap;
    class Fence;

    class SamplerDescriptor;
    class RenderPipelineColorAttachmentDescriptor;
    class DepthStencilDescriptor;
    class TextureDescriptor;
    class CompileOptions;
    class RenderPipelineDescriptor;
    class RenderPassDescriptor;
	class RenderPipelineReflection;
	typedef ns::AutoReleased<RenderPipelineReflection> AutoReleasedRenderPipelineReflection;
	class TileRenderPipelineDescriptor;
    class ComputePipelineDescriptor;
    class MTLPP_EXPORT ComputePipelineReflection;
	typedef ns::AutoReleased<ComputePipelineReflection> AutoReleasedComputePipelineReflection;
    class CommandQueueDescriptor;
    class HeapDescriptor;

    enum class FeatureSet
    {
        iOS_GPUFamily1_v1         MTLPP_AVAILABLE_IOS(8_0)   = 0,
        iOS_GPUFamily2_v1         MTLPP_AVAILABLE_IOS(8_0)   = 1,

        iOS_GPUFamily1_v2         MTLPP_AVAILABLE_IOS(8_0)   = 2,
        iOS_GPUFamily2_v2         MTLPP_AVAILABLE_IOS(8_0)   = 3,
        iOS_GPUFamily3_v1         MTLPP_AVAILABLE_IOS(9_0)   = 4,

        iOS_GPUFamily1_v3         MTLPP_AVAILABLE_IOS(10_0)  = 5,
        iOS_GPUFamily2_v3         MTLPP_AVAILABLE_IOS(10_0)  = 6,
        iOS_GPUFamily3_v2         MTLPP_AVAILABLE_IOS(10_0)  = 7,
		
		iOS_GPUFamily1_v4         MTLPP_AVAILABLE_IOS(11_0)  = 8,
		iOS_GPUFamily2_v4         MTLPP_AVAILABLE_IOS(11_0)  = 9,
		iOS_GPUFamily3_v3         MTLPP_AVAILABLE_IOS(11_0)  = 10,
		iOS_GPUFamily4_v1 		  MTLPP_AVAILABLE_IOS(11_0)  = 11,
		
		iOS_GPUFamily1_v5         MTLPP_AVAILABLE_IOS(12_0)  = 12,
		iOS_GPUFamily2_v5         MTLPP_AVAILABLE_IOS(12_0)  = 13,
		iOS_GPUFamily3_v4         MTLPP_AVAILABLE_IOS(12_0)  = 14,
		iOS_GPUFamily4_v2 		  MTLPP_AVAILABLE_IOS(12_0)  = 15,
		iOS_GPUFamily5_v1 		  MTLPP_AVAILABLE_IOS(12_0)  = 16,

        macOS_GPUFamily1_v1         MTLPP_AVAILABLE_MAC(10_11)   = 10000,

        macOS_GPUFamily1_v2         MTLPP_AVAILABLE_MAC(10_12) = 10001,
        macOS_ReadWriteTextureTier2 MTLPP_AVAILABLE_MAC(10_12) = 10002,

		macOS_GPUFamily1_v3         MTLPP_AVAILABLE_MAC(10_13) = 10003,
		
		macOS_GPUFamily1_v4         MTLPP_AVAILABLE_MAC(10_14) = 10004,
		macOS_GPUFamily2_v1         MTLPP_AVAILABLE_MAC(10_14) = 10005,
		
        tvOS_GPUFamily1_v1        MTLPP_AVAILABLE_TVOS(9_0)  = 30000,

        tvOS_GPUFamily1_v2        MTLPP_AVAILABLE_TVOS(10_0) = 30001,
		
		tvOS_GPUFamily1_v3        MTLPP_AVAILABLE_TVOS(11_0) = 30002,
		tvOS_GPUFamily2_v1 		MTLPP_AVAILABLE_TVOS(11_0) = 30003,
		
		tvOS_GPUFamily1_v4        MTLPP_AVAILABLE_TVOS(12_0) = 30004,
		tvOS_GPUFamily2_v2 		MTLPP_AVAILABLE_TVOS(12_0) = 30005,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	enum PipelineOption : NSUInteger
    {
        NoPipelineOption    = 0,
        ArgumentInfo   		= 1 << 0,
        BufferTypeInfo 		= 1 << 1,
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
	enum class ReadWriteTextureTier
	{
		None  = 0,
		Tier1 = 1,
		Tier2 = 2,
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	enum class ArgumentBuffersTier
	{
		Tier1 = 0,
		Tier2 = 1,
	}
	MTLPP_AVAILABLE(10_13, 11_0);

    struct SizeAndAlign
    {
        NSUInteger Size;
        NSUInteger Align;
    };
	
	class MTLPP_EXPORT ArgumentDescriptor : public ns::Object<MTLArgumentDescriptor*>
	{
	public:
		ArgumentDescriptor();
		ArgumentDescriptor(ns::Ownership const retain);
		ArgumentDescriptor(MTLArgumentDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLArgumentDescriptor*>(handle, retain) {}
		
		DataType GetDataType() const;
		NSUInteger GetIndex() const;
		NSUInteger GetArrayLength() const;
		ArgumentAccess GetAccess() const;
		TextureType GetTextureType() const;
		NSUInteger GetConstantBlockAlignment() const;
		
		void SetDataType(DataType Type);
		void SetIndex(NSUInteger Index);
		void SetArrayLength(NSUInteger Len);
		void SetAccess(ArgumentAccess Access);
		void SetTextureType(TextureType Type);
		void SetConstantBlockAlignment(NSUInteger Align);
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	MTLPP_CLOSURE(DeviceHandler, void, const Device&, ns::String const&);
	MTLPP_CLOSURE(BufferDeallocHandler, void, void* pointer, NSUInteger length);
	MTLPP_CLOSURE(LibraryHandler, void, const Library&, const ns::AutoReleasedError&);
	MTLPP_CLOSURE(RenderPipelineStateHandler, void, const RenderPipelineState&, const ns::AutoReleasedError&);
	MTLPP_CLOSURE(RenderPipelineStateReflectionHandler, void, const RenderPipelineState&, const AutoReleasedRenderPipelineReflection&, const ns::AutoReleasedError&);
	MTLPP_CLOSURE(ComputePipelineStateHandler, void, const ComputePipelineState&, const ns::AutoReleasedError&);
	MTLPP_CLOSURE(ComputePipelineStateReflectionHandler, void, const ComputePipelineState&, const AutoReleasedComputePipelineReflection&, const ns::AutoReleasedError&);

	class MTLPP_EXPORT Device : public ns::Object<ns::Protocol<id<MTLDevice>>::type>
    {
    public:
		Device(ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLDevice>>::type>(retain) { }
        Device(ns::Protocol<id<MTLDevice>>::type handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLDevice>>::type>(handle, retain) { }

		static ns::AutoReleased<ns::String> GetWasAddedNotification() MTLPP_AVAILABLE_MAC(10_13);
		static ns::AutoReleased<ns::String> GetRemovalRequestedNotification() MTLPP_AVAILABLE_MAC(10_13);
		static ns::AutoReleased<ns::String> GetWasRemovedNotification() MTLPP_AVAILABLE_MAC(10_13);
		
		static ns::Array<Device> CopyAllDevicesWithObserver(ns::Object<id <NSObject>>& observer, DeviceHandler handler) MTLPP_AVAILABLE_MAC(10_13);
		static void RemoveDeviceObserver(ns::Object<id <NSObject>> observer) MTLPP_AVAILABLE_MAC(10_13);
		
        static Device CreateSystemDefaultDevice() MTLPP_AVAILABLE(10_11, 8_0);
        static ns::Array<Device> CopyAllDevices() MTLPP_AVAILABLE(10_11, NA);

        ns::AutoReleased<ns::String> GetName() const;
        Size       GetMaxThreadsPerThreadgroup() const MTLPP_AVAILABLE(10_11, 9_0);
        bool       IsLowPower() const MTLPP_AVAILABLE_MAC(10_11);
        bool       IsHeadless() const MTLPP_AVAILABLE_MAC(10_11);
		bool       IsRemovable() const MTLPP_AVAILABLE_MAC(10_13);
        uint64_t   GetRecommendedMaxWorkingSetSize() const MTLPP_AVAILABLE_MAC(10_12);
        bool       IsDepth24Stencil8PixelFormatSupported() const MTLPP_AVAILABLE_MAC(10_11);
		
		uint64_t GetRegistryID() const MTLPP_AVAILABLE(10_13, 11_0);
		
		ReadWriteTextureTier GetReadWriteTextureSupport() const MTLPP_AVAILABLE(10_13, 11_0);
		ArgumentBuffersTier GetArgumentsBufferSupport() const MTLPP_AVAILABLE(10_13, 11_0);
		
		bool AreRasterOrderGroupsSupported() const MTLPP_AVAILABLE(10_13, 11_0);
		
		uint64_t GetCurrentAllocatedSize() const MTLPP_AVAILABLE(10_13, 11_0);

        CommandQueue NewCommandQueue();
        CommandQueue NewCommandQueue(NSUInteger maxCommandBufferCount);
        SizeAndAlign HeapTextureSizeAndAlign(const TextureDescriptor& desc) MTLPP_AVAILABLE(10_13, 10_0);
        SizeAndAlign HeapBufferSizeAndAlign(NSUInteger length, ResourceOptions options) MTLPP_AVAILABLE(10_13, 10_0);
        Heap NewHeap(const HeapDescriptor& descriptor) MTLPP_AVAILABLE(10_13, 10_0);
        MTLPP_VALIDATED Buffer NewBuffer(NSUInteger length, ResourceOptions options);
        MTLPP_VALIDATED Buffer NewBuffer(const void* pointer, NSUInteger length, ResourceOptions options);
        MTLPP_VALIDATED Buffer NewBuffer(void* pointer, NSUInteger length, ResourceOptions options, BufferDeallocHandler deallocator);
        DepthStencilState NewDepthStencilState(const DepthStencilDescriptor& descriptor);
        MTLPP_VALIDATED Texture NewTexture(const TextureDescriptor& descriptor);
		MTLPP_VALIDATED Texture NewTextureWithDescriptor(const TextureDescriptor& descriptor, ns::IOSurface& iosurface, NSUInteger plane) MTLPP_AVAILABLE(10_11, NA);
        SamplerState NewSamplerState(const SamplerDescriptor& descriptor);
        Library NewDefaultLibrary();
		Library NewDefaultLibraryWithBundle(const ns::Bundle& bundle, ns::AutoReleasedError* error) MTLPP_AVAILABLE(10_12, 10_0);
        Library NewLibrary(const ns::String& filepath, ns::AutoReleasedError* error);
		Library NewLibrary(dispatch_data_t data, ns::AutoReleasedError* error);
		Library NewLibrary(ns::String source, const CompileOptions& options, ns::AutoReleasedError* error);
		Library NewLibrary(ns::URL const& url, ns::AutoReleasedError* error) MTLPP_AVAILABLE(10_13, 11_0);
        void NewLibrary(ns::String source, const CompileOptions& options, LibraryHandler completionHandler);
        RenderPipelineState NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, ns::AutoReleasedError* error);
        RenderPipelineState NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, AutoReleasedRenderPipelineReflection* outReflection, ns::AutoReleasedError* error);
        void NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, RenderPipelineStateHandler completionHandler);
        void NewRenderPipelineState(const RenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineStateReflectionHandler completionHandler);
        ComputePipelineState NewComputePipelineState(const Function& computeFunction, ns::AutoReleasedError* error);
        ComputePipelineState NewComputePipelineState(const Function& computeFunction, PipelineOption options, AutoReleasedComputePipelineReflection* outReflection, ns::AutoReleasedError* error);
        void NewComputePipelineState(const Function& computeFunction, ComputePipelineStateHandler completionHandler);
        void NewComputePipelineState(const Function& computeFunction, PipelineOption options, ComputePipelineStateReflectionHandler completionHandler);
        ComputePipelineState NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, AutoReleasedComputePipelineReflection* outReflection, ns::AutoReleasedError* error);
        void NewComputePipelineState(const ComputePipelineDescriptor& descriptor, PipelineOption options, ComputePipelineStateReflectionHandler completionHandler) MTLPP_AVAILABLE(10_11, 9_0);
        Fence NewFence() const MTLPP_AVAILABLE(10_13, 10_0);
        bool SupportsFeatureSet(FeatureSet featureSet) const;
        bool SupportsTextureSampleCount(NSUInteger sampleCount) const MTLPP_AVAILABLE(10_11, 9_0);
		NSUInteger GetMinimumLinearTextureAlignmentForPixelFormat(PixelFormat format) const MTLPP_AVAILABLE(10_13, 11_0);
		NSUInteger GetMaxThreadgroupMemoryLength() const MTLPP_AVAILABLE(10_13, 11_0);
		bool AreProgrammableSamplePositionsSupported() const MTLPP_AVAILABLE(10_13, 11_0);
		void GetDefaultSamplePositions(SamplePosition* positions, NSUInteger count) MTLPP_AVAILABLE(10_13, 11_0);
		ArgumentEncoder NewArgumentEncoderWithArguments(ns::Array<ArgumentDescriptor> const& arguments) MTLPP_AVAILABLE(10_13, 11_0);
		RenderPipelineState NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, AutoReleasedRenderPipelineReflection* outReflection, ns::AutoReleasedError* error) MTLPP_AVAILABLE_IOS(11_0);
		void NewRenderPipelineState(const TileRenderPipelineDescriptor& descriptor, PipelineOption options, RenderPipelineStateReflectionHandler completionHandler) MTLPP_AVAILABLE_IOS(11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedDevice : public ns::AutoReleased<Device>
	{
		DeviceValidationTable Validator;
		
	public:
		static void Register(Device& Wrapped)
		{
			DeviceValidationTable Register(Wrapped);
		}
		
		ValidatedDevice()
		: Validator(nullptr)
		{
		}
		
		ValidatedDevice(Device& Wrapped)
		: ns::AutoReleased<Device>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<DeviceValidationTable>(DeviceValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED Buffer NewBuffer(NSUInteger length, ResourceOptions options);
		MTLPP_VALIDATED Buffer NewBuffer(const void* pointer, NSUInteger length, ResourceOptions options);
		MTLPP_VALIDATED Buffer NewBuffer(void* pointer, NSUInteger length, ResourceOptions options, BufferDeallocHandler deallocator);
		
		MTLPP_VALIDATED Texture NewTexture(const TextureDescriptor& descriptor);
		MTLPP_VALIDATED Texture NewTextureWithDescriptor(const TextureDescriptor& descriptor, ns::IOSurface& iosurface, NSUInteger plane) MTLPP_AVAILABLE(10_11, NA);
	};
	
	template <>
	class MTLPP_EXPORT Validator<Device>
	{
		public:
		Validator(Device& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedDevice(Val);
			}
		}
		
		ValidatedDevice& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		Device* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		Device& Resource;
		ValidatedDevice Validation;
	};
#endif
}

MTLPP_END
