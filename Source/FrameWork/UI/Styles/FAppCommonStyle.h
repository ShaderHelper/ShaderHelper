#pragma once

namespace FRAMEWORK
{
	class FRAMEWORK_API FAppCommonStyle
	{
	public:
        static const ISlateStyle& Get();
        
		static const FName& GetStyleSetName()
		{
			checkf(Instance.IsValid(), TEXT("Instance must be valid."));
			return Instance->GetStyleSetName();
		}
    private:
        static TSharedRef<ISlateStyle> Create();
        
	private:
		/** Singleton instances of this style. */
		static inline TSharedPtr<ISlateStyle> Instance = nullptr;
	};

}

