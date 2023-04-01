#include "CommonHeader.h"
#include "App.h"
#include <HAL/PlatformOutputDevices.h>
#include <HAL/PlatformApplicationMisc.h>
#include <StandaloneRenderer/StandaloneRenderer.h>
#include "Common/Path/BaseResourcePath.h"

#include <SlateCore/Fonts/SlateFontInfo.h>
#include <Misc/OutputDeviceConsole.h>

namespace FRAMEWORK {

	void UE_Init(const TCHAR* CommandLine) {
		FCommandLine::Set(CommandLine);
		
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

	App::App(TSharedPtr<SWindow> InWindow)
		: AppWindow(MoveTemp(InWindow))
	{
		FSlateApplication::Get().AddWindow(AppWindow.ToSharedRef());
	}

	App::App(TSharedPtr<SWindow> InWindow, TUniquePtr<Renderer> InRenderer)
		: AppWindow(MoveTemp(InWindow))
		, AppRenderer(MoveTemp(InRenderer))
	{
		FSlateApplication::Get().AddWindow(AppWindow.ToSharedRef());
	}

	void App::Run()
	{
		Init();
		PostInit();
		while (!IsEngineExitRequested()) {
			BeginExitIfRequested();

			double CurrentRealTime = FPlatformTime::Seconds();
			double LastRealTime = GetCurrentTime();
			double DeltaTime = CurrentRealTime - LastRealTime;
			SetCurrentTime(CurrentRealTime);
			SetDeltaTime(DeltaTime);

			FSlateApplication::Get().PumpMessages();
			Update(DeltaTime);

			if (AppRenderer.IsValid()) {
				AppRenderer->Render();
			}
			//if not change GFrameCounter, slate texture may not update.

			FSlateApplication::Get().Tick();
			GFrameCounter++;
		}
		ShutDown();
	}

}
