#include "CommonHeader.h"
#include "UnitTestConSole.h"
#include "UI/Widgets/Log/SOutputLog.h"
#include "UI/Styles/FAppCommonStyle.h"

namespace UNITTEST_FRAMEWORK
{
	extern void TestUtil();

	void UnitTestConSole::Init()
	{
		App::Init();
		FAppCommonStyle::Init();
		InitLogWindow();
	}

	void UnitTestConSole::InitLogWindow()
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
				FTabManager::NewArea(static_cast<float>(AppClientSize.X), static_cast<float>(AppClientSize.Y))
				->Split
				(
					FTabManager::NewStack()
					->AddTab("LogTab", ETabState::OpenedTab)
					->SetForegroundTab(FName("LogTab"))
				)
			);

		FGlobalTabmanager::Get()->RestoreFrom(Layout, TSharedPtr<SWindow>());
	}

	void UnitTestConSole::PostInit()
	{
		App::PostInit();

		TestUtil();
	}


	void UnitTestConSole::ShutDown()
	{
		App::ShutDown();
	}


	void UnitTestConSole::Update(double DeltaTime)
	{
		App::Update(DeltaTime);
	}
}


