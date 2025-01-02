#include "CommonHeader.h"
#include "Reflection.h"

namespace FW
{
	
	TMap<FString, MetaType*>& GetTypeNameToMetaType()
	{
		static TMap<FString, MetaType*> TypeNameToMetaType;
		return TypeNameToMetaType;
	}

	TMap<FString, MetaType*>& GetRegisteredNameToMetaType()
	{
		static TMap<FString, MetaType*> RegisteredNameToMetaType;
		return RegisteredNameToMetaType;
	}

}
