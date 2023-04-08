/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include <Metal/MTLDrawable.h>
#include "drawable.hpp"

MTLPP_BEGIN

namespace mtlpp
{
    double Drawable::GetPresentedTime() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->PresentedTime(m_ptr);
#else
        return [(id<MTLDrawable>)m_ptr presentedTime];
#endif
#else
        return 0.0;
#endif
    }

    uint64_t Drawable::GetDrawableID() const
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
#if MTLPP_CONFIG_IMP_CACHE
		return m_table->DrawableID(m_ptr);
#else
        return [(id<MTLDrawable>)m_ptr drawableID];
#endif
#else
        return 0;
#endif
    }

    void Drawable::Present()
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->Present(m_ptr);
#else
        [(id<MTLDrawable>)m_ptr present];
#endif
    }

    void Drawable::PresentAtTime(double presentationTime)
    {
        Validate();
#if MTLPP_CONFIG_IMP_CACHE
		m_table->PresentAtTime(m_ptr, presentationTime);
#else
        [(id<MTLDrawable>)m_ptr presentAtTime:presentationTime];
#endif
    }

    void Drawable::PresentAfterMinimumDuration(double duration)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
#if MTLPP_CONFIG_IMP_CACHE
		m_table->PresentAfterMinimumDuration(m_ptr, duration);
#else
        [(id<MTLDrawable>)m_ptr presentAfterMinimumDuration:duration];
#endif
#endif
    }

    void Drawable::AddPresentedHandler(PresentHandler handler)
    {
        Validate();
#if MTLPP_IS_AVAILABLE_AX(10_3)
#if MTLPP_CONFIG_IMP_CACHE
		ITable* table = m_table;
		m_table->AddPresentedHandler(m_ptr, ^(id <MTLDrawable> mtlDrawable){
			Drawable drawable(mtlDrawable, table);
			handler(drawable);
		});
#else
        [(id<MTLDrawable>)m_ptr addPresentedHandler:^(id <MTLDrawable> mtlDrawable){
            Drawable drawable(mtlDrawable);
            handler(drawable);
        }];
#endif
#endif
    }

}

MTLPP_END
