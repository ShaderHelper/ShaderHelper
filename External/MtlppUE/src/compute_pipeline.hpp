/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_ComputePipeline.hpp"
#include "device.hpp"
#include "argument.hpp"
#include "pipeline.hpp"
#include "stage_input_output_descriptor.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct MTLPP_EXPORT ITable<id<MTLComputePipelineState>, void> : public IMPTable<id<MTLComputePipelineState>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLComputePipelineState>, void>(C)
		{
		}
	};
	
	template<>
	inline ITable<MTLComputePipelineReflection*, void>* CreateIMPTable(MTLComputePipelineReflection* handle)
	{
		static MTLPP_EXPORT ITable<MTLComputePipelineReflection*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLComputePipelineDescriptor*, void>* CreateIMPTable(MTLComputePipelineDescriptor* handle)
	{
		static MTLPP_EXPORT ITable<MTLComputePipelineDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
	class PipelineBufferDescriptor;
	
    class MTLPP_EXPORT ComputePipelineReflection : public ns::Object<MTLComputePipelineReflection*>
	{
	public:
		ComputePipelineReflection();
		ComputePipelineReflection(ns::Ownership const retain) : ns::Object<MTLComputePipelineReflection*>(retain) {}
		ComputePipelineReflection(MTLComputePipelineReflection* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLComputePipelineReflection*>(handle, retain) { }
		
		ns::AutoReleased<ns::Array<Argument>> GetArguments() const;
	}
	MTLPP_AVAILABLE(10_11, 9_0);
	typedef ns::AutoReleased<ComputePipelineReflection> AutoReleasedComputePipelineReflection;

	class MTLPP_EXPORT ComputePipelineDescriptor : public ns::Object<MTLComputePipelineDescriptor*>
    {
    public:
        ComputePipelineDescriptor();
        ComputePipelineDescriptor(MTLComputePipelineDescriptor* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLComputePipelineDescriptor*>(handle, retain) { }

        ns::AutoReleased<ns::String>                 GetLabel() const;
        ns::AutoReleased<Function>                   GetComputeFunction() const;
        bool                       GetThreadGroupSizeIsMultipleOfThreadExecutionWidth() const;
        ns::AutoReleased<StageInputOutputDescriptor> GetStageInputDescriptor() const MTLPP_AVAILABLE(10_12, 10_0);
		ns::AutoReleased<ns::Array<PipelineBufferDescriptor>> GetBuffers() const MTLPP_AVAILABLE(10_13, 11_0);
		NSUInteger GetMaxTotalThreadsPerThreadgroup() const MTLPP_AVAILABLE(10_14, 12_0);

        void SetLabel(const ns::String& label);
        void SetComputeFunction(const Function& function);
        void SetThreadGroupSizeIsMultipleOfThreadExecutionWidth(bool value);
        void SetStageInputDescriptor(const StageInputOutputDescriptor& stageInputDescriptor) const MTLPP_AVAILABLE(10_12, 10_0);
		void SetMaxTotalThreadsPerThreadgroup(NSUInteger ThreadCount) MTLPP_AVAILABLE(10_14, 12_0);

        void Reset();
    }
    MTLPP_AVAILABLE(10_11, 9_0);

    class MTLPP_EXPORT ComputePipelineState : public ns::Object<ns::Protocol<id<MTLComputePipelineState>>::type>
    {
    public:
        ComputePipelineState() { }
		ComputePipelineState(ns::Protocol<id<MTLComputePipelineState>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLComputePipelineState>>::type>(handle, retain, UE::ITableCacheRef(cache).GetComputePipelineState(handle)) { }

		ns::AutoReleased<ns::String> GetLabel() const MTLPP_AVAILABLE(10_13, 11_0);
        ns::AutoReleased<Device>   GetDevice() const;
        NSUInteger GetMaxTotalThreadsPerThreadgroup() const;
        NSUInteger GetThreadExecutionWidth() const;
		NSUInteger GetStaticThreadgroupMemoryLength() const MTLPP_AVAILABLE(10_13, 11_0);
		
		NSUInteger GetImageblockMemoryLengthForDimensions(Size const& imageblockDimensions) MTLPP_AVAILABLE_IOS(11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
