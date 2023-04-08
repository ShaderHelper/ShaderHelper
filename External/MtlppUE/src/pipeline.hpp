// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "declare.hpp"
#include "device.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	inline ITable<MTLPipelineBufferDescriptor*, void>* CreateIMPTable(MTLPipelineBufferDescriptor* handle)
	{
		static ITable<MTLPipelineBufferDescriptor*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
	enum class Mutability
	{
		Default   = 0,
		Mutable   = 1,
		Immutable = 2,
	}
	MTLPP_AVAILABLE(10_13, 11_0);
	
	class MTLPP_EXPORT PipelineBufferDescriptor : public ns::Object<MTLPipelineBufferDescriptor*>
	{
	public:
		PipelineBufferDescriptor();
		PipelineBufferDescriptor(ns::Ownership const retain) : ns::Object<MTLPipelineBufferDescriptor*>(retain) {}
		PipelineBufferDescriptor(MTLPipelineBufferDescriptor* h, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLPipelineBufferDescriptor*>(h, retain) {}
		
		void SetMutability(Mutability m);
		Mutability GetMutability() const;
	}
	MTLPP_AVAILABLE(10_13, 11_0);
}

MTLPP_END
