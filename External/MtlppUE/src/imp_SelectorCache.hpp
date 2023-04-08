// Copyright Epic Games, Inc. All Rights Reserved.

#ifndef imp_SelectorCache_hpp
#define imp_SelectorCache_hpp

#include "declare.hpp"
#include <assert.h>
#include <thread>
#include <unordered_map>

MTLPP_BEGIN

template<typename ObjClass, typename Return, typename... Params>
struct SelectorType
{
	typedef Return (*DefinedIMP)(id,SEL,Params...);
};

template<typename ObjClass, typename... Params>
struct SelectorType<ObjClass, void, Params...>
{
	typedef void (*DefinedIMP)(id,SEL,Params...);
};

template<typename ObjClass, typename Return, typename... Params>
class SelectorCache
{
public:
	typedef typename SelectorType<ObjClass, Return, Params...>::DefinedIMP DefinedIMP;
	
	inline SelectorCache()
	: ClassPtr(nullptr)
	, Implementation(nullptr)
	, Selector(nullptr)
	{
	}

	inline SelectorCache(Class ObjcClass, SEL Select)
	: ClassPtr(ObjcClass)
	, Implementation(nullptr)
	, Selector(Select)
	{
		Implementation = (DefinedIMP)class_getMethodImplementation(ObjcClass, Selector);
	}
	
	inline Return operator()(ObjClass Object, Params... Parameters) const
	{
		return Implementation(Object, Selector, Parameters...);
	}
	
	static inline DefinedIMP Get(SEL Selector, ObjClass Object)
	{
		assert(Object && Selector);
		DefinedIMP Original = (DefinedIMP)class_getMethodImplementation(object_getClass(Object), Selector);
		assert(Original);
		return Original;
	}
	
protected:
	Class ClassPtr;
	DefinedIMP Implementation;
	SEL Selector;
};

template<typename ObjClass, typename... Params>
class SelectorCache<ObjClass, void, Params...>
{
public:
	typedef typename SelectorType<ObjClass, void, Params...>::DefinedIMP DefinedIMP;
	
	inline SelectorCache()
	: ClassPtr(nullptr)
	, Implementation(nullptr)
	, Selector(nullptr)
	{
	}
	
	inline SelectorCache(Class ObjcClass, SEL Select)
	: ClassPtr(ObjcClass)
	, Implementation(nullptr)
	, Selector(Select)
	{
		Implementation = (DefinedIMP)class_getMethodImplementation(ObjcClass, Selector);
	}
	
	inline void operator()(ObjClass Object, Params... Parameters) const
	{
		Implementation(Object, Selector, Parameters...);
	}
	
	static inline DefinedIMP Get(SEL Selector, ObjClass Object)
	{
		assert(Object && Selector);
		DefinedIMP Original = (DefinedIMP)class_getMethodImplementation(object_getClass(Object), Selector);
		assert(Original);
		return Original;
	}
	
protected:
	Class ClassPtr;
	DefinedIMP Implementation;
	SEL Selector;
};

extern void RegisterSwizzleSelector(Class ObjcClass, SEL Select, SEL& Output);

template<typename ObjClass, SEL (*Original)(), typename Return, typename... Params>
class InterposeSelector : public SelectorCache<ObjClass, Return, Params...>
{
	static SEL kSelector;
public:
	typedef typename SelectorType<ObjClass, Return, Params...>::DefinedIMP DefinedIMP;
	typedef typename SelectorType<ObjClass, Return, DefinedIMP, Params...>::DefinedIMP ReplacementIMP;
	
	static ReplacementIMP Replacement;
	
	inline InterposeSelector()
	{
	}
	
	inline InterposeSelector(Class ObjcClass)
	: SelectorCache<ObjClass, Return, Params...>(ObjcClass, Original())
	{
	}
	
	inline void Swizzle(SEL SwizzleSelector, DefinedIMP Replace)
	{
		assert(SwizzleSelector);
		assert(Replace);
		Method originalMethod = class_getInstanceMethod(SelectorCache<ObjClass, Return, Params...>::ClassPtr, SelectorCache<ObjClass, Return, Params...>::Selector);
		class_addMethod(SelectorCache<ObjClass, Return, Params...>::ClassPtr, SwizzleSelector, (IMP)SelectorCache<ObjClass, Return, Params...>::Implementation, method_getTypeEncoding(originalMethod));
		class_replaceMethod(SelectorCache<ObjClass, Return, Params...>::ClassPtr, SelectorCache<ObjClass, Return, Params...>::Selector, (IMP)Replace, method_getTypeEncoding(originalMethod));
		SelectorCache<ObjClass, Return, Params...>::Implementation = Replace;
	}
	
	inline void Register(Class ObjcClass, ReplacementIMP InReplacement)
	{
		if (SelectorCache<ObjClass, Return, Params...>::Implementation && Replacement == nullptr)
		{
			RegisterSwizzleSelector(ObjcClass, SelectorCache<ObjClass, Return, Params...>::Selector, kSelector);
			Replacement = InReplacement;
			Swizzle(kSelector, &InterposeFunc);
		}
	}
	
	static Return InterposeFunc(id Object,SEL InvokedSel,Params... Parameters)
	{
		return ((ReplacementIMP)Replacement)(Object, Original(), SelectorCache<ObjClass, Return, Params...>::Get(kSelector, Object), Parameters...);
	}
};
template<typename ObjClass, SEL (*Original)(), typename Return, typename... Params>
SEL InterposeSelector<ObjClass, Original, Return, Params...>::kSelector = nullptr;
template<typename ObjClass, SEL (*Original)(), typename Return, typename... Params>
typename InterposeSelector<ObjClass, Original, Return, Params...>::ReplacementIMP InterposeSelector<ObjClass, Original, Return, Params...>::Replacement = nullptr;

