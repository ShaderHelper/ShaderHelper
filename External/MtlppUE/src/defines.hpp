/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/mtlpp/blob/master/LICENSE
 */
// Copyright Epic Games, Inc. All Rights Reserved.
// Modifications for Unreal Engine

#pragma once

#include <stdint.h>
#include <assert.h>
#include <TargetConditionals.h>

#ifndef __has_feature
#   define __has_feature(x) 0
#endif

#ifndef MTLPP_CONFIG_RVALUE_REFERENCES
#   define MTLPP_CONFIG_RVALUE_REFERENCES __has_feature(cxx_rvalue_references)
#endif

#ifndef MTLPP_CONFIG_IMP_CACHE
#   define MTLPP_CONFIG_IMP_CACHE 1
#endif

#ifndef MTLPP_CONFIG_VALIDATE
#   define MTLPP_CONFIG_VALIDATE 1
#endif

#ifndef MTLPP_CONFIG_USE_AVAILABILITY
#   define MTLPP_CONFIG_USE_AVAILABILITY !(MTLPP_CONFIG_USE_SDK_AVAILABILITY) && (__has_feature(attribute_availability_with_version_underscores) || (__has_feature(attribute_availability_with_message) && __clang__ && __clang_major__ >= 7))
#endif

#ifndef MTLPP_CONFIG_USE_SDK_AVAILABILITY
#	define MTLPP_CONFIG_USE_SDK_AVAILABILITY 0
#endif

#ifndef MTLPP_PLATFORM_MAC
#define MTLPP_PLATFORM_MAC TARGET_OS_OSX
#endif

#ifndef MTLPP_PLATFORM_IOS
#define MTLPP_PLATFORM_IOS TARGET_OS_IOS
#endif

#ifndef MTLPP_PLATFORM_TVOS
#define MTLPP_PLATFORM_TVOS TARGET_OS_TV
#endif

#ifndef MTLPP_PLATFORM_AX
#define MTLPP_PLATFORM_AX (MTLPP_PLATFORM_IOS || MTLPP_PLATFORM_TVOS)
#endif

#ifndef MTLPP_EXPORT
#define MTLPP_EXPORT __attribute__ ((visibility("default")))
#endif

#if MTLPP_CONFIG_USE_AVAILABILITY
#   if __has_feature(attribute_availability_with_version_underscores) || (__has_feature(attribute_availability_with_message) && __clang__ && __clang_major__ >= 7)
#		define MTLPP_AVAILABILITY(_os, _vers) 						__attribute__((availability(_os,introduced=_vers)))
#		define MTLPP_UNAVAILABILITY(_os) 							__attribute__((availability(_os,unavailable)))
#		define MTLPPP_DEPRECATION(_os, _intro, _dep, ...) 			__attribute__((availability(_os,introduced=_intro,deprecated=_dep,message="" __VA_ARGS__)))
#       define MTLPP_UNAVAILABLE_MAC	 	                        MTLPP_UNAVAILABILITY(macosx)
#       define MTLPP_UNAVAILABLE_IOS		                        MTLPP_UNAVAILABILITY(ios)
#       define MTLPP_UNAVAILABLE_TVOS								MTLPP_UNAVAILABILITY(tvos)
#       define MTLPP_UNAVAILABLE_AX		    		                MTLPP_UNAVAILABLE_IOS MTLPP_UNAVAILABLE_TVOS
#       define MTLPP_AVAILABLE_MAC(_mac)                            MTLPP_AVAILABILITY(macosx, _mac) MTLPP_UNAVAILABLE_AX
#       define MTLPP_AVAILABLE_IOS(_ios)                            MTLPP_AVAILABILITY(ios, _ios) MTLPP_UNAVAILABLE_MAC MTLPP_UNAVAILABLE_TVOS
#       define MTLPP_AVAILABLE_TVOS(_tvos)							MTLPP_AVAILABILITY(tvos, _tvos) MTLPP_UNAVAILABLE_MAC MTLPP_UNAVAILABLE_IOS
#       define MTLPP_AVAILABLE_AX(_os)								MTLPP_AVAILABILITY(ios, _os) MTLPP_AVAILABILITY(tvos, _os) MTLPP_UNAVAILABLE_MAC
#       define MTLPP_DEPRECATED_MAC(macIntro, macDep)               MTLPPP_DEPRECATION(macosx, macIntro, macDep)
#       define MTLPP_DEPRECATED_IOS(iosIntro, iosDep)               MTLPPP_DEPRECATION(ios, iosIntro, iosDep)
#       define MTLPP_DEPRECATED_TVOS(iosIntro, iosDep)              MTLPPP_DEPRECATION(tvos, iosIntro, iosDep)
#		if MTLPP_PLATFORM_MAC
#       	define MTLPP_AVAILABLE(_mac, _ios)                      MTLPP_AVAILABLE_MAC(_mac)
# 	        define MTLPP_DEPRECATED(macIntro, macDep, iosIntro, iosDep) MTLPP_DEPRECATED_MAC(macIntro, macDep)
#		elif MTLPP_PLATFORM_IOS
#       	define MTLPP_AVAILABLE(_mac, _ios)                      MTLPP_AVAILABLE_IOS(_ios)
# 	        define MTLPP_DEPRECATED(macIntro, macDep, iosIntro, iosDep) MTLPP_DEPRECATED_IOS(iosIntro, iosDep)
#		elif MTLPP_PLATFORM_TVOS
#       	define MTLPP_AVAILABLE(_mac, _ios)                      MTLPP_AVAILABLE_TVOS(_ios)
# 	        define MTLPP_DEPRECATED(macIntro, macDep, iosIntro, iosDep) MTLPP_DEPRECATED_TVOS(iosIntro, iosDep)
#		else
#       	define MTLPP_AVAILABLE(_mac, _ios)
# 	        define MTLPP_DEPRECATED(macIntro, macDep, iosIntro, iosDep)
#		endif
#   endif
#endif

