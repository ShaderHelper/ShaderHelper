/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once


#include "declare.hpp"
#include "imp_Object.hpp"

MTLPP_BEGIN

namespace ns
{
#if __OBJC__
	template<typename T>
	struct MTLPP_EXPORT Protocol
	{
		typedef T type;
	};
#else
	template<typename t>
	struct MTLPP_EXPORT id
	{
		typedef t* ptr;
	};
	
	template<typename T>
	struct MTLPP_EXPORT Protocol
	{
		typedef typename T::ptr type;
	};
#endif
	
	enum class Ownership
	{
		/** Handle ownership is transfered and this object manages lifetime. Handle is assigned in the constructor and released in the destructor. */
		Assign = 0,
		/** Handle is owned and this object manages lifetime. Handle is retained in the constructor and released in the destructor. */
		Retain = 1,
		/** Handle is not owned, lifetime is externally managed. Handle is assigned in the constructor and ignored in the destructor. */
		AutoRelease = 2,
	};
	
	enum class CallingConvention
	{
		/** Invoke the Objective-C selector directly. */
		ObjectiveC = 0,
		/** Assert that an ITable is valid and invoke the underlying C-function. */
		C = 1,
		/** Mixed mode - will prefer an ITable if one is present but will fallback to Objective-C when there isn't. */
		Mixed = 2
	};
	
	template<typename T, CallingConvention C = CallingConvention::C>
	class MTLPP_EXPORT Object
    {
    public:
		typedef T Type;
		typedef UE::ITable<T, void> ITable;
		static constexpr CallingConvention Convention = C;
		
        inline const T GetPtr() const { return m_ptr; }
		inline T* GetInnerPtr() { return &m_ptr; }
		
#if MTLPP_CONFIG_IMP_CACHE
		inline ITable* GetTable() { return m_table; }
#else
		inline ITable* GetTable() { return nullptr; }
#endif

        inline operator bool() const { return m_ptr != nullptr; }
		operator T() const { return m_ptr; }

        Object(ns::Ownership const retain = ns::Ownership::Retain);
        Object(T const handle, ns::Ownership const retain = ns::Ownership::Retain, ITable* table = nullptr);
        Object(const Object& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
        Object(Object&& rhs);
#endif
        virtual ~Object();

        Object& operator=(const Object& rhs);
#if MTLPP_CONFIG_RVALUE_REFERENCES
        Object& operator=(Object&& rhs);
#endif

		template<typename AssociatedObject>
		AssociatedObject GetAssociatedObject(void const* Key) const
		{
			@autoreleasepool
			{
				typename AssociatedObject::Type Val = (typename AssociatedObject::Type)objc_getAssociatedObject((id)GetPtr(), Key);
				return AssociatedObject(Val);
			}
		}
		
		template<typename AssociatedObject>
		void SetAssociatedObject(void const* Key, AssociatedObject const& Assoc)
		{
			objc_setAssociatedObject((id)GetPtr(), Key, (id)Assoc.GetPtr(), (objc_AssociationPolicy)(01401));
		}
		
		void ClearAssociatedObject(void const* Key)
		{
			objc_setAssociatedObject((id)GetPtr(), Key, (id)nullptr, (objc_AssociationPolicy)(01401));
		}
		
        inline void Validate() const
        {
#if MTLPP_CONFIG_VALIDATE
			assert(m_ptr);
#if MTLPP_CONFIG_IMP_CACHE
			assert(C != CallingConvention::C || m_table);
#endif
#endif
        }

	protected:
        T m_ptr = nullptr;
#if MTLPP_CONFIG_IMP_CACHE
		ITable* m_table = nullptr;
#endif
		ns::Ownership Mode = ns::Ownership::Retain;
    };
	
