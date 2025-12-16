#pragma once
#include <Styling/StyleColors.h>

namespace FW {

	enum class SupportedLanguage
	{
		English,
		Chinese,
		Num,
	};

	FRAMEWORK_API FString EditorConfigPath();
	FRAMEWORK_API FString EditorKeyBindingConfigPath();

	class FRAMEWORK_API Editor
	{
	public:
		Editor();
		virtual ~Editor();

		static void SetLanguage(SupportedLanguage InLanguage);
		static SupportedLanguage GetLanguage() { return CurLanguage; }
		static void SetTheme(const FGuid& InId);
		static const FStyleTheme& GetTheme(const FGuid& InId);
		static const FStyleTheme& GetDefaultTheme() { return GetTheme(DefaultThemeId); };
		static FStyleTheme& GetMutableCurrentTheme();

        virtual void Update(double DeltaTime) {}
		virtual TSharedPtr<SWindow> GetMainWindow() const { return {}; }
		static FConfigFile* GetEditorConfig() { return EditorConfig.Get(); }
		static void SaveEditorConfig();

	private:
		static inline SupportedLanguage CurLanguage = SupportedLanguage::English;
		static inline FGuid DefaultThemeId = FGuid(0x13438026, 0x5FBB4A9C, 0xA00A1DC9, 0x770217B8);
		static inline TUniquePtr<FConfigFile> EditorConfig = MakeUnique<FConfigFile>();
	};
}


