/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	struct Origin : public MTLPPOrigin
    {
		inline Origin() {}
		inline Origin(MTLPPOrigin const& Other) : MTLPPOrigin(Other) {}
        inline Origin(NSUInteger X, NSUInteger Y, NSUInteger Z) { x = X; y = Y; z = Z; }
    };

    struct Size  : public MTLPPSize
    {
		inline Size() {}
		inline Size(MTLPPSize const& Other) : MTLPPSize(Other) {}
		inline Size(NSUInteger Width, NSUInteger Height, NSUInteger Depth) { width = Width; height = Height; depth = Depth; }
    };

	struct Region : public MTLPPRegion
    {
		inline Region() {}
		inline Region(MTLPPRegion const& Other) : MTLPPRegion(Other) {}
        inline Region(NSUInteger x, NSUInteger width) { origin = {x, 0, 0}; size = {width, 1, 1}; }

        inline Region(NSUInteger x, NSUInteger y, NSUInteger width, NSUInteger height) { origin = {x, y, 0}; size = {width, height, 1}; }

        inline Region(NSUInteger x, NSUInteger y, NSUInteger z, NSUInteger width, NSUInteger height, NSUInteger depth) { origin = {x, y, z}; size = {width, height, depth}; }
    };
	
	struct SamplePosition : public MTLPPSamplePosition
	{
		inline SamplePosition(MTLPPSamplePosition const& Other) : MTLPPSamplePosition(Other) {}
		inline SamplePosition(float a, float b) { x = a; y = b; }
	} MTLPP_AVAILABLE(10_13, 11_0);
}

MTLPP_END
