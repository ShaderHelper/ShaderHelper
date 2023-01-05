#include "CommonHeader.h"
#include "App.h"
#include <HAL/PlatformOutputDevices.h>
#include <HAL/PlatformApplicationMisc.h>
#include <StandaloneRenderer/StandaloneRenderer.h>
#include "Common/Path/BaseResourcePath.h"

#include <SlateCore/Fonts/SlateFontInfo.h>
#include <Misc/OutputDeviceConsole.h>
#include "Common/Util/UnitTest.h"

static TUniquePtr<FOutputDeviceConsole>	GScopedLogConsole;

namespace {
	void UE_Init() {
#if UNIT_TEST
		GScopedLogConsole = TUniquePtr<FOutputDeviceConsole>(FPlatformApplicationMisc::CreateConsoleOutputDevice());
		GLogConsole = GScopedLogConsole.Get();
#endif
		//Initializing OutputDevices for features like UE_LOG.
		FPlatformOutputDevices::SetupOutputDevices();

		FTaskGraphInterface::Startup(FPlatformMisc::NumberOfCores());

		//Some interfaces with uobject in slate module depend on CoreUobject module.
		FModuleManager::Get().LoadModule(TEXT("CoreUObject"));

		FPlatformMisc::PlatformPreInit();
		FPlatformApplicationMisc::PreInit();

		GConfig = new FConfigCacheIni(EConfigCacheType::DiskBacked);

		FCoreDelegates::OnInit.Broadcast();

		FMath::RandInit(FPlatformTime::Cycles());

		FPlatformMisc::PlatformInit();
		FPlatformApplicationMisc::Init();
		FPlatformMemory::Init();
		FPlatformApplicationMisc::PostInit();

		//This project uses slate as UI framework, so we need to initialize it.
		SetSlateFontPath(FRAMEWORK::BaseResourcePath::UE_SlateFontDir);
		FSlateApplication::SetCoreStylePath(FRAMEWORK::BaseResourcePath::UE_SlateResourceDir);
		FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer(FRAMEWORK::BaseResourcePath::UE_StandaloneRenderShaderDir));

	}

	void UE_ShutDown() {
		FCoreDelegates::OnPreExit.Broadcast();
		FCoreDelegates::OnExit.Broadcast();
		FSlateApplication::Shutdown();
		FModuleManager::Get().UnloadModulesAtShutdown();
		FTaskGraphInterface::Shutdown();

		FPlatformApplicationMisc::TearDown();
		FPlatformMisc::PlatformTearDown();

		if (GLog)
		{
			GLog->TearDown();
		}
	}

	void UE_Update() {
		BeginExitIfRequested();
		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
	}
}

namespace FRAMEWORK {

	App::App(const TCHAR* CommandLine)
	{
		FCommandLine::Set(CommandLine);
	}

	void App::Init()
	{
		UE_Init();
	}

	void App::PostInit()
	{
#if UNIT_TEST
		if (GLogConsole) { GLogConsole->Show(true); }
		FName TestName;
		if (FParse::Value(FCommandLine::Get(), TEXT("UnitTest="), TestName)) {
			UnitTest::Run(TestName);
		}
#else
		if (AppWindow.IsValid()) {
			FSlateApplication::Get().AddWindow(AppWindow.ToSharedRef());
		}
		else {
            FSlateApplication::Get().AddWindow(SNew(SWindow).ClientSize(DefaultClientSize));
		}
#endif
	}

	void App::ShutDown()
	{
		UE_ShutDown();
	}

	void App::Update(double DeltaTime)
	{
		UE_Update();

	}

	void App::Run()
	{
		Init();
		PostInit();
		while (!IsEngineExitRequested()) {
			double CurrentRealTime = FPlatformTime::Seconds();
			double LastRealTime = GetCurrentTime();
			double DeltaTime = CurrentRealTime - LastRealTime;
			SetCurrentTime(CurrentRealTime);
			SetDeltaTime(DeltaTime);
			Update(DeltaTime);
		}
		ShutDown();
	}


}
