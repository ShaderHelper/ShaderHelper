#pragma once
#include "Auxiliary.h"
#include "magic_enum.hpp"

namespace FW
{
	struct MetaType;
    enum class ObjectOwnerShip;
    template<typename T, ObjectOwnerShip> class ObjectPtr;
    template<typename T>
    struct is_object_ptr { static constexpr bool value = false;};
    template<typename T, ObjectOwnerShip OwnerShip>
    struct is_object_ptr<ObjectPtr<T, OwnerShip>> { static constexpr bool value = true; };
    template<typename T>
    struct object_ptr_trait;
    template<typename T, ObjectOwnerShip OwnerShip>
    struct object_ptr_trait<ObjectPtr<T, OwnerShip>> { using type = T;};

	FRAMEWORK_API TMap<FString, MetaType*>& GetTypeNameToMetaType();
	FRAMEWORK_API TMap<FString, MetaType*>& GetRegisteredNameToMetaType();

    enum class MetaInfo
    {
        None,
        Property, //will occur in the property panel.
    };

    struct MetaMemberData
    {
        template<typename T>
        bool IsType()
        {
            return TypeName == AUX::TypeName<T>;
        }
        
        template<typename T>
        void SetValue(void* Instance, const T& Value)
        {
            Set(Instance, &Value);
        }
        
        template<typename T>
        T GetValue(void* Instance)
        {
            return *static_cast<T*>(Get(Instance));
        }
        
        bool IsAssetRef();
        
        FString MemberName;
        FString TypeName;
        void(*Set)(void*, void*);
        void*(*Get)(void*);
        MetaInfo InfoType;
        void* InfoData;
        
        bool bShObjectRef;
        MetaType*(*GetShObjectMetaType)();
		
		FString(*GetEnumValueName)(void*) = nullptr;
		TMap<FString, TSharedPtr<void>> EnumEntries;
    };

	struct MetaType
	{
		void* GetDefaultObject()
		{
			if (!DefaultObject && Constructor)
			{
				DefaultObject = Constructor();
			}
			return DefaultObject;
		}

		void* Construct()
		{
			check(Constructor);
			return Constructor();
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
		TOptional<FString> RegisteredName; //Portable
		void*(*Constructor)();
		MetaType*(*BaseMetaTypeGetter)();
		void* DefaultObject;
        TArray<MetaMemberData> Datas;
	};

    template<typename T>
    MetaType* GetMetaType()
    {
        checkf(GetTypeNameToMetaType().Contains(GetGeneratedTypeName<T>()),
            TEXT("Please ensure that have registered the type: %s."), *AUX::TypeName<T>);
        return GetTypeNameToMetaType()[GetGeneratedTypeName<T>()];
    }

    inline MetaType* GetMetaType(const FString& InRegisteredName)
    {
        return GetRegisteredNameToMetaType()[InRegisteredName];
    }

