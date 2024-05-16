#pragma once
namespace SH {
	
	class FShaderHelperStyle
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


