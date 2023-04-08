/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLParallelRenderCommandEncoder.h>
#include <Metal/MTLSampler.h>
#include <Metal/MTLStageInputOutputDescriptor.h>
#include "parallel_render_command_encoder.hpp"
#include "render_command_encoder.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    RenderCommandEncoder ParallelRenderCommandEncoder::GetRenderCommandEncoder()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		RenderCommandEncoder Encoder = RenderCommandEncoder(m_table->RenderCommandEncoder(m_ptr), m_table->TableCache);
#else
        RenderCommandEncoder Encoder = [(id<MTLParallelRenderCommandEncoder>)m_ptr renderCommandEncoder];
#endif
#if MTLPP_CONFIG_VALIDATE
		Encoder.SetCommandBufferFence(GetCommandBufferFence());
#endif
		return Encoder;
    }

    void ParallelRenderCommandEncoder::SetColorStoreAction(StoreAction storeAction, NSUInteger colorAttachmentIndex)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetcolorstoreactionAtindex(m_ptr, MTLStoreAction(storeAction), colorAttachmentIndex);
#else
        [(id<MTLParallelRenderCommandEncoder>)m_ptr setColorStoreAction:MTLStoreAction(storeAction) atIndex:colorAttachmentIndex];
#endif
#endif
    }

    void ParallelRenderCommandEncoder::SetDepthStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setdepthstoreaction(m_ptr, MTLStoreAction(storeAction));
#else
        [(id<MTLParallelRenderCommandEncoder>)m_ptr setDepthStoreAction:MTLStoreAction(storeAction)];
#endif
#endif
    }

    void ParallelRenderCommandEncoder::SetStencilStoreAction(StoreAction storeAction)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setstencilstoreaction(m_ptr, MTLStoreAction(storeAction));
#else
        [(id<MTLParallelRenderCommandEncoder>)m_ptr setStencilStoreAction:MTLStoreAction(storeAction)];
#endif
#endif
    }
	
	void ParallelRenderCommandEncoder::SetColorStoreActionOptions(StoreActionOptions options, NSUInteger colorAttachmentIndex)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetcolorstoreactionoptionsAtindex(m_ptr, MTLStoreActionOptions(options), colorAttachmentIndex);
#else
		[(id<MTLParallelRenderCommandEncoder>)m_ptr setColorStoreActionOptions:(MTLStoreActionOptions)options atIndex:colorAttachmentIndex];
#endif
#endif
	}
	
	void ParallelRenderCommandEncoder::SetDepthStoreActionOptions(StoreActionOptions options)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setdepthstoreactionoptions(m_ptr, MTLStoreActionOptions(options));
#else
		[(id<MTLParallelRenderCommandEncoder>)m_ptr setDepthStoreActionOptions:(MTLStoreActionOptions)options];
#endif
#endif
	}
	
	void ParallelRenderCommandEncoder::SetStencilStoreActionOptions(StoreActionOptions options)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setstencilstoreactionoptions(m_ptr, MTLStoreActionOptions(options));
#else
		[(id<MTLParallelRenderCommandEncoder>)m_ptr setStencilStoreActionOptions:(MTLStoreActionOptions)options];
#endif
#endif
	}
	
#if MTLPP_CONFIG_VALIDATE
	RenderCommandEncoder ValidatedParallelRenderCommandEncoder::GetRenderCommandEncoder()
	{
		RenderCommandEncoder Enc = ParallelRenderCommandEncoder::GetRenderCommandEncoder();
		Validator.AddEncoderValidator(Enc);
		return Enc;
	}
#endif
}

MTLPP_END
