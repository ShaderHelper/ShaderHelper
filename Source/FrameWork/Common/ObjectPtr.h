#pragma once

namespace FW
{
    template <typename T, ObjectOwnerShip OwnerShip = ObjectOwnerShip::Retain>
    class ObjectPtr
    {
        template <typename OtherType, ObjectOwnerShip OtherOwnerShip>
        friend class ObjectPtr;
        
    public:
        ObjectPtr(ShObject* InReference = nullptr)
        {
            SetReference(InReference);
        }
        
        ~ObjectPtr()
        {
            ReleaseReference();
        }
        
        ObjectPtr(const ObjectPtr& Copy)
        {
            SetReference(Copy.Reference);
            Guid = Copy.Guid;
        }

        template<typename OtherType, ObjectOwnerShip OtherOwnerShip>
		requires std::is_convertible_v<OtherType*, T*>
        ObjectPtr(const ObjectPtr<OtherType, OtherOwnerShip>& Copy)
        {
            SetReference(Copy.Reference);
            Guid = Copy.Guid;
        }
        
        ObjectPtr& operator=(const ObjectPtr& OtherObjectPtr)
        {
            ObjectPtr Temp{ OtherObjectPtr };
            Swap(Temp, *this);
            return *this;
        }

        template<typename OtherType, ObjectOwnerShip OtherOwnerShip>
		requires std::is_convertible_v<OtherType*, T*>
        ObjectPtr& operator=(const ObjectPtr<OtherType, OtherOwnerShip>& OtherObjectPtr)
        {
            ObjectPtr Temp{ OtherObjectPtr };
            Swap(Temp, *this);
            return *this;
        }
        
        T* operator->() const
        {
			check(IsValid());
            return Get();
        }

		T* Get() const { return static_cast<T*>(Reference); }

        FGuid GetGuid() const
        {
            return Reference ? Reference->GetGuid() : Guid;
        }

        void SetReference(ShObject* InReference)
        {
			if (Reference == InReference)
			{
				Guid = Reference ? Reference->GetGuid() : FGuid{};
				return;
			}

			ReleaseReference();
			Reference = InReference;
			AddReference();
			Guid = Reference ? Reference->GetGuid() : FGuid{};
        }

        void Reset()
        {
            ReleaseReference();
            Reference = nullptr;
            Guid = {};
        }

        void SetGuid(const FGuid& InGuid)
        {
            Guid = InGuid;
            check(!Reference || Reference->GetGuid() == Guid);
        }

        operator T*() const
        {
            return Get();
        }
        
        bool IsValid() const
        {
            return Reference != nullptr && GlobalValidShObjects.Contains(Reference);
        }
        
        explicit operator bool() const
        {
            return IsValid();
        }        
        friend uint32 GetTypeHash(const ObjectPtr& InPtr)
        {
			return GetTypeHash(InPtr.GetGuid());
        }
        
        template<typename OtherType, ObjectOwnerShip OtherOwnerShip>
        bool operator==(const ObjectPtr<OtherType, OtherOwnerShip>& Other) const
        {
            return GetGuid() == Other.GetGuid();
        }
                
    private:
		void AddReference()
		{
			if constexpr (OwnerShip != ObjectOwnerShip::Assign)
			{
				if (Reference)
				{
					Reference->Add();
				}
			}
		}

        void ReleaseReference()
        {
			if constexpr (OwnerShip != ObjectOwnerShip::Assign)
            {
                if (IsValid())
                {
                    Reference->Release();
                }
            }
        }

        ShObject* Reference = nullptr;
        FGuid Guid{};
    };
}
