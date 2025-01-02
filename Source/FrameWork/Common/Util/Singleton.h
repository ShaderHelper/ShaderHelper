#pragma once
#include <Misc/LazySingleton.h>
#include "Auxiliary.h"

//Expand TLazySingleton implemented by UE to make it work across modules.
//We will get the same instance when calling TSingleton<T>::Get() from any modules.
//TSingleton<T>::Get() keeps Thread-safe.

namespace FW
{
namespace PRIVATE
{
    template<typename T>
    struct CrossModuleWrapper {
        using Type = T;
    };
}
    template<typename T>
    using TSingleton = TLazySingleton<PRIVATE::CrossModuleWrapper<T>>;

    FRAMEWORK_API TMap<FString,FLazySingleton*>& GetSharedInstanceMap(const FString& TypeName);
    FRAMEWORK_API FCriticalSection& GetSingleTonCS();
}

template<typename T>
class TLazySingleton<FW::PRIVATE::CrossModuleWrapper<T>> final : public FLazySingleton
{
public:
    static T& Get()
    {
        return GetLazy(Construct<T>).GetValue();
    }

    static void TearDown()
    {
        return GetLazy(nullptr).Reset();
    }

    static T* TryGet()
    {
        return GetLazy(Construct<T>).TryGetValue();
    }

private:
    static TLazySingleton& GetLazy(void(*Constructor)(void*))
    {
        auto& SingletonCS = FW::GetSingleTonCS();
        FString TypeName = FW::AUX::TypeName<T>;
        auto& SharedInstanceMap = FW::GetSharedInstanceMap(TypeName);
        
        FScopeLock ScopeLock(&SingletonCS);
        FLazySingleton** SingleTon = SharedInstanceMap.Find(TypeName);
        if(!SingleTon) {
            static TLazySingleton StaticSingleton(Constructor);
            SharedInstanceMap.Add(TypeName,&StaticSingleton);
            return StaticSingleton;
        }
        return *static_cast<TLazySingleton*>(*SingleTon);
    }

    alignas(T) unsigned char Data[sizeof(T)];
    T* Ptr;

    TLazySingleton(void(*Constructor)(void*))
    {
        if (Constructor)
        {
            Constructor(Data);
        }

        Ptr = Constructor ? (T*)Data : nullptr;
    }

    T* TryGetValue()
    {
        return Ptr;
    }

    T& GetValue()
    {
        check(Ptr);
        return *Ptr;
    }

    void Reset()
    {
        if (Ptr)
        {
            Destruct(Ptr);
            Ptr = nullptr;
        }
    }
};