#ifndef MTLPP_AVAILABLE
#   define MTLPP_AVAILABLE(mac, ios)
#   define MTLPP_AVAILABLE_MAC(mac)
#   define MTLPP_AVAILABLE_IOS(ios)
#   define MTLPP_AVAILABLE_TVOS(tvos)
#   define MTLPP_AVAILABLE_AX(tvos)
#   define MTLPP_DEPRECATED(macIntro, macDep, iosIntro, iosDep)
#   define MTLPP_DEPRECATED_MAC(macIntro, macDep)
#   define MTLPP_DEPRECATED_IOS(iosIntro, iosDep)
#endif

#ifndef __DARWIN_ALIAS_STARTING_MAC___MAC_10_11
#   define __DARWIN_ALIAS_STARTING_MAC___MAC_10_11(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_MAC___MAC_10_12
#   define __DARWIN_ALIAS_STARTING_MAC___MAC_10_12(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_MAC___MAC_10_13
#   define __DARWIN_ALIAS_STARTING_MAC___MAC_10_13(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_MAC___MAC_10_14
#   define __DARWIN_ALIAS_STARTING_MAC___MAC_10_14(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_8_0
#   define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_8_0(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_9_0
#   define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_9_0(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_0
#   define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_0(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_3
#   define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_10_3(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_11_0
#   define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_11_0(x)
#endif
#ifndef __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_12_0
#   define __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_12_0(x)
#endif

// tvOS doesn't have versions less than 9.0
#define __TVOS_8_0      80000
#define __TVOS_8_1      80100
#define __TVOS_8_2      80200
#define __TVOS_8_3      80300

#if MTLPP_CONFIG_USE_SDK_AVAILABILITY
#	define MTLPP_IS_AVAILABLE_MAC(mac)  (0 || (defined(__MAC_##mac) && MTLPP_PLATFORM_MAC && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_##mac))
#	define MTLPP_IS_AVAILABLE_IOS(ios)  (0 || (defined(__IPHONE_##ios) && MTLPP_PLATFORM_IOS && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_##ios))
#	define MTLPP_IS_AVAILABLE_TVOS(ios) (0 || (defined(__TVOS_##ios) && MTLPP_PLATFORM_TVOS && __TV_OS_VERSION_MAX_ALLOWED >= __TVOS_##ios))
#else
#	define MTLPP_IS_AVAILABLE_MAC(mac)  (0 __DARWIN_ALIAS_STARTING_MAC___MAC_##mac( || 1 ))
#	define MTLPP_IS_AVAILABLE_IOS(ios)  (0 __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_##ios( || 1 ))
#	define MTLPP_IS_AVAILABLE_TVOS(ios) (0 __DARWIN_ALIAS_STARTING_IPHONE___IPHONE_##ios( || 1 ))
#endif
#define MTLPP_IS_AVAILABLE_AX(ios) (MTLPP_IS_AVAILABLE_IOS(ios) || MTLPP_IS_AVAILABLE_TVOS(ios))
#define MTLPP_IS_AVAILABLE(mac, ios) (MTLPP_IS_AVAILABLE_MAC(mac) || MTLPP_IS_AVAILABLE_IOS(ios) || MTLPP_IS_AVAILABLE_TVOS(ios))

