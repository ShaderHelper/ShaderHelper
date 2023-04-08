/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma clang push visibility(default)
#include <Foundation/NSDictionary.h>
#pragma clang pop visibility

#include <Metal/MTLLibrary.h>

#include "library.hpp"
#include "device.hpp"
#include "function_constant_values.hpp"
#include "argument_encoder.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    VertexAttribute::VertexAttribute() :
        ns::Object<MTLVertexAttribute*>([[MTLVertexAttribute alloc] init], ns::Ownership::Assign)
    {
    }

    ns::AutoReleased<ns::String> VertexAttribute::GetName() const
    {
        Validate();
        return ns::AutoReleased<ns::String>([(MTLVertexAttribute*)m_ptr name]);
    }

    NSUInteger VertexAttribute::GetAttributeIndex() const
    {
        Validate();
        return NSUInteger([(MTLVertexAttribute*)m_ptr attributeIndex]);
    }

    DataType VertexAttribute::GetAttributeType() const
    {
        Validate();
        return DataType([(MTLVertexAttribute*)m_ptr attributeType]);
    }

    bool VertexAttribute::IsActive() const
    {
        Validate();
        return [(MTLVertexAttribute*)m_ptr isActive];
    }

    bool VertexAttribute::IsPatchData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLVertexAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    bool VertexAttribute::IsPatchControlPointData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLVertexAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    Attribute::Attribute() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLAttribute*>([[MTLAttribute alloc] init], ns::Ownership::Assign)
#else
        ns::Object<MTLAttribute*>(nullptr)
#endif
    {
    }

    ns::AutoReleased<ns::String> Attribute::GetName() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return ns::AutoReleased<ns::String>([(MTLAttribute*)m_ptr name]);
#else
        return ns::AutoReleased<ns::String>();
#endif
    }

    NSUInteger Attribute::GetAttributeIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLAttribute*)m_ptr attributeIndex]);
#else
        return 0;
#endif
    }

    DataType Attribute::GetAttributeType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return DataType([(MTLAttribute*)m_ptr attributeType]);
#else
        return DataType(0);
#endif
    }

    bool Attribute::IsActive() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    bool Attribute::IsPatchData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    bool Attribute::IsPatchControlPointData() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLAttribute*)m_ptr isActive];
#else
        return false;
#endif
    }

    FunctionConstant::FunctionConstant() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLFunctionConstant*>([[MTLFunctionConstant alloc] init], ns::Ownership::Assign)
#else
        ns::Object<MTLFunctionConstant*>(nullptr)
#endif
    {
    }

    ns::AutoReleased<ns::String> FunctionConstant::GetName() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return ns::AutoReleased<ns::String>([(MTLFunctionConstant*)m_ptr name]);
#else
        return ns::AutoReleased<ns::String>();
#endif
    }

    DataType FunctionConstant::GetType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return DataType([(MTLFunctionConstant*)m_ptr type]);
#else
        return DataType(0);
#endif
    }

    NSUInteger FunctionConstant::GetIndex() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return NSUInteger([(MTLFunctionConstant*)m_ptr index]);
#else
        return 0;
#endif
    }

    bool FunctionConstant::IsRequired() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLFunctionConstant*)m_ptr required];
#else
        return false;
#endif
    }

    ns::AutoReleased<ns::String> Function::GetLabel() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLFunction>)m_ptr label]);
#endif
#else
        return ns::AutoReleased<ns::String>();
#endif
    }

    ns::AutoReleased<Device> Function::GetDevice() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Device>(m_table->Device(m_ptr));
#else
        return ns::AutoReleased<Device>([(id<MTLFunction>)m_ptr device]);
#endif
    }

    FunctionType Function::GetFunctionType() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return FunctionType(m_table->FunctionType(m_ptr));
#else
        return FunctionType([(id<MTLFunction>)m_ptr functionType]);
#endif
    }

    PatchType Function::GetPatchType() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return PatchType(m_table->PatchType(m_ptr));
#else
        return PatchType([(id<MTLFunction>)m_ptr patchType]);
#endif
#else
        return PatchType(0);
#endif
    }

    NSInteger Function::GetPatchControlPointCount() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->PatchControlPointCount(m_ptr);
#else
        return int32_t([(id<MTLFunction>)m_ptr patchControlPointCount]);
#endif
#else
        return 0;
#endif
    }

    const ns::AutoReleased<ns::Array<VertexAttribute>> Function::GetVertexAttributes() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<VertexAttribute>>(m_table->VertexAttributes(m_ptr));
#else
        return ns::AutoReleased<ns::Array<VertexAttribute>>([(id<MTLFunction>)m_ptr vertexAttributes]);
#endif
    }

    const ns::AutoReleased<ns::Array<Attribute>> Function::GetStageInputAttributes() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<Attribute>>(m_table->StageInputAttributes(m_ptr));
#else
        return ns::AutoReleased<ns::Array<Attribute>>([(id<MTLFunction>)m_ptr stageInputAttributes]);
#endif
#else
        return ns::AutoReleased<ns::Array<Attribute>>();
#endif
    }

    ns::AutoReleased<ns::String> Function::GetName() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Name(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLFunction>)m_ptr name]);
#endif
    }

    ns::AutoReleased<ns::Dictionary<ns::String, FunctionConstant>> Function::GetFunctionConstants() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Dictionary<ns::String, FunctionConstant>>(m_table->FunctionConstantsDictionary(m_ptr));
#else
        return ns::AutoReleased<ns::Dictionary<ns::String, FunctionConstant>>([(id<MTLFunction>)m_ptr functionConstantsDictionary]);
