#pragma once
#include "Auxiliary.h"

namespace FRAMEWORK::ShReflectToy
{
	struct MetaType;

	FRAMEWORK_API extern TMap<FString, MetaType*> TypeNameToMetaType;
	FRAMEWORK_API extern TMap<FString, MetaType*> RegisteredMetaTypes;

	struct MetaType
	{
		void* GetDefaultObject()
		{
			return DefaultObject;
		}

		MetaType* GetBaseClass()
		{
			if (BaseMetaTypeGetter)
			{
				return BaseMetaTypeGetter();
			}
			return nullptr;
		}

		template<typename T>
		bool IsDerivedFrom()
		{
			MetaType* BaseMetaType = GetBaseClass();
			while(BaseMetaType)
			{
				if (BaseMetaType->TypeName == AUX::TypeName<T>)
				{
					return true;
				}
				BaseMetaType = BaseMetaType->GetBaseClass();
			} 
			return false;
		}

		FString TypeName;
		FString RegisteredName;
		void* DefaultObject;
		TFunction<MetaType*()> BaseMetaTypeGetter;
	};

	template<typename T>
	class MetaTypeBuilder
	{
	public:
		MetaTypeBuilder()
		{
			Meta = new MetaType{};
			Meta->TypeName = AUX::TypeName<T>;
			Meta->RegisteredName = Meta->TypeName;
			if constexpr(std::is_default_constructible_v<T>)
			{
				Meta->DefaultObject = new T;
			}
		}

		MetaTypeBuilder(const FString& InRegisteredName)
			: MetaTypeBuilder()
		{
			Meta->RegisteredName = InRegisteredName;
		}

		template<typename Base>
		MetaTypeBuilder& BaseClass()
		{
			Meta->BaseMetaTypeGetter = [] {
				return TypeNameToMetaType[AUX::TypeName<Base>];
			};
			return *this;
		}

		~MetaTypeBuilder()
		{
			TypeNameToMetaType.Add(Meta->TypeName, Meta);
			RegisteredMetaTypes.Add(Meta->RegisteredName, Meta);
		}

	private:
		MetaType* Meta;
	};

	template<typename T>
	MetaTypeBuilder<T> AddClass(const FString& InName)
	{
		return MetaTypeBuilder<T>{InName};
	}

	template<typename T>
	MetaTypeBuilder<T> AddClass()
	{
		return MetaTypeBuilder<T>{};
	}

	template<typename T>
	MetaType* GetMetaType()
	{
		return TypeNameToMetaType[AUX::TypeName<T>];
	}

	template<typename T>
	TArray<MetaType*> GetChildMetaTypes()
	{
		TArray<MetaType*> MetaTypes;
		for (auto [_, MetaTypeUniquePtr] : RegisteredMetaTypes)
		{
			if (MetaTypeUniquePtr->IsDerivedFrom<T>)
			{
				MetaTypes.Add(MetaTypeUniquePtr.Get());
			}
		}
		return MetaTypes;
	}

#define GLOBAL_REFLECTION_REGISTER(...)	\
	static const int PREPROCESSOR_JOIN(ReflectionGlobalRegister_,__COUNTER__) = [] { __VA_ARGS__; return 0; }();

}