	/**
	 * Auto-released classes are used to hold Objective-C that are retained by a parent or for out-results that are implicitly placed in an auto-release pool
	 * For externally owned objects we can avoid retain/release pairs to increase our CPU efficiency dramatically.
	 * For the out-results can't retain these objects safely unless done explicitly after the function that assigns them returns (or is called for handlers).
	 * So we need a variant of the class that doesn't retain on copy/assign but that can be assigned from to our normal 'retained' class.
	 */
	template<typename T>
	class MTLPP_EXPORT AutoReleased : public T
	{
	public:
		AutoReleased() : T(ns::Ownership::AutoRelease) {}
		explicit AutoReleased(typename T::Type handle, typename T::ITable* Table = nullptr);
		explicit AutoReleased(T const& other) : T(ns::Ownership::AutoRelease) { operator=(other); }
		AutoReleased(AutoReleased const& other) : T(ns::Ownership::AutoRelease) { operator=(other); }
		AutoReleased& operator=(T const& other)
		{
			if (this != & other)
			{
				T::operator=(other);
			}
			return *this;
		}
		AutoReleased& operator=(AutoReleased const& other)
		{
			if (this != & other)
			{
				T::operator=(other);
			}
			return *this;
		}
		
		AutoReleased& operator=(typename T::Type handle);

#if MTLPP_CONFIG_RVALUE_REFERENCES
		AutoReleased(AutoReleased&& other) : T(ns::Ownership::AutoRelease) { operator=(other); }
		AutoReleased& operator=(AutoReleased&& other) { T::operator=((T&&)other); return *this; }
		explicit AutoReleased(T&& other) : T(ns::Ownership::AutoRelease) { operator=(other); }
		AutoReleased& operator=(T&& other) { T::operator=((T const&)other); return *this; }
#endif
	};

    struct MTLPP_EXPORT Range
    {
		inline Range() :
		Location(0),
		Length(0)
		{ }
		
        inline Range(NSUInteger location, NSUInteger length) :
            Location(location),
            Length(length)
        { }

        NSUInteger Location;
        NSUInteger Length;
    };

	class MTLPP_EXPORT ArrayBase
    {
    public:
        static NSUInteger GetSize(NSArray<id<NSObject>>* const handle);
        static void* GetItem(NSArray<id<NSObject>>* const handle, NSUInteger index);
		static bool EqualToArray(NSArray<id<NSObject>>* const Left, NSArray<id<NSObject>>* const Right);
    };

    template<typename T>
    class MTLPP_EXPORT Array : public Object<NSArray<typename T::Type>*, CallingConvention::ObjectiveC>
    {
    public:
		Array(ns::Ownership const retain = ns::Ownership::Retain) : Object<NSArray<typename T::Type>*, CallingConvention::ObjectiveC>(retain) { }
		Array(NSArray<typename T::Type>* const handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<NSArray<typename T::Type>*, CallingConvention::ObjectiveC>(handle, retain) { }

        const AutoReleased<T> operator[](NSUInteger index) const
        {
#if MTLPP_CONFIG_VALIDATE
			this->Validate();
#endif
			typedef typename T::Type InnerType;
			return AutoReleased<T>((InnerType)ArrayBase::GetItem((NSArray<id<NSObject>> *)this->m_ptr, index));
        }

        AutoReleased<T> operator[](NSUInteger index)
        {
#if MTLPP_CONFIG_VALIDATE
			this->Validate();
#endif
			typedef typename T::Type InnerType;
			return AutoReleased<T>((InnerType)ArrayBase::GetItem((NSArray<id<NSObject>> *)this->m_ptr, index));
        }
		
		NSUInteger GetSize() const
		{
#if MTLPP_CONFIG_VALIDATE
			this->Validate();
#endif
			return ArrayBase::GetSize((NSArray<id<NSObject>> *)this->m_ptr);
		}
		
		class MTLPP_EXPORT Iterator
		{
		public:
			Iterator(Array const& ptr, NSUInteger In): array(ptr), index(In) {}
			Iterator operator++() { if (index < array.GetSize()) { ++index; } return *this; }
			
			bool operator!=(Iterator const& other) const { return (&array != &other.array) || (index != other.index); }
			
			const AutoReleased<T> operator*() const
			{
				if (index < array.GetSize())
					return array[index];
				else
					return AutoReleased<T>();
			}
		private:
			Array const& array;
			NSUInteger index;
		};
		
		Iterator begin() const
		{
			return Iterator(*this, 0);
		}
		
		Iterator end() const
		{
			return Iterator(*this, GetSize());
		}
		
		bool operator==(Array const& Other) const
		{
			return (this->m_ptr == Other.m_ptr || ArrayBase::EqualToArray(this->m_ptr, Other.m_ptr));
		}
    };

//    class MTLPP_EXPORT DictionaryBase : public Object<NSDictionary<id<NSObject>, id<NSObject>>*>
//    {
//    public:
//		DictionaryBase(ns::Ownership const retain = ns::Ownership::Retain) : Object<NSDictionary<id<NSObject>, id<NSObject>>*>(retain) { }
//        DictionaryBase(NSDictionary<id<NSObject>, id<NSObject>>* const handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<NSDictionary<id<NSObject>, id<NSObject>>*>(handle, retain) { }
//
//    protected:
//
//    };

