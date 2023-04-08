/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLFunctionConstantValues.h>
#include "function_constant_values.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    FunctionConstantValues::FunctionConstantValues() :
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        ns::Object<MTLFunctionConstantValues*>([[MTLFunctionConstantValues alloc] init], ns::Ownership::Assign)
#else
        ns::Object<MTLFunctionConstantValues*>(nullptr)
#endif
    {
    }

    void FunctionConstantValues::SetConstantValue(const void* value, DataType type, NSUInteger index)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLFunctionConstantValues*)m_ptr setConstantValue:value type:MTLDataType(type) atIndex:index];
#endif
    }

    void FunctionConstantValues::SetConstantValue(const void* value, DataType type, const ns::String& name)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLFunctionConstantValues*)m_ptr setConstantValue:value type:MTLDataType(type) withName:(NSString*)name.GetPtr()];
#endif
    }

    void FunctionConstantValues::SetConstantValues(const void* value, DataType type, const ns::Range& range)
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        [(MTLFunctionConstantValues*)m_ptr setConstantValues:value type:MTLDataType(type) withRange:NSMakeRange(range.Location, range.Length)];
#endif
    }

    void FunctionConstantValues::Reset()
    {
        Validate();
#if MTLPP_IS_AVAILABLE(10_12, 10_0)
        return [(MTLFunctionConstantValues*)m_ptr reset];
#endif
    }
}

MTLPP_END