#endif
#else
        return ns::AutoReleased<ns::Dictionary<ns::String, FunctionConstant>>();
#endif
    }
	
	ArgumentEncoder Function::NewArgumentEncoderWithBufferIndex(NSUInteger index)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return ArgumentEncoder(m_table->NewArgumentEncoderWithBufferIndex(m_ptr, index), m_table->TableCache);
#else
		return [(id<MTLFunction>)m_ptr newArgumentEncoderWithBufferIndex:index];
#endif
#else
		return ArgumentEncoder();
#endif
	}

	ArgumentEncoder Function::NewArgumentEncoderWithBufferIndex(NSUInteger index, Argument* reflection)
	{
		Validate();
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
		
		MTLArgument* arg = nil;
#if MTLPP_CONFIG_IMP_CACHE
		ArgumentEncoder encoder(m_table->NewArgumentEncoderWithBufferIndexreflection(m_ptr, index, reflection ? &arg : nil), m_table->TableCache);
#else
		ArgumentEncoder encoder( [(id<MTLFunction>)m_ptr newArgumentEncoderWithBufferIndex:index reflection:reflection ? &arg : nil] );
#endif
		if (reflection) { *reflection = arg; }
		return encoder;
#else
		return ArgumentEncoder();
#endif
	}

    void Function::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetLabel(m_ptr, label.GetPtr());
#else
        [(id<MTLFunction>)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
#endif
    }

    ns::AutoReleased<ns::Dictionary<ns::String, ns::Object<NSObject*>>> CompileOptions::GetPreprocessorMacros() const
    {
        Validate();
        return ns::AutoReleased<ns::Dictionary<ns::String, ns::Object<NSObject*>>>([(MTLCompileOptions*)m_ptr preprocessorMacros]);
    }

    bool CompileOptions::IsFastMathEnabled() const
    {
        Validate();
        return [(MTLCompileOptions*)m_ptr fastMathEnabled];
    }

    LanguageVersion CompileOptions::GetLanguageVersion() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_11, 9_0)
        return LanguageVersion([(MTLCompileOptions*)m_ptr languageVersion]);
#else
        return LanguageVersion::Version1_0;
#endif
    }

    void CompileOptions::SetFastMathEnabled(bool fastMathEnabled)
    {
        Validate();
        [(MTLCompileOptions*)m_ptr setFastMathEnabled:fastMathEnabled];
    }

    void CompileOptions::SetLanguageVersion(LanguageVersion languageVersion)
    {
        Validate();
        [(MTLCompileOptions*)m_ptr setLanguageVersion:MTLLanguageVersion(languageVersion)];
    }
	
	void CompileOptions::SetPreprocessorMacros(ns::Dictionary<ns::String, ns::Object<NSObject*>> macros)
	{		
		Validate();
		[(MTLCompileOptions*)m_ptr setPreprocessorMacros:(NSDictionary<NSString *,NSObject *> *)macros.GetPtr()];
	}

    ns::AutoReleased<ns::String> Library::GetLabel() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::String>(m_table->Label(m_ptr));
#else
        return ns::AutoReleased<ns::String>([(id<MTLLibrary>)m_ptr label]);
#endif
    }

    void Library::SetLabel(const ns::String& label)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->SetLabel(m_ptr, label.GetPtr());
#else
        [(id<MTLLibrary>)m_ptr setLabel:(NSString*)label.GetPtr()];
#endif
    }

    ns::AutoReleased<ns::Array<ns::String>> Library::GetFunctionNames() const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<ns::Array<ns::String>>(m_table->FunctionNames(m_ptr));
#else
        return ns::AutoReleased<ns::Array<ns::String>>([(id<MTLLibrary>)m_ptr functionNames]);
#endif
    }

    Function Library::NewFunction(const ns::String& functionName) const
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		return Function(m_table->NewFunctionWithName(m_ptr, functionName.GetPtr()), m_table->TableCache);
#else
        return [(id<MTLLibrary>)m_ptr newFunctionWithName:(NSString*)functionName.GetPtr()];
#endif
    }

    Function Library::NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, ns::AutoReleasedError* error) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		return Function(m_table->NewFunctionWithNameconstantValueserror(m_ptr, functionName.GetPtr(), constantValues.GetPtr(), error ? (NSError**)error->GetInnerPtr() : nullptr), m_table->TableCache);
#else
		NSError** nsError = error ? (NSError**)error->GetInnerPtr() : nullptr;
        return [(id<MTLLibrary>)m_ptr
                                            newFunctionWithName:(NSString*)functionName.GetPtr()
                                            constantValues:(MTLFunctionConstantValues*)constantValues.GetPtr()
                                            error:nsError];
#endif
#else
        return nullptr;
#endif
    }

    void Library::NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, FunctionHandler completionHandler) const
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
#if MTLPP_CONFIG_IMP_CACHE
		UE::ITableCache* cache = m_table->TableCache;
		m_table->NewFunctionWithNameconstantValuescompletionHandler(m_ptr, functionName.GetPtr(), constantValues.GetPtr(), ^(id <MTLFunction> mtlFunction, NSError* error)
		{
			completionHandler(Function(mtlFunction, cache), error);
		});
#else
        [(id<MTLLibrary>)m_ptr
             newFunctionWithName:(NSString*)functionName.GetPtr()
             constantValues:(MTLFunctionConstantValues*)constantValues.GetPtr()
             completionHandler:^(id <MTLFunction> mtlFunction, NSError* error){
                 completionHandler(mtlFunction, error);
             }];
#endif
#endif
    }

}

MTLPP_END
