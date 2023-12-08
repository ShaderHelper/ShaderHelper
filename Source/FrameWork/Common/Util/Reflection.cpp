#include "CommonHeader.h"
#include "Reflection.h"

namespace FRAMEWORK::ShReflectToy
{
	TMap<FString, MetaType*> TypeNameToMetaType;
	TMap<FString, MetaType*> RegisteredMetaTypes;
}