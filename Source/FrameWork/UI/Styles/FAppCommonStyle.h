#pragma once

namespace FRAMEWORK
{
	class FRAMEWORK_API FAppCommonStyle
	{
	public:
		static void Init();
		static void ShutDown();

		static TSharedRef<ISlateStyle> Create();
		static const ISlateStyle& Get()
		{
			return *Instance;
		}

		static const FName& GetStyleSetName()
		{
			checkf(Instance.IsValid(), TEXT("Instance must be valid."));
			return Instance->GetStyleSetName();
		}
	private:
		/** Singleton instances of this style. */
		static inline TSharedPtr<ISlateStyle> Instance = nullptr;
	};

}

