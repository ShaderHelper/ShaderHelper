/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Drawable.hpp"
#include "ns.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	class Drawable;
	MTLPP_CLOSURE(PresentHandler, void, const Drawable&);
	
    class MTLPP_EXPORT Drawable : public ns::Object<ns::Protocol<id<MTLDrawable>>::type>
    {
    public:
        Drawable() { }
        Drawable(ns::Protocol<id<MTLDrawable>>::type handle, ITable* table = nullptr) : ns::Object<ns::Protocol<id<MTLDrawable>>::type>(handle, ns::Ownership::Retain, table) { }

        double   GetPresentedTime() const MTLPP_AVAILABLE_AX(10_3);
        uint64_t GetDrawableID() const MTLPP_AVAILABLE_AX(10_3);

        void Present();
        void PresentAtTime(double presentationTime);
        void PresentAfterMinimumDuration(double duration) MTLPP_AVAILABLE_AX(10_3);
        void AddPresentedHandler(PresentHandler handler) MTLPP_AVAILABLE_AX(10_3);
    }
    MTLPP_AVAILABLE(10_11, 8_0);
}

MTLPP_END
