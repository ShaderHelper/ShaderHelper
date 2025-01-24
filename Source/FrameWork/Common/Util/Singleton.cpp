#include "CommonHeader.h"
#include "Singleton.h"

namespace FW
{
    TMap<FString, FLazySingleton*>& GetSharedInstanceMap(const FString& TypeName) {
        static TMap<FString, FLazySingleton*> SharedInstanceMap;
        return  SharedInstanceMap;
    }

    FCriticalSection& GetSingleTonCS() {
        static FCriticalSection SingleTonCS;
        return SingleTonCS;
    }
}