	template<typename T>
	class MetaTypeBuilder
	{
	public:
		MetaTypeBuilder()
		{
			Meta = new MetaType{};
			//Dont use AUX::TypeName<T>, there is a unordered dynamic initialization problem.
			Meta->TypeName = GetGeneratedTypeName<T>();
			if constexpr(std::is_default_constructible_v<T>)
			{
				Meta->Constructor = []() -> void* { 
					return new T;
				};
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
        
        template<auto DataPtr, MetaInfo InfoType = MetaInfo::None>
        MetaTypeBuilder& Data(const FString& InMemberName, void* InfoData = nullptr)
        {
            using MemberType = typename AUX::TraitMemberTypeFromMemberPtr<decltype(DataPtr)>::Type;
            using RawType = std::decay_t<MemberType>;
            MetaMemberData MemberData{};
            MemberData.MemberName = InMemberName;
            MemberData.TypeName = GetGeneratedTypeName<RawType>();
            MemberData.Set = [](void* Instance, void* Value){
                (T*)Instance->*DataPtr = *(MemberType*)Value;
            };
            MemberData.Get = [](void* Instance) -> void* {
                return &((T*)Instance->*DataPtr);
            };
			if constexpr(std::is_enum_v<MemberType>)
			{
				MemberData.GetEnumValueName = [](void* Instance) -> FString {
					return magic_enum::enum_name((T*)Instance->*DataPtr).data();
				};
				for(const auto& [EntryValue, EntryStr] : magic_enum::enum_entries<MemberType>())
				{
					MemberData.EnumEntries.Add(EntryStr.data(), MakeShared<MemberType>(EntryValue));
				}
			}
            MemberData.InfoType = InfoType;
            MemberData.InfoData = InfoData;
            if constexpr(is_object_ptr<RawType>::value)
            {
                MemberData.bShObjectRef = true;
                //the metatype of member may not be registered at the moment, so lazy get.
                using ShObjectType = typename object_ptr_trait<RawType>::type;
                MemberData.GetShObjectMetaType = [] { return GetMetaType<ShObjectType>(); };
            }
 
            Meta->Datas.Add(MoveTemp(MemberData));
            return *this;
        }

		~MetaTypeBuilder()
		{
			GetTypeNameToMetaType().Add(Meta->TypeName, Meta);
			if (Meta->RegisteredName)
			{
				GetRegisteredNameToMetaType().Add(*Meta->RegisteredName, Meta);
			}
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

	inline FString GetRegisteredName(MetaType* InMt)
	{
		const FString* Rel = GetRegisteredNameToMetaType().FindKey(InMt);
		check(Rel);
		return *Rel;
	}

	template<typename To, typename From>
	To* DynamicCast(From* InPtr)
    {
        if(!InPtr)
        {
            return nullptr;
        }
        
		MetaType* Mt = InPtr->DynamicMetaType();
		if (Mt->IsDerivedFrom<To>())
		{
			return (To*)(InPtr);
		}
		return nullptr;
	}

	template<typename T>
	TArray<MetaType*> GetMetaTypes()
	{
		TArray<MetaType*> MetaTypes;
		for (auto [_, MetaTypePtr] : GetTypeNameToMetaType())
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
    void ForEachDefaultObject(TFunctionRef<void(T*)> Func)
    {
        TArray<MetaType*> MetaTypes = GetMetaTypes<T>();
        for (auto MetaTypePtr : MetaTypes)
        {
            void* DefaultObject = MetaTypePtr->GetDefaultObject();
            if (DefaultObject)
            {
                T* RelDefaultObject = static_cast<T*>(DefaultObject);
				Func(RelDefaultObject);
            }
        }
    }

    inline TArray<MetaMemberData*> GetProperties(MetaType* InMt)
    {
        TArray<MetaMemberData*> Datas;
        for(auto& Data : InMt->Datas)
        {
            if(Data.InfoType == MetaInfo::Property)
            {
                Datas.Add(&Data);
            }
        }
        return Datas;
    }

#define REFLECTION_REGISTER(...)	\
	static const int PREPROCESSOR_JOIN(ReflectionGlobalRegister_,__COUNTER__) = [] { __VA_ARGS__; return 0; }();

#define REFLECTION_TYPE(Type)       \
    public:                         \
        virtual FW::MetaType* DynamicMetaType() const {  return FW::GetMetaType<Type>(); }
    

#define MANUAL_RTTI_BASE_TYPE() \
    public: \
	template<typename T> bool IsOfType() const { return IsOfTypeImpl(T::GetTypeId());} \
	virtual bool IsOfTypeImpl(const FString& Type) const { return false; }

//Don't support MANUAL_RTTI_TYPE(PropertyItem<T>, PropertyItemBase)
#define MANUAL_RTTI_TYPE(TYPE, Base) \
    public: \
	static const FString& GetTypeId() { static FString Type = TEXT(#TYPE); return Type; } \
	virtual bool IsOfTypeImpl(const FString& Type) const override { return GetTypeId() == Type || Base::IsOfTypeImpl(Type); }
}