#ifdef __OBJC__
#define MTLPP_CLASS(name) @class name
#define MTLPP_PROTOCOL(name) @protocol name
#define MTLPP_IF_AVAILABLE(macvers, iosvers, tvosvers) if (@available(macOS macvers, iOS iosvers, tvOS tvosvers, *))
#define MTLPP_IF_AVAILABLE_MAC(vers) if (@available(macOS vers, *))
#define MTLPP_IF_AVAILABLE_IOS(vers) if (@available(iOS vers, *))
#define MTLPP_IF_AVAILABLE_TVOS(vers) if (@available(tvOS vers, *))
#define MTLPP_IF_AVAILABLE_AX(vers) if (@available(iOS vers, tvOS vers, *))
#define MTLPP_CHECK_AVAILABLE(macvers, iosvers, tvosvers) ^{ MTLPP_IF_AVAILABLE(macvers, iosvers, tvosvers) { return true; } else { return false; } }()
#define MTLPP_CHECK_AVAILABLE_MAC(vers) ^{ MTLPP_IF_AVAILABLE_MAC(vers) { return MTLPP_PLATFORM_MAC; } else { return false; } }()
#define MTLPP_CHECK_AVAILABLE_IOS(vers) ^{ MTLPP_IF_AVAILABLE_IOS(vers) { return MTLPP_PLATFORM_IOS; } else { return false; } }()
#define MTLPP_CHECK_AVAILABLE_TVOS(vers) ^{ MTLPP_IF_AVAILABLE_TVOS(vers) { return MTLPP_PLATFORM_TVOS; } else { return false; } }()
#define MTLPP_CHECK_AVAILABLE_AX(vers) ^{ MTLPP_IF_AVAILABLE_AX(vers) { return MTLPP_PLATFORM_AX; } else { return false; } }()
#else
#define MTLPP_CLASS(name) class name
#define MTLPP_PROTOCOL(name) class name
#define MTLPP_IF_AVAILABLE(macvers, iosvers, tvosvers) if (false)
#define MTLPP_IF_AVAILABLE_MAC(vers) if (false)
#define MTLPP_IF_AVAILABLE_IOS(vers) if (false)
#define MTLPP_IF_AVAILABLE_TVOOS(vers) if (false)
#define MTLPP_IF_AVAILABLE_AX(vers) if (false)
#define MTLPP_CHECK_AVAILABLE(macvers, iosvers, tvosvers) false
#define MTLPP_CHECK_AVAILABLE_MAC(vers) false
#define MTLPP_CHECK_AVAILABLE_IOS(vers) false
#define MTLPP_CHECK_AVAILABLE_TVOS(vers) false
#define MTLPP_CHECK_AVAILABLE_AX(vers) false
#endif

#if MTLPP_CONFIG_VALIDATE
#define MTLPP_VALIDATED virtual
#define MTLPP_VALIDATE(Type, Resource, bEnable, ...) mtlpp::Validator<Type>(Resource, bEnable)->__VA_ARGS__
#define MTLPP_VALIDATE_ONLY(Type, Resource, bEnable, ...) if (bEnable) (*mtlpp::Validator<Type>(Resource, bEnable)).__VA_ARGS__
#else
#define MTLPP_VALIDATED
#define MTLPP_VALIDATE(Type, Resource, bEnable, ...) Resource.__VA_ARGS__
#define MTLPP_VALIDATE_ONLY(Type, Resource, bEnable, ...)
#endif

#if __has_extension(blocks) && (!__cplusplus || __OBJC__)
#define MTLPP_CLOSURE(Name, Return, ...) typedef Return (^Name)(__VA_ARGS__)
#else
#include <functional>
#define MTLPP_CLOSURE(Name, Return, ...) typedef std::function<Return (__VA_ARGS__)> Name
#endif

#if __clang__
#define MTLPP_BEGIN \
	_Pragma ("clang diagnostic push")	\
	_Pragma ("clang diagnostic ignored \"-Wnullability-completeness\"")	\
	_Pragma ("clang diagnostic ignored \"-Wunguarded-availability\"")	\
	_Pragma ("clang diagnostic ignored \"-Wunguarded-availability-new\"")	\
	_Pragma ("clang diagnostic ignored \"-Wundeclared-selector\"")
#define MTLPP_END \
	_Pragma ("clang diagnostic pop")
#else
#define MTLPP_BEGIN
#define MTLPP_END
#endif

#if __cplusplus
#define MTLPP_EXTERN extern "C"
#else
#define MTLPP_EXTERN extern
#endif

#if MTLPP_CONFIG_VALIDATE
#define MTLPP_VALIDATION(Code) Code
#else
#define MTLPP_VALIDATION(Code)
#endif

