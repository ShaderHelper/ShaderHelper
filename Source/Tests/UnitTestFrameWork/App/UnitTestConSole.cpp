#include "CommonHeader.h"
#include "UnitTestConSole.h"
#include "FrameWork/UI/Widgets/Log/SOutputLog.h"
#include "FrameWork/UI/Styles/FAppCommonStyle.h"

UnitTestConSole::UnitTestConSole(const TCHAR* CommandLine)
	:App(CommandLine)
{
	
}

void UnitTestConSole::Init()
{
	App::Init();
}

void UnitTestConSole::InitLogWindow()
{
	auto SpawnLog = [](const FSpawnTabArgs&) -> TSharedRef<SDockTab>
	{
		return SNew(SDockTab)
			.Label(FText::FromString("OutPutLog"))
			[
				SNew(SOutputLog)
			];
	};
	FGlobalTabmanager::Get()->RegisterTabSpawner("LogTab", FOnSpawnTab::CreateLambda(SpawnLog));
	
	TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("UnitTestLayout")
		->AddArea
		(
			FTabManager::NewArea(AppClientSize.X, AppClientSize.Y)
			->Split
			(
				FTabManager::NewStack()
				->AddTab("LogTab", ETabState::OpenedTab)
			)
		);
		
	FGlobalTabmanager::Get()->RestoreFrom(Layout, TSharedPtr<SWindow>());
}

void UnitTestConSole::PostInit()
{
	FAppCommonStyle::Init();
	InitLogWindow();
	App::PostInit();
	
	extern void TestUtil();
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
