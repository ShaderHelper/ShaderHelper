/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "ns.hpp"

MTLPP_BEGIN

namespace ns
{
    NSUInteger ArrayBase::GetSize(NSArray<id<NSObject>>* const handle)
    {
        return NSUInteger([handle count]);
    }

    void* ArrayBase::GetItem(NSArray<id<NSObject>>* const handle, NSUInteger index)
    {
        return (void*)[handle objectAtIndexedSubscript:index];
    }
	
	bool ArrayBase::EqualToArray(NSArray<id<NSObject>>* const Left, NSArray<id<NSObject>>* const Right)
	{
		return [Left isEqualToArray: Right];
	}

    String::String(const char* cstr) :
        Object<NSString*, CallingConvention::ObjectiveC>([NSString stringWithUTF8String:cstr], ns::Ownership::Retain)
    {
    }

    const char* String::GetCStr() const
    {
        Validate();
        return [(NSString*)m_ptr cStringUsingEncoding:NSUTF8StringEncoding];
    }

    NSUInteger String::GetLength() const
    {
        Validate();
        return NSUInteger([(NSString*)m_ptr length]);
    }

	ns::AutoReleased<String> Error::GetDomain() const
	{
		Validate();
		return ns::AutoReleased<String>([(NSError*)m_ptr domain]);
	}
	
	NSUInteger Error::GetCode() const
	{
		Validate();
		return NSUInteger([(NSError*)m_ptr code]);
	}
	
	//@property (readonly, copy) NSDictionary *userInfo;
	
	ns::AutoReleased<String> Error::GetLocalizedDescription() const
	{
		Validate();
		return ns::AutoReleased<String>([(NSError*)m_ptr localizedDescription]);
	}
	
	ns::AutoReleased<String> Error::GetLocalizedFailureReason() const
	{
		Validate();
		return ns::AutoReleased<String>([(NSError*)m_ptr localizedFailureReason]);
	}
	
	ns::AutoReleased<String> Error::GetLocalizedRecoverySuggestion() const
	{
		Validate();
		return ns::AutoReleased<String>([(NSError*)m_ptr localizedRecoverySuggestion]);
	}
	
	ns::AutoReleased<Array<String>> Error::GetLocalizedRecoveryOptions() const
	{
		Validate();
		return ns::AutoReleased<Array<String>>((NSArray<NSString*>*)[(NSError*)m_ptr localizedRecoveryOptions]);
	}
	
	//@property (nullable, readonly, strong) id recoveryAttempter;
	
	ns::AutoReleased<String> Error::GetHelpAnchor() const
	{
		Validate();
		return ns::AutoReleased<String>([(NSError*)m_ptr helpAnchor]);
	}
}

MTLPP_END
