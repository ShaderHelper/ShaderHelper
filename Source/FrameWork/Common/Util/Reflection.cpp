#include "CommonHeader.h"
#include "Reflection.h"

namespace FRAMEWORK::ShReflectToy
{
	
	TMap<FString, MetaType*>& GetTypeNameToMetaType()
	{
		static TMap<FString, MetaType*> TypeNameToMetaType;
		return TypeNameToMetaType;
	}

	TMap<FString, MetaType*>& GetRegisteredMetaTypes()
	{
		static TMap<FString, MetaType*> RegisteredMetaTypes;
		return RegisteredMetaTypes;
	}

}