    template<typename KeyT, typename ValueT>
    class MTLPP_EXPORT Dictionary : public Object<NSDictionary*>/*: public DictionaryBase*/
    {
    public:
        Dictionary(ns::Ownership const retain = ns::Ownership::Retain) /*: DictionaryBase(retain) */{ }
        Dictionary(NSDictionary<typename KeyT::Type, typename ValueT::Type>* const handle) /*: DictionaryBase(handle) */ { }
    };

    class MTLPP_EXPORT String : public Object<NSString*, CallingConvention::ObjectiveC>
    {
    public:
		String(ns::Ownership const retain = ns::Ownership::Retain) : Object<NSString*, CallingConvention::ObjectiveC>(retain) { }
        String(NSString* handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<NSString*, CallingConvention::ObjectiveC>(handle, retain) { }
        String(const char* cstr);

        const char* GetCStr() const;
        NSUInteger    GetLength() const;
    };
	
	class MTLPP_EXPORT URL : public Object<NSURL*>
	{
	public:
		URL(ns::Ownership const retain = ns::Ownership::Retain) : Object<NSURL*>(retain) { }
		URL(NSURL* const handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<NSURL*>(handle, retain) { }
	};

	class MTLPP_EXPORT Error : public Object<NSError*>
	{
	public:
		Error(ns::Ownership const retain = ns::Ownership::Retain) : Object<NSError*>(retain) {}
		Error(NSError* const handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<NSError*>(handle, retain) { }
		
		AutoReleased<String>   GetDomain() const;
		NSUInteger GetCode() const;
		//@property (readonly, copy) NSDictionary *userInfo;
		AutoReleased<String>   GetLocalizedDescription() const;
		AutoReleased<String>   GetLocalizedFailureReason() const;
		AutoReleased<String>   GetLocalizedRecoverySuggestion() const;
		AutoReleased<Array<String>>   GetLocalizedRecoveryOptions() const;
		//@property (nullable, readonly, strong) id recoveryAttempter;
		AutoReleased<String>   GetHelpAnchor() const;
	};
	typedef AutoReleased<Error> AutoReleasedError;
	
	class MTLPP_EXPORT IOSurface : public Object<IOSurfaceRef>
	{
	public:
		IOSurface(ns::Ownership const retain = ns::Ownership::Retain) : Object<IOSurfaceRef>(retain) {}
		IOSurface(IOSurfaceRef const handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<IOSurfaceRef>(handle, retain) { }
	};
	
	class MTLPP_EXPORT Bundle : public Object<NSBundle*>
	{
	public:
		Bundle(ns::Ownership const retain = ns::Ownership::Retain) : Object<NSBundle*>(retain) {}
		Bundle(NSBundle* const handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<NSBundle*>(handle, retain) { }
	};
	
	class MTLPP_EXPORT Condition : public Object<NSCondition*, CallingConvention::ObjectiveC>
	{
	public:
		Condition(ns::Ownership const retain = ns::Ownership::Retain) : Object<NSCondition*, CallingConvention::ObjectiveC>(retain) {}
		Condition(NSCondition* const handle, ns::Ownership const retain = ns::Ownership::Retain) : Object<NSCondition*, CallingConvention::ObjectiveC>(handle, retain) { }
	};
}

#include "ns.inl"

MTLPP_END
