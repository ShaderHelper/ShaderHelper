/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_ParallelRenderCommandEncoder.hpp"
#include "ns.hpp"
#include "render_pass.hpp"
#include "command_encoder.hpp"
#include "validation.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct ITable<id<MTLParallelRenderCommandEncoder>, void> : public IMPTable<id<MTLParallelRenderCommandEncoder>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLParallelRenderCommandEncoder>, void>(C)
		{
		}
	};
}

namespace mtlpp
{
    class RenderCommandEncoder;

    class MTLPP_EXPORT ParallelRenderCommandEncoder : public CommandEncoder<ns::Protocol<id<MTLParallelRenderCommandEncoder>>::type>
    {
    public:
        ParallelRenderCommandEncoder(ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLParallelRenderCommandEncoder>>::type>(retain) { }
		ParallelRenderCommandEncoder(ns::Protocol<id<MTLParallelRenderCommandEncoder>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : CommandEncoder<ns::Protocol<id<MTLParallelRenderCommandEncoder>>::type>(handle, retain, UE::ITableCacheRef(cache).GetParallelRenderCommandEncoder(handle)) { }

        MTLPP_VALIDATED RenderCommandEncoder GetRenderCommandEncoder();

        void SetColorStoreAction(StoreAction storeAction, NSUInteger colorAttachmentIndex) MTLPP_AVAILABLE(10_12, 10_0);
        void SetDepthStoreAction(StoreAction storeAction) MTLPP_AVAILABLE(10_12, 10_0);
        void SetStencilStoreAction(StoreAction storeAction) MTLPP_AVAILABLE(10_12, 10_0);
		
		void SetColorStoreActionOptions(StoreActionOptions options, NSUInteger colorAttachmentIndex) MTLPP_AVAILABLE(10_13, 11_0);
		void SetDepthStoreActionOptions(StoreActionOptions options) MTLPP_AVAILABLE(10_13, 11_0);
		void SetStencilStoreActionOptions(StoreActionOptions options) MTLPP_AVAILABLE(10_13, 11_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
	
#if MTLPP_CONFIG_VALIDATE
	class MTLPP_EXPORT ValidatedParallelRenderCommandEncoder : public ns::AutoReleased<ParallelRenderCommandEncoder>
	{
		ParallelEncoderValidationTable Validator;
		
	public:
		ValidatedParallelRenderCommandEncoder()
		: Validator(nullptr)
		{
		}
		
		ValidatedParallelRenderCommandEncoder(ParallelRenderCommandEncoder& Wrapped)
		: ns::AutoReleased<ParallelRenderCommandEncoder>(Wrapped)
		, Validator(Wrapped.GetAssociatedObject<ParallelEncoderValidationTable>(ParallelEncoderValidationTable::kTableAssociationKey).GetPtr())
		{
		}
		
		MTLPP_VALIDATED RenderCommandEncoder GetRenderCommandEncoder();
	};
	
	template <>
	class MTLPP_EXPORT Validator<ParallelRenderCommandEncoder>
	{
		public:
		Validator(ParallelRenderCommandEncoder& Val, bool bEnable)
		: Resource(Val)
		{
			if (bEnable)
			{
				Validation = ValidatedParallelRenderCommandEncoder(Val);
			}
		}
		
		ValidatedParallelRenderCommandEncoder& operator*()
		{
			assert(Validation.GetPtr() != nullptr);
			return Validation;
		}
		
		ParallelRenderCommandEncoder* operator->()
		{
			return Validation.GetPtr() == nullptr ? &Resource : &Validation;
		}
		
		private:
		ParallelRenderCommandEncoder& Resource;
		ValidatedParallelRenderCommandEncoder Validation;
	};
#endif
}

MTLPP_END
