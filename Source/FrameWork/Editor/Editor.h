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
		static inline SupportedLanguage CurLanguage = SupportedLanguage::Chinese;
		static inline TUniquePtr<FConfigFile> EditorConfig = MakeUnique<FConfigFile>();
        
        virtual void Update(double DeltaTime) {}
		virtual TSharedPtr<SWindow> GetMainWindow() const { return {}; }
	};
}


