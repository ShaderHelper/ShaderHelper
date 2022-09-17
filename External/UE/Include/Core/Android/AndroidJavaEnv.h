// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include <jni.h>

namespace AndroidJavaEnv
{
	// Returns the java environment
	CORE_API void InitializeJavaEnv(JavaVM* VM, jint Version, jobject GlobalThis);
	CORE_API jobject GetGameActivityThis();
	CORE_API jobject GetClassLoader();
	CORE_API JNIEnv* GetJavaEnv(bool bRequireGlobalThis = true);
	CORE_API jclass FindJavaClass(const char* name);
	CORE_API jclass FindJavaClassGlobalRef(const char* name);
	CORE_API void DetachJavaEnv();
	CORE_API bool CheckJavaException();
}

// Helper class that automatically calls DeleteLocalRef on the passed-in Java object when goes out of scope
template <typename T>
class CORE_API FScopedJavaObject
{
public:
	FScopedJavaObject(JNIEnv* InEnv, const T& InObjRef) :
	Env(InEnv),
	ObjRef(InObjRef)
	{}
	
	FScopedJavaObject(FScopedJavaObject&& Other) :
	Env(Other.Env),
	ObjRef(Other.ObjRef)
	{
		Other.Env = nullptr;
		Other.ObjRef = nullptr;
	}
	
	FScopedJavaObject(const FScopedJavaObject& Other) = delete;
	FScopedJavaObject& operator=(const FScopedJavaObject& Other) = delete;
	
	~FScopedJavaObject()
	{
		if (*this)
		{
			Env->DeleteLocalRef(ObjRef);
		}
	}
	
	// Returns the underlying JNI pointer
	T operator*() const { return ObjRef; }
	
	operator bool() const
	{
		if (!Env || !ObjRef || Env->IsSameObject(ObjRef, NULL))
		{
			return false;
		}
		
		return true;
	}
	
private:
	JNIEnv* Env = nullptr;
	T ObjRef = nullptr;
};

/**
 Helper function that allows template deduction on the java object type, for example:
 auto ScopeObject = NewScopedJavaObject(Env, JavaString);
 instead of FScopedJavaObject<jstring> ScopeObject(Env, JavaString);
 */
template <typename T>
CORE_API FScopedJavaObject<T> NewScopedJavaObject(JNIEnv* InEnv, const T& InObjRef)
{
	return FScopedJavaObject<T>(InEnv, InObjRef);
}

class CORE_API FJavaHelper
{
public:
	// Converts the java string to FString and calls DeleteLocalRef on the passed-in java string reference
	static FString FStringFromLocalRef(JNIEnv* Env, jstring JavaString);
	
	// Converts the java string to FString and calls DeleteGlobalRef on the passed-in java string reference
	static FString FStringFromGlobalRef(JNIEnv* Env, jstring JavaString);
	
	// Converts the java string to FString, does NOT modify the passed-in java string reference
	static FString FStringFromParam(JNIEnv* Env, jstring JavaString);
	
	// Converts FString into a Java string wrapped in FScopedJavaObject
	static FScopedJavaObject<jstring> ToJavaString(JNIEnv* Env, const FString& UnrealString);
};
