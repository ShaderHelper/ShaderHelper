#pragma once

namespace FW
{
    template <typename T, ObjectOwnerShip OwnerShip = ObjectOwnerShip::Retain>
    class ObjectPtr
    {
        template <typename OtherType, ObjectOwnerShip OtherOwnerShip>
        friend class ObjectPtr;
        
    public:
        ObjectPtr(T* InReference = nullptr)
            : Reference(InReference)
        {
            if (OwnerShip != ObjectOwnerShip::Assign)
            {
                if(Reference)
                {
                    Reference->Add();
                }
            }
        }
        
        ~ObjectPtr()
        {
            if (OwnerShip != ObjectOwnerShip::Assign)
            {
                if(IsValid())
                {
                    Reference->Release();
                }
            }
        }
        
        ObjectPtr(const ObjectPtr& Copy) : Reference(Copy.Reference)
        {
            if (OwnerShip != ObjectOwnerShip::Assign)
            {
                if(Reference)
                {
                    Reference->Add();
                }
            }
        }

        template<typename OtherType, ObjectOwnerShip OtherOwnerShip>
		requires std::is_convertible_v<OtherType*, T*>
        ObjectPtr(const ObjectPtr<OtherType, OtherOwnerShip>& Copy) : Reference(Copy.Reference)
        {
            if (OwnerShip != ObjectOwnerShip::Assign)
            {
                if(Reference)
                {
                    Reference->Add();
                }
            }
        }
        
        ObjectPtr& operator=(const ObjectPtr& OtherObjectPtr)
        {
            ObjectPtr Temp{ OtherObjectPtr };
            Swap(Temp, *this);
            return *this;
        }
        
        T* operator->() const
        {
            return Reference;
        }

        operator T*() const
        {
            return Reference;
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
            return GetTypeHash(InPtr->GetGuid());
        }
        
        template<typename OtherType, ObjectOwnerShip OtherOwnerShip>
        bool operator==(const ObjectPtr<OtherType, OtherOwnerShip>& Other) const
        {
            return Reference == Other.Reference;
        }
                
    private:
        T* Reference;
    };
}
