// Copyright Epic Games, Inc. All Rights Reserved.

#include <Metal/MTLArgumentEncoder.h>
#include <Metal/MTLSampler.h>
#include "argument_encoder.hpp"
#include "device.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "sampler.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	ns::AutoReleased<Device>     ArgumentEncoder::GetDevice() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
		return ns::AutoReleased<Device>([(id<MTLArgumentEncoder>) m_ptr device]);
#endif
#else
		return ns::AutoReleased<Device>();
#endif
	}
	
	ns::AutoReleased<ns::String> ArgumentEncoder::GetLabel() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
		return ns::AutoReleased<ns::String>([(id<MTLArgumentEncoder>) m_ptr label]);
#endif
#else
		return ns::AutoReleased<ns::String>();
#endif
	}
	
	NSUInteger ArgumentEncoder::GetEncodedLength() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->EncodedLength(m_ptr);
#else
		return [(id<MTLArgumentEncoder>) m_ptr encodedLength];
#endif
#else
		return 0;
#endif
	}

	NSUInteger ArgumentEncoder::GetAlignment() const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->Alignment(m_ptr);
#else
		return [(id<MTLArgumentEncoder>) m_ptr alignment];
#endif
#else
		return 0;
#endif
	}
	
	void* ArgumentEncoder::GetConstantDataAtIndex(NSUInteger index) const
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->ConstantDataAtIndex(m_ptr, index);
#else
		return [(id<MTLArgumentEncoder>) m_ptr constantDataAtIndex:index];
#endif
#else
		return nullptr;
#endif
	}
	
	void ArgumentEncoder::SetLabel(const ns::String& label)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetLabel(m_ptr, label.GetPtr());
#else
		[(id<MTLArgumentEncoder>) m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
#endif
	}
	
	void ArgumentEncoder::SetArgumentBuffer(const Buffer& buffer, NSUInteger offset)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setargumentbufferoffset(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset());
#else
		[(id<MTLArgumentEncoder>) m_ptr setArgumentBuffer:(id<MTLBuffer>)buffer.GetPtr() offset:offset + buffer.GetOffset()];
#endif
#endif
	}
	
	void ArgumentEncoder::SetArgumentBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setargumentbufferstartoffsetarrayelement(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset(), index);
#else
		[(id<MTLArgumentEncoder>) m_ptr setArgumentBuffer:(id<MTLBuffer>)buffer.GetPtr() startOffset:offset + buffer.GetOffset() arrayElement:index];
#endif
#endif
	}
	
	void ArgumentEncoder::SetBuffer(const Buffer& buffer, NSUInteger offset, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setbufferoffsetatindex(m_ptr, (id<MTLBuffer>)buffer.GetPtr(), offset + buffer.GetOffset(), index);
#else
		[(id<MTLArgumentEncoder>) m_ptr setBuffer:(id<MTLBuffer>)buffer.GetPtr() offset:offset + buffer.GetOffset() atIndex:index];
#endif
#endif
	}
	
	void ArgumentEncoder::SetBuffers(const Buffer* buffers, const NSUInteger* offsets, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		id<MTLBuffer>* array = (id<MTLBuffer>*)alloca(range.Length * sizeof(id<MTLBuffer>));
		NSUInteger* theOffsets = (NSUInteger*)alloca(range.Length * sizeof(NSUInteger));
		for (NSUInteger i = 0; i < range.Length; i++)
		{
			array[i] = (id<MTLBuffer>)buffers[i].GetPtr();
			theOffsets[i] = offsets[i] + buffers[i].GetOffset();
		}
		
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setbuffersoffsetswithrange(m_ptr, array, (NSUInteger*)theOffsets, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLArgumentEncoder>) m_ptr setBuffers:array offsets:(const NSUInteger*)theOffsets withRange:NSMakeRange(range.Location, range.Length)];
#endif
#endif
	}
	
	void ArgumentEncoder::SetTexture(const Texture& texture, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Settextureatindex(m_ptr, (id<MTLTexture>)texture.GetPtr(), index);
#else
		[(id<MTLArgumentEncoder>) m_ptr setTexture:(id<MTLTexture>)texture.GetPtr() atIndex:index];
#endif
#endif
	}
	
	void ArgumentEncoder::SetTextures(const Texture* textures, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		id<MTLTexture>* array = (id<MTLTexture>*)alloca(range.Length * sizeof(id<MTLTexture>));
		for (NSUInteger i = 0; i < range.Length; i++)
			array[i] = (id<MTLTexture>)textures[i].GetPtr();

#if MTLPP_CONFIG_IMP_CACHE
		m_table->Settextureswithrange(m_ptr, array, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLArgumentEncoder>) m_ptr setTextures:array withRange:NSMakeRange(range.Location, range.Length)];
#endif
#endif
	}
	
	void ArgumentEncoder::SetSamplerState(const SamplerState& sampler, NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setsamplerstateatindex(m_ptr, (id<MTLSamplerState>)sampler.GetPtr(), index);
#else
		[(id<MTLArgumentEncoder>) m_ptr setSamplerState:(id<MTLSamplerState>)sampler.GetPtr() atIndex:index];
#endif
#endif
	}
	
	void ArgumentEncoder::SetSamplerStates(const SamplerState* samplers, const ns::Range& range)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		id<MTLSamplerState>* array = (id<MTLSamplerState>*)alloca(range.Length * sizeof(id<MTLSamplerState>));
		for (NSUInteger i = 0; i < range.Length; i++)
			array[i] = (id<MTLSamplerState>)samplers[i].GetPtr();

#if MTLPP_CONFIG_IMP_CACHE
		m_table->Setsamplerstateswithrange(m_ptr, array, NSMakeRange(range.Location, range.Length));
#else
		[(id<MTLArgumentEncoder>) m_ptr setSamplerStates:array withRange:NSMakeRange(range.Location, range.Length)];
#endif
#endif
	}
	
	ArgumentEncoder ArgumentEncoder::NewArgumentEncoderForBufferAtIndex(NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE_MAC(10_13)
#if MTLPP_CONFIG_IMP_CACHE
		return ArgumentEncoder(m_table->NewArgumentEncoderForBufferAtIndex(m_ptr, index), m_table->TableCache);
#else
		return [(id<MTLArgumentEncoder>) m_ptr newArgumentEncoderForBufferAtIndex:index];
#endif
#else
		return ArgumentEncoder();
#endif
	}
}

MTLPP_END
