#pragma once
#include "Auxiliary.h"

namespace FRAMEWORK
{
	struct MetaType;

	//TODO compile-time hash type name
	FRAMEWORK_API TMap<FString, MetaType*>& GetTypeNameToMetaType();
	FRAMEWORK_API TMap<FString, MetaType*>& GetRegisteredMetaTypes();

	struct MetaType
	{
		void* GetDefaultObject()
		{
			if (DefaultObjectGetter)
			{
				return DefaultObjectGetter();
			}
			return nullptr;
		}

		void* Construct()
		{
			check(DefaultObjectGetter);
			return DefaultObjectGetter();
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
			if (TypeName == GetGeneratedTypeName<T>())
			{
				return true;
			}

			MetaType* BaseMetaType = GetBaseClass();
			while(BaseMetaType)
			{
				if (BaseMetaType->TypeName == GetGeneratedTypeName<T>())
				{
					return true;
				}
				BaseMetaType = BaseMetaType->GetBaseClass();
			} 
			return false;
		}

		FString TypeName;
		FString RegisteredName;
		TFunction<void*()> DefaultObjectGetter;
		TFunction<MetaType*()> BaseMetaTypeGetter;
	};

	template<typename T>
	class MetaTypeBuilder
	{
	public:
		MetaTypeBuilder()
		{
			Meta = new MetaType{};
			//Dont use AUX::TypeName<T>, there is a unordered dynamic initialization problem.
			Meta->TypeName = GetGeneratedTypeName<T>();
			Meta->RegisteredName = Meta->TypeName;
			if constexpr(std::is_default_constructible_v<T>)
			{
				Meta->DefaultObjectGetter = [] { return new T; };
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
			check((std::is_base_of_v<Base, T>));
			Meta->BaseMetaTypeGetter = [] {
				return GetTypeNameToMetaType()[GetGeneratedTypeName<Base>()];
			};
			return *this;
		}

		~MetaTypeBuilder()
		{
			GetTypeNameToMetaType().Add(Meta->TypeName, Meta);
			GetRegisteredMetaTypes().Add(Meta->RegisteredName, Meta);
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
		return GetTypeNameToMetaType()[GetGeneratedTypeName<T>()];
	}

	template<typename T>
	TArray<MetaType*> GetMetaTypes()
	{
		TArray<MetaType*> MetaTypes;
		for (auto [_, MetaTypePtr] : GetRegisteredMetaTypes())
		{
			if (MetaTypePtr->IsDerivedFrom<T>())
			{
				MetaTypes.Add(MetaTypePtr);
			}
		}
		return MetaTypes;
	}

    template<typename T>
    T* GetDefaultObject(TFunctionRef<bool(T*)> Pred)
    {
        TArray<MetaType*> MetaTypes = GetMetaTypes<T>();
        for (auto MetaTypePtr : MetaTypes)
        {
            void* DefaultObject = MetaTypePtr->GetDefaultObject();
            if (DefaultObject)
            {
                T* RelDefaultObject = static_cast<T*>(DefaultObject);
                if(Pred(RelDefaultObject))
                {
                    return RelDefaultObject;
                }
            }
        }
        return nullptr;
    }
    
    template<typename T>
    void ForEachDefaultObject(TFunctionRef<void(T*)> Pred)
    {
        TArray<MetaType*> MetaTypes = GetMetaTypes<T>();
        for (auto MetaTypePtr : MetaTypes)
        {
            void* DefaultObject = MetaTypePtr->GetDefaultObject();
            if (DefaultObject)
            {
                T* RelDefaultObject = static_cast<T*>(DefaultObject);
                Pred(RelDefaultObject);
            }
        }
    }

#define GLOBAL_REFLECTION_REGISTER(...)	\
	static const int PREPROCESSOR_JOIN(ReflectionGlobalRegister_,__COUNTER__) = [] { __VA_ARGS__; return 0; }();

#define MANUAL_RTTI_BASE_TYPE() \
	template<typename T> bool IsOfType() const { return IsOfTypeImpl(T::GetTypeId());} \
	virtual bool IsOfTypeImpl(const FString& Type) const { return false; }

#define MANUAL_RTTI_TYPE(TYPE, Base) \
	static const FString& GetTypeId() { static FString Type = TEXT(#TYPE); return Type; } \
	virtual bool IsOfTypeImpl(const FString& Type) const override { return GetTypeId() == Type || Base::IsOfTypeImpl(Type); }
}
