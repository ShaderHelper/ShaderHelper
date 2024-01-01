#include "CommonHeader.h"
#include "UnitTestEditor.h"
#include "UI/Widgets/Log/SOutputLog.h"

namespace UNITTEST_UTIL
{
	extern void TestUtil();

	UnitTestEditor::UnitTestEditor(const Vector2f& InWindowSize)
		: WindowSize(InWindowSize)
	{
		InitLogWindow();

		TestUtil();
	}

	UnitTestEditor::~UnitTestEditor()
	{
		
	}

	void UnitTestEditor::InitLogWindow()
	{
		auto SpawnLog = [](const FSpawnTabArgs&) -> TSharedRef<SDockTab>
		{
			return SNew(SDockTab)
				.Label(FText::FromString("OutputLog"))
				.TabRole(ETabRole::NomadTab)
				[
					SNew(SOutputLog)
				];
		};
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner("LogTab", FOnSpawnTab::CreateLambda(SpawnLog));

		TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("UnitTestLayout")
			->AddArea
			(
				FTabManager::NewArea(WindowSize.x, WindowSize.y)
				->Split
				(
					FTabManager::NewStack()
					->AddTab("LogTab", ETabState::OpenedTab)
					->SetForegroundTab(FName("LogTab"))
				)
			);

		FGlobalTabmanager::Get()->RestoreFrom(Layout, TSharedPtr<SWindow>());
	}

}
