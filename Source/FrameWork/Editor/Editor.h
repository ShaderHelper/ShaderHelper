#pragma once

namespace FW {

	enum class SupportedLanguage
	{
		English,
		Chinese,
		Num,
	};

	class FRAMEWORK_API Editor
	{
	public:
		Editor();
		virtual ~Editor();

		static void SetLanguage(SupportedLanguage InLanguage);
		static SupportedLanguage GetLanguage() { return CurLanguage; }
        virtual void Update(double DeltaTime) {}
		virtual TSharedPtr<SWindow> GetMainWindow() const { return {}; }
		static FConfigFile* GetEditorConfig() { return EditorConfig.Get(); }
		static void SaveEditorConfig();

	private:
		static inline SupportedLanguage CurLanguage = SupportedLanguage::English;
		static inline TUniquePtr<FConfigFile> EditorConfig = MakeUnique<FConfigFile>();
	};
}


