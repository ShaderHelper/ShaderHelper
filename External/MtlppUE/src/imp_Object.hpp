// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_Object_hpp
#define imp_Object_hpp

#include "imp_SelectorCache.hpp"

MTLPP_BEGIN

template<typename ObjC>
struct MTLPP_EXPORT IMPTableBase
{
	IMPTableBase()
	{
	}
	
	IMPTableBase(Class C)
	: Retain(C)
	, Release(C)
	, Dealloc(C)
	{
	}
	
	template<typename AssociatedObject>
	static AssociatedObject GetAssociatedObject(ObjC Object)
	{
		return (AssociatedObject)objc_getAssociatedObject((id)Object, (void const*)&GetAssociatedObject<AssociatedObject>);
	}
	
	template<typename AssociatedObject>
	static void SetAssociatedObject(ObjC Object, AssociatedObject Assoc)
	{
		objc_setAssociatedObject((id)Object, (void const*)&GetAssociatedObject<AssociatedObject>, (id)Assoc, (objc_AssociationPolicy)(01401));
	}
	
	template<typename InterposeClass>
	void RegisterInterpose(Class C)
	{
		INTERPOSE_REGISTRATION(Retain, C);
		INTERPOSE_REGISTRATION(Release, C);
		INTERPOSE_REGISTRATION(Dealloc, C);
	}

	INTERPOSE_SELECTOR(ObjC, retain, Retain, void);
	INTERPOSE_SELECTOR(ObjC, release, Release, void);
	INTERPOSE_SELECTOR(ObjC, dealloc, Dealloc, void);
};

template<typename ObjC, typename Interpose>
struct MTLPP_EXPORT IMPTable : public IMPTableBase<ObjC>
{
	IMPTable()
	{
	}
	
	IMPTable(Class C)
	: IMPTableBase<ObjC>(C)
	{
	}
	
	void RegisterInterpose(Class C)
	{
		IMPTableBase<ObjC>::template RegisterInterpose<Interpose>(C);
	}
};

namespace UE
{
	template<typename ObjC, typename Interpose>
	struct MTLPP_EXPORT ITable : public IMPTable<ObjC, Interpose>
	{
		ITable() {}
		ITable(Class Obj) : IMPTable<ObjC, Interpose>(Obj) {}
	};
}

MTLPP_END

#endif /* imp_Object_hpp */
