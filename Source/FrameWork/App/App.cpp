#include "CommonHeader.h"
#include "App.h"
#include <HAL/PlatformOutputDevices.h>
#include <HAL/PlatformApplicationMisc.h>
#include <StandaloneRenderer/StandaloneRenderer.h>
#include "Common/Path/BaseResourcePath.h"

#include <SlateCore/Fonts/SlateFontInfo.h>
#include <Misc/OutputDeviceConsole.h>

namespace FRAMEWORK {

	void App::UE_Init() {
		//Initializing OutputDevices for features like UE_LOG.
		FPlatformOutputDevices::SetupOutputDevices();

		FTaskGraphInterface::Startup(FPlatformMisc::NumberOfCores());

		//Some interfaces with uobject in slate module depend on the CoreUobject module.
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

	void App::UE_ShutDown() {
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

	void App::UE_Update() {
		BeginExitIfRequested();
		FSlateApplication::Get().PumpMessages();
		FSlateApplication::Get().Tick();
	}

	App::App(const TCHAR* CommandLine)
	{
		FCommandLine::Set(CommandLine);
	}

	App::App(const TCHAR* CommandLine, TUniquePtr<Renderer>&& InRenderer)
		: AppRenderer(MoveTemp(InRenderer))
	{
		FCommandLine::Set(CommandLine);
	}

	void App::Init()
	{
		UE_Init();
	}

	void App::PostInit()
	{
		if (AppWindow.IsValid()) {
			FSlateApplication::Get().AddWindow(AppWindow.ToSharedRef());
		}
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

			if (AppRenderer.IsValid()) {
				AppRenderer->Render();
			}
			//if not change GFrameCounter, slate texture may not update.
			GFrameCounter++;
		}
		ShutDown();
	}


}
