/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once

#include "declare.hpp"
#include "imp_Library.hpp"
#include "ns.hpp"
#include "argument.hpp"

MTLPP_BEGIN

namespace UE
{
	template<>
	struct MTLPP_EXPORT ITable<id<MTLLibrary>, void> : public IMPTable<id<MTLLibrary>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLLibrary>, void>(C)
		{
		}
	};
	
	template<>
	struct MTLPP_EXPORT ITable<id<MTLFunction>, void> : public IMPTable<id<MTLFunction>, void>, public ITableCacheRef
	{
		ITable()
		{
		}
		
		ITable(Class C)
		: IMPTable<id<MTLFunction>, void>(C)
		{
		}
	};
	
	template<>
	inline ITable<MTLVertexAttribute*, void>* CreateIMPTable(MTLVertexAttribute* handle)
	{
		static ITable<MTLVertexAttribute*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLAttribute*, void>* CreateIMPTable(MTLAttribute* handle)
	{
		static ITable<MTLAttribute*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLFunctionConstant*, void>* CreateIMPTable(MTLFunctionConstant* handle)
	{
		static ITable<MTLFunctionConstant*, void> Table(object_getClass(handle));
		return &Table;
	}
	
	template<>
	inline ITable<MTLCompileOptions*, void>* CreateIMPTable(MTLCompileOptions* handle)
	{
		static ITable<MTLCompileOptions*, void> Table(object_getClass(handle));
		return &Table;
	}
}

namespace mtlpp
{
	class ArgumentEncoder;
    class Device;
    class FunctionConstantValues;

    enum class PatchType
    {
        None     = 0,
        Triangle = 1,
        Quad     = 2,
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    class MTLPP_EXPORT VertexAttribute : public ns::Object<MTLVertexAttribute*>
    {
    public:
        VertexAttribute();
		VertexAttribute(ns::Ownership const retain) : ns::Object<MTLVertexAttribute*>(retain) {}
        VertexAttribute(MTLVertexAttribute* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLVertexAttribute*>(handle, retain) { }

        ns::AutoReleased<ns::String>   GetName() const;
        NSUInteger     GetAttributeIndex() const;
        DataType     GetAttributeType() const MTLPP_AVAILABLE(10_11, 8_3);
        bool         IsActive() const;
        bool         IsPatchData() const MTLPP_AVAILABLE(10_12, 10_0);
        bool         IsPatchControlPointData() const MTLPP_AVAILABLE(10_12, 10_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT Attribute : public ns::Object<MTLAttribute*>
    {
    public:
        Attribute();
		Attribute(ns::Ownership const retain) : ns::Object<MTLAttribute*>(retain) {}
        Attribute(MTLAttribute* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLAttribute*>(handle, retain) { }

        ns::AutoReleased<ns::String>   GetName() const;
        NSUInteger     GetAttributeIndex() const;
        DataType     GetAttributeType() const MTLPP_AVAILABLE(10_11, 8_3);
        bool         IsActive() const;
        bool         IsPatchData() const MTLPP_AVAILABLE(10_12, 10_0);
        bool         IsPatchControlPointData() const MTLPP_AVAILABLE(10_12, 10_0);
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    enum class FunctionType
    {
        Vertex   = 1,
        Fragment = 2,
        Kernel   = 3,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    class MTLPP_EXPORT FunctionConstant : public ns::Object<MTLFunctionConstant*>
    {
    public:
        FunctionConstant();
		FunctionConstant(ns::Ownership const retain) : ns::Object<MTLFunctionConstant*>(retain) {}
        FunctionConstant(MTLFunctionConstant* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLFunctionConstant*>(handle, retain) { }

        ns::AutoReleased<ns::String> GetName() const;
        DataType   GetType() const;
        NSUInteger   GetIndex() const;
        bool       IsRequired() const;
    }
    MTLPP_AVAILABLE(10_12, 10_0);

    class MTLPP_EXPORT Function : public ns::Object<ns::Protocol<id<MTLFunction>>::type>
    {
    public:
		Function(ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLFunction>>::type>(retain) { }
		Function(ns::Protocol<id<MTLFunction>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLFunction>>::type>(handle, retain, UE::ITableCacheRef(cache).GetFunction(handle)) { }

        ns::AutoReleased<ns::String>                                   GetLabel() const MTLPP_AVAILABLE(10_12, 10_0);
        ns::AutoReleased<Device>                                       GetDevice() const;
        FunctionType                                 GetFunctionType() const;
        PatchType                                    GetPatchType() const MTLPP_AVAILABLE(10_12, 10_0);
        NSInteger                                    GetPatchControlPointCount() const MTLPP_AVAILABLE(10_12, 10_0);
        const ns::AutoReleased<ns::Array<VertexAttribute>>             GetVertexAttributes() const;
        const ns::AutoReleased<ns::Array<Attribute>>                   GetStageInputAttributes() const MTLPP_AVAILABLE(10_12, 10_0);
        ns::AutoReleased<ns::String>                                   GetName() const;
        ns::AutoReleased<ns::Dictionary<ns::String, FunctionConstant>> GetFunctionConstants() const MTLPP_AVAILABLE(10_12, 10_0);

		ArgumentEncoder NewArgumentEncoderWithBufferIndex(NSUInteger index) MTLPP_AVAILABLE(10_13, 11_0);
		ArgumentEncoder NewArgumentEncoderWithBufferIndex(NSUInteger index, Argument* reflection) MTLPP_AVAILABLE(10_13, 11_0);

        void SetLabel(const ns::String& label) MTLPP_AVAILABLE(10_12, 10_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class LanguageVersion
    {
        Version1_0 MTLPP_AVAILABLE(NA, 9_0)     = (1 << 16),
        Version1_1 MTLPP_AVAILABLE(10_11, 9_0)  = (1 << 16) + 1,
        Version1_2 MTLPP_AVAILABLE(10_12, 10_0) = (1 << 16) + 2,
        Version2_0 MTLPP_AVAILABLE(10_13, 11_0) = (2 << 16),
        Version2_1 MTLPP_AVAILABLE(10_14, 12_0) = (2 << 16) + 1,
        Version2_2 MTLPP_AVAILABLE(10_15, 13_0) = (2 << 16) + 2,
		Version2_3 MTLPP_AVAILABLE(11_0, 14_0) = (2 << 16) + 3,
     	Version2_4 MTLPP_AVAILABLE(12_0, 15_0) = (2 << 16) + 4

    }
    MTLPP_AVAILABLE(10_11, 9_0);

    class MTLPP_EXPORT CompileOptions : public ns::Object<MTLCompileOptions*>
    {
    public:
        CompileOptions();
        CompileOptions(MTLCompileOptions* handle, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<MTLCompileOptions*>(handle, retain) { }

        ns::AutoReleased<ns::Dictionary<ns::String, ns::Object<NSObject*>>> GetPreprocessorMacros() const;
        bool                                   IsFastMathEnabled() const;
        LanguageVersion                        GetLanguageVersion() const MTLPP_AVAILABLE(10_11, 9_0);

        void SetFastMathEnabled(bool fastMathEnabled);
        void SetLanguageVersion(LanguageVersion languageVersion) MTLPP_AVAILABLE(10_11, 9_0);
		void SetPreprocessorMacros(ns::Dictionary<ns::String, ns::Object<NSObject*>> macros);
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class LibraryError
    {
        Unsupported                                   = 1,
        Internal                                      = 2,
        CompileFailure                                = 3,
        CompileWarning                                = 4,
        FunctionNotFound MTLPP_AVAILABLE(10_12, 10_0) = 5,
        FileNotFound     MTLPP_AVAILABLE(10_12, 10_0) = 6,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

    enum class RenderPipelineError
    {
        Internal     = 1,
        Unsupported  = 2,
        InvalidInput = 3,
    }
    MTLPP_AVAILABLE(10_11, 8_0);

	MTLPP_CLOSURE(FunctionHandler, void, const Function&, const ns::Error&);
	
    class MTLPP_EXPORT Library : public ns::Object<ns::Protocol<id<MTLLibrary>>::type>
    {
    public:
        Library() { }
		Library(ns::Protocol<id<MTLLibrary>>::type handle, UE::ITableCache* cache = nullptr, ns::Ownership const retain = ns::Ownership::Retain) : ns::Object<ns::Protocol<id<MTLLibrary>>::type>(handle, retain, UE::ITableCacheRef(cache).GetLibrary(handle)) { }

        ns::AutoReleased<ns::String>            GetLabel() const;
        ns::AutoReleased<Device>                GetDevice() const;
        ns::AutoReleased<ns::Array<ns::String>> GetFunctionNames() const;

        void SetLabel(const ns::String& label);

        Function NewFunction(const ns::String& functionName) const;
        Function NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, ns::AutoReleasedError* error) const MTLPP_AVAILABLE(10_12, 10_0);
        void NewFunction(const ns::String& functionName, const FunctionConstantValues& constantValues, FunctionHandler completionHandler) const MTLPP_AVAILABLE(10_12, 10_0);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
