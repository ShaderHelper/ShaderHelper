/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#include "texture.hpp"

MTLPP_BEGIN

namespace mtlpp
{
	template<typename T>
    RenderPassAttachmentDescriptor<T>::RenderPassAttachmentDescriptor()
    {
    }

	template<typename T>
    ns::AutoReleased<Texture> RenderPassAttachmentDescriptor<T>::GetTexture() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
		return ns::AutoReleased<Texture>(Texture(this->m_table->texture(this->m_ptr), nullptr, ns::Ownership::AutoRelease));
#else
        return ns::AutoReleased<Texture>(Texture([this->m_ptr texture], nullptr, ns::Ownership::AutoRelease));
#endif
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetLevel() const
    {
      MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(this->m_table->level(this->m_ptr));
#else
        return NSUInteger([this->m_ptr level]);
#endif
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetSlice() const
    {
      MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(this->m_table->slice(this->m_ptr));
#else
        return NSUInteger([this->m_ptr slice]);
#endif
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetDepthPlane() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(this->m_table->depthPlane(this->m_ptr));
#else
        return NSUInteger([this->m_ptr depthPlane]);
#endif
    }

	template<typename T>
    ns::AutoReleased<Texture> RenderPassAttachmentDescriptor<T>::GetResolveTexture() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return ns::AutoReleased<Texture>(this->m_table->resolveTexture(this->m_ptr));
#else
        return ns::AutoReleased<Texture>([this->m_ptr resolveTexture]);
#endif
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetResolveLevel() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(this->m_table->resolveLevel(this->m_ptr));
#else
        return NSUInteger([this->m_ptr resolveLevel]);
#endif
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetResolveSlice() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(this->m_table->resolveSlice(this->m_ptr));
#else
        return NSUInteger([this->m_ptr resolveSlice]);
#endif
    }

	template<typename T>
    NSUInteger RenderPassAttachmentDescriptor<T>::GetResolveDepthPlane() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return NSUInteger(this->m_table->resolveDepthPlane(this->m_ptr));
#else
        return NSUInteger([this->m_ptr resolveDepthPlane]);
#endif
    }

	template<typename T>
    LoadAction RenderPassAttachmentDescriptor<T>::GetLoadAction() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return LoadAction(this->m_table->loadAction(this->m_ptr));
#else
        return LoadAction([this->m_ptr loadAction]);
#endif
    }

	template<typename T>
    StoreAction RenderPassAttachmentDescriptor<T>::GetStoreAction() const
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        return StoreAction(this->m_table->storeAction(this->m_ptr));
#else
        return StoreAction([this->m_ptr storeAction]);
#endif
    }
	
	template<typename T>
	StoreActionOptions RenderPassAttachmentDescriptor<T>::GetStoreActionOptions() const
	{
		MTLPP_VALIDATION(this->Validate());
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		return StoreActionOptions(this->m_table->storeActionOptions(this->m_ptr));
#else
		return StoreActionOptions([this->m_ptr storeActionOptions]);
#endif
#else
		return (StoreActionOptions)0;
#endif
	}

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetTexture(const Texture& texture)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setTexture(this->m_ptr, texture.GetPtr());
#else
        [this->m_ptr setTexture:(id<MTLTexture>)texture.GetPtr()];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetLevel(NSUInteger level)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setLevel(this->m_ptr, level);
#else
        [this->m_ptr setLevel:level];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetSlice(NSUInteger slice)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setSlice(this->m_ptr, slice);
#else
        [this->m_ptr setSlice:slice];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetDepthPlane(NSUInteger depthPlane)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setDepthPlane(this->m_ptr, depthPlane);
#else
        [this->m_ptr setDepthPlane:depthPlane];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveTexture(const Texture& texture)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setResolveTexture(this->m_ptr, texture.GetPtr());
#else
        [this->m_ptr setResolveTexture:(id<MTLTexture>)texture.GetPtr()];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveLevel(NSUInteger resolveLevel)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setResolveLevel(this->m_ptr, resolveLevel);
#else
        [this->m_ptr setResolveLevel:resolveLevel];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveSlice(NSUInteger resolveSlice)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setResolveSlice(this->m_ptr, resolveSlice);
#else
        [this->m_ptr setResolveSlice:resolveSlice];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetResolveDepthPlane(NSUInteger resolveDepthPlane)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setResolveDepthPlane(this->m_ptr, resolveDepthPlane);
#else
        [this->m_ptr setResolveDepthPlane:resolveDepthPlane];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetLoadAction(LoadAction loadAction)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setLoadAction(this->m_ptr, MTLLoadAction(loadAction));
#else
        [this->m_ptr setLoadAction:MTLLoadAction(loadAction)];
#endif
    }

	template<typename T>
    void RenderPassAttachmentDescriptor<T>::SetStoreAction(StoreAction storeAction)
    {
        MTLPP_VALIDATION(this->Validate());
#if MTLPP_CONFIG_IMP_CACHE
        this->m_table->setStoreAction(this->m_ptr, MTLStoreAction(storeAction));
#else
        [this->m_ptr setStoreAction:MTLStoreAction(storeAction)];
#endif
    }
	
	template<typename T>
	void RenderPassAttachmentDescriptor<T>::SetStoreActionOptions(StoreActionOptions options)
	{
		MTLPP_VALIDATION(this->Validate());
#if MTLPP_IS_AVAILABLE(10_13, 11_0)
#if MTLPP_CONFIG_IMP_CACHE
		this->m_table->setStoreActionOptions(this->m_ptr, MTLStoreActionOptions(options));
#else
		[this->m_ptr setStoreActionOptions:(MTLStoreActionOptions)options];
#endif
#endif
	}
}

MTLPP_END