template<typename ObjClass, SEL (*Original)(), typename... Params>
class InterposeSelector<ObjClass, Original, void, Params...> : public SelectorCache<ObjClass, void, Params...>
{
	static SEL kSelector;
public:
	typedef typename SelectorType<ObjClass, void, Params...>::DefinedIMP DefinedIMP;
	typedef typename SelectorType<ObjClass, void, DefinedIMP, Params...>::DefinedIMP ReplacementIMP;
	
	static ReplacementIMP Replacement;
	
	inline InterposeSelector()
	{
	}

	inline InterposeSelector(Class ObjcClass)
	: SelectorCache<ObjClass, void, Params...>(ObjcClass, Original())
	{
	}
	
	inline void Swizzle(SEL SwizzleSelector, DefinedIMP Replace)
	{
		assert(SwizzleSelector);
		assert(Replace);
		Method originalMethod = class_getInstanceMethod(SelectorCache<ObjClass, void, Params...>::ClassPtr, SelectorCache<ObjClass, void, Params...>::Selector);
		class_addMethod(SelectorCache<ObjClass, void, Params...>::ClassPtr, SwizzleSelector, (IMP)SelectorCache<ObjClass, void, Params...>::Implementation, method_getTypeEncoding(originalMethod));
		class_replaceMethod(SelectorCache<ObjClass, void, Params...>::ClassPtr, SelectorCache<ObjClass, void, Params...>::Selector, (IMP)Replace, method_getTypeEncoding(originalMethod));
		SelectorCache<ObjClass, void, Params...>::Implementation = Replace;
	}
	
	inline void Register(Class ObjcClass, ReplacementIMP InReplacement)
	{
		if (SelectorCache<ObjClass, void, Params...>::Implementation && Replacement == nullptr)
		{
			RegisterSwizzleSelector(ObjcClass, SelectorCache<ObjClass, void, Params...>::Selector, kSelector);
			Replacement = InReplacement;
			Swizzle(kSelector, &InterposeFunc);
		}
	}
	
	static void InterposeFunc(id Object,SEL InvokedSel,Params... Parameters)
	{
		((ReplacementIMP)Replacement)(Object, Original(), SelectorCache<ObjClass, void, Params...>::Get(kSelector, Object), Parameters...);
	}
};
template<typename ObjClass, SEL (*Original)(), typename... Params>
SEL InterposeSelector<ObjClass, Original, void, Params...>::kSelector = nullptr;
template<typename ObjClass, SEL (*Original)(), typename... Params>
typename InterposeSelector<ObjClass, Original, void, Params...>::ReplacementIMP InterposeSelector<ObjClass, Original, void, Params...>::Replacement = nullptr;

#define INTERPOSE_SELECTOR(ObjCClass, Select, FuncName, ...)	\
	static SEL Get##FuncName##Selector() MTLPP_EXPORT { return @selector(Select); } \
	typedef InterposeSelector<ObjCClass, &Get##FuncName##Selector, __VA_ARGS__> FuncName##Type;	\
	MTLPP_EXPORT FuncName##Type FuncName;

#define INTERPOSE_REGISTRATION(FuncName, Class)	\
	FuncName.Register(Class, &InterposeClass::FuncName##Impl)

#define INTERPOSE_CONSTRUCTOR(FuncName, Class)	\
	FuncName(Class)

#define INTERPOSE_DECLARATION_VOID(Name, Return)	\
	static Return Name##Impl(id, SEL, Super::Name##Type::DefinedIMP)

#define INTERPOSE_DECLARATION(Name, Return, ...)	\
	static Return Name##Impl(id, SEL, Super::Name##Type::DefinedIMP, __VA_ARGS__)

#define INTERPOSE_DEFINITION_VOID(Class, Name, Return)	\
	Return Class::Name##Impl(id Obj, SEL Cmd, Super::Name##Type::DefinedIMP Original)

#define INTERPOSE_DEFINITION(Class, Name, Return, ...)	\
	Return Class::Name##Impl(id Obj, SEL Cmd, Super::Name##Type::DefinedIMP Original, __VA_ARGS__)

#define INTERPOSE_CLASS_REGISTER(CPP, ObjC)	\
	static CPP& CPP##Register(Class C)	\
	{	\
		static CPP Table = CPP(C);	\
		return Table;	\
	}	\
	@implementation ObjC (Hooks)	\
	+(void)load	\
	{	\
		static dispatch_once_t onceToken;	\
		dispatch_once(&onceToken, ^{	\
			CPP##Register([self class]);	\
		});	\
	}	\
	@end

#define INTERPOSE_PROTOCOL_REGISTER(CPP, ObjC)	\
	ObjC CPP::Register(ObjC Object)	\
	{	\
		if(Object)	\
		{	\
			static std::mutex Lock;	\
			static std::unordered_map<Class, CPP> Impls;	\
			Class c = object_getClass(Object);	\
			Lock.lock();	\
			if(!Impls.count(c)) { Impls.emplace(c, CPP(Object)); }	\
			Lock.unlock();	\
		}	\
		return Object;	\
	}

MTLPP_END

#endif /* imp_SelectorCache_hpp */
