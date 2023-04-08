// Copyright Epic Games, Inc. All Rights Reserved.

#include <Metal/MTLPipeline.h>
#include "pipeline.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	PipelineBufferDescriptor::PipelineBufferDescriptor()
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
	: ns::Object<MTLPipelineBufferDescriptor*>([MTLPipelineBufferDescriptor new], ns::Ownership::Assign)
#endif
	{
	}
	
	void PipelineBufferDescriptor::SetMutability(Mutability m)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		[(MTLPipelineBufferDescriptor*)m_ptr setMutability:(MTLMutability)m];
#endif
	}
	
	Mutability PipelineBufferDescriptor::GetMutability() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		return (Mutability)[(MTLPipelineBufferDescriptor*)m_ptr mutability];
#else
		return 0;
#endif
	}
	
}

MTLPP_END
