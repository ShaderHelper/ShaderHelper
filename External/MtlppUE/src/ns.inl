/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "ns.hpp"
#include "imp_cache.hpp"
#ifdef __OBJC__
#import <CoreFoundation/CFBase.h>
#import <Foundation/NSString.h>
#import <Foundation/NSError.h>
#import <Foundation/NSArray.h>
#import <cstring>
#endif

namespace ns
{
	template<typename T, CallingConvention C>
    Object<T, C>::Object(ns::Ownership const retain) :
        m_ptr(nullptr),
#if MTLPP_CONFIG_IMP_CACHE
		m_table(nullptr),
#endif
		Mode(retain)
    {
    }

	template<typename T, CallingConvention C>
    Object<T, C>::Object(const T handle, ns::Ownership const retain, ITable* table) :
	m_ptr(handle),
#if MTLPP_CONFIG_IMP_CACHE
	m_table(table),
#endif
	Mode(retain)
    {
		if (m_ptr)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (C == CallingConvention::C && !m_table)
			{
				m_table = UE::CreateIMPTable(handle);
			}
#endif
			if (Mode == ns::Ownership::Retain)
			{
#if MTLPP_CONFIG_IMP_CACHE
				if (C == CallingConvention::C || m_table)
				{
					assert(m_table);
					m_table->Retain(m_ptr);
				}
				else
#endif
				{
					CFRetain(m_ptr);
				}
			}
		}
    }

	template<typename T, CallingConvention C>
    Object<T, C>::Object(const Object& rhs) :
	m_ptr(rhs.m_ptr),
#if MTLPP_CONFIG_IMP_CACHE
	m_table(rhs.m_table),
#endif
	Mode(ns::Ownership::Retain)
    {
		if (m_ptr)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (C == CallingConvention::C || m_table)
			{
				if (!m_table)
				{
					m_table = UE::CreateIMPTable(m_ptr);
				}
				assert(m_table);
				m_table->Retain(m_ptr);
			}
			else
#endif
			{
				CFRetain(m_ptr);
			}
		}
    }

#if MTLPP_CONFIG_RVALUE_REFERENCES
	template<typename T, CallingConvention C>
    Object<T, C>::Object(Object&& rhs) :
	m_ptr(rhs.m_ptr),
#if MTLPP_CONFIG_IMP_CACHE
	m_table(rhs.m_table),
#endif
	Mode(ns::Ownership::Retain)
    {
		if (m_ptr)
		{
#if	MTLPP_CONFIG_IMP_CACHE
			if (C == CallingConvention::C || m_table)
			{
				if (!m_table)
				{
					m_table = UE::CreateIMPTable(m_ptr);
				}
				assert(m_table);
				if (rhs.Mode == ns::Ownership::AutoRelease)
				{
					m_table->Retain(m_ptr);
				}
			}
			else
#endif
			if (rhs.Mode == ns::Ownership::AutoRelease)
			{
				CFRetain(rhs.m_ptr);
			}

			if (Mode != ns::Ownership::AutoRelease || rhs.Mode == ns::Ownership::AutoRelease)
			{
				rhs.m_ptr = nullptr;
#if MTLPP_CONFIG_IMP_CACHE
				rhs.m_table = nullptr;
#endif
			}
		}
    }
#endif

	template<typename T, CallingConvention C>
    Object<T, C>::~Object()
    {
		if (Mode != ns::Ownership::AutoRelease && m_ptr)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (C == CallingConvention::C || m_table)
			{
				assert(m_table);
				m_table->Release(m_ptr);
			}
			else
#endif
			{
				CFRelease(m_ptr);
			}
		}
    }

	template<typename T, CallingConvention C>
    Object<T, C>& Object<T, C>::operator=(const Object& rhs)
    {
#if MTLPP_CONFIG_IMP_CACHE
        if (rhs.m_ptr == m_ptr && rhs.m_table == m_table)
#else
        if (rhs.m_ptr == m_ptr)
#endif
            return *this;
		
		if (Mode != ns::Ownership::AutoRelease && rhs.m_ptr)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (rhs.m_table)
			{
				rhs.m_table->Retain(rhs.m_ptr);
			}
			else
#endif
			{
				CFRetain(rhs.m_ptr);
			}
		}
        if (Mode != ns::Ownership::AutoRelease && m_ptr)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (C == CallingConvention::C || m_table)
			{
				assert(m_table);
				m_table->Release(m_ptr);
			}
			else
#endif
			{
				CFRelease(m_ptr);
			}
		}
        m_ptr = rhs.m_ptr;
#if MTLPP_CONFIG_IMP_CACHE
		m_table = rhs.m_table;
		if (C == CallingConvention::C && m_ptr && !m_table)
		{
			m_table = UE::CreateIMPTable(m_ptr);
		}
#endif
        return *this;
    }

#if MTLPP_CONFIG_RVALUE_REFERENCES
	template<typename T, CallingConvention C>
    Object<T, C>& Object<T, C>::operator=(Object&& rhs)
    {
#if MTLPP_CONFIG_IMP_CACHE
        if (rhs.m_ptr == m_ptr && rhs.m_table == m_table)
#else
		if (rhs.m_ptr == m_ptr)
#endif
            return *this;
		
		if (rhs.Mode == ns::Ownership::AutoRelease && Mode != ns::Ownership::AutoRelease && rhs.m_ptr)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (rhs.m_table)
			{
				rhs.m_table->Retain(rhs.m_ptr);
			}
			else
#endif
			{
				CFRetain(rhs.m_ptr);
			}
		}
		
        if (Mode != ns::Ownership::AutoRelease && m_ptr)
		{
#if MTLPP_CONFIG_IMP_CACHE
			if (C == CallingConvention::C || m_table)
			{
				assert(m_table);
				m_table->Release(m_ptr);
			}
			else
#endif
			{
				CFRelease(m_ptr);
			}
		}
        m_ptr = rhs.m_ptr;
#if MTLPP_CONFIG_IMP_CACHE
		m_table = rhs.m_table;
		if (C == CallingConvention::C && m_ptr && !m_table)
		{
			m_table = UE::CreateIMPTable(m_ptr);
		}
#endif
		if (Mode != ns::Ownership::AutoRelease || rhs.Mode == ns::Ownership::AutoRelease)
		{
			rhs.m_ptr = nullptr;
#if MTLPP_CONFIG_IMP_CACHE
			rhs.m_table = nullptr;
#endif
		}
        return *this;
    }
#endif
	
	template<typename T>
	AutoReleased<T>::AutoReleased(typename T::Type handle, typename T::ITable* Table) : T(ns::Ownership::AutoRelease)
	{
		T::m_ptr = handle;
#if MTLPP_CONFIG_IMP_CACHE
		if (T::Convention == CallingConvention::C || Table)
		{
			T::m_table = handle && !Table ? UE::CreateIMPTable(handle) : Table;
		}
		else
		{
			T::m_table = nullptr;
		}
#endif
	}
	
	template<typename T>
	AutoReleased<T>& AutoReleased<T>::operator=(typename T::Type handle)
	{
		if (T::m_ptr != handle)
		{
			T::m_ptr = handle;
#if MTLPP_CONFIG_IMP_CACHE
			if (T::Convention == CallingConvention::C)
			{
				T::m_table = handle ? UE::CreateIMPTable(handle) : nullptr;
			}
			else
			{
				T::m_table = nullptr;
			}
#endif
		}
		return *this;
	}
}
