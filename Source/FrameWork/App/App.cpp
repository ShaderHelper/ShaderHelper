#include "CommonHeader.h"
#include "App.h"
#include <HAL/PlatformOutputDevices.h>
#include <HAL/PlatformApplicationMisc.h>
#include <StandaloneRenderer.h>
#include "Common/Path/BaseResourcePath.h"
#include <Fonts/SlateFontInfo.h>
#include <Misc/OutputDeviceConsole.h>
#include <DirectoryWatcherModule.h>
#include <IDirectoryWatcher.h>
#include "GpuApi/GpuRhi.h"

namespace FRAMEWORK {
	TUniquePtr<App> GApp;

	static void UE_Init(const TCHAR* CommandLine)
    {
		FCommandLine::Set(CommandLine);
		
		//Initialize OutputDevices for features like UE_LOG, but not make log files.
		FCommandLine::Append(TEXT(" "));
		FCommandLine::Append(TEXT("-NODEFAULTLOG"));
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
		FSlateApplication::SetCoreStylePath(FRAMEWORK::BaseResourcePath::UE_CoreStyleDir);
		FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer(FRAMEWORK::BaseResourcePath::UE_StandaloneRenderShaderDir));

	}
	
	static void UE_ShutDown()
    {
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

	App::App(const Vector2D& InClientSize, const TCHAR* CommandLine)
		: AppClientSize(InClientSize)
	{
		UE_Init(CommandLine);

		// Create Rhi backend.
		GpuRhiConfig Config;
		Config.EnableValidationCheck = true;
		Config.BackendType = GpuRhiBackendType::Default;

		if (FParse::Param(FCommandLine::Get(), TEXT("disableValidation"))) {
			Config.EnableValidationCheck = false;
		}
		FString BackendName;
		if (FParse::Value(FCommandLine::Get(), TEXT("backend="), BackendName)) {
			if (BackendName.Equals(TEXT("Vulkan"))) {
				Config.BackendType = GpuRhiBackendType::Vulkan;
			}
#if PLATFORM_MAC
			else if (BackendName.Equals(TEXT("Metal"))) {
				Config.BackendType = GpuRhiBackendType::Metal;
			}
#elif PLATFORM_WINDOWS
			else if (BackendName.Equals(TEXT("DX12"))) {
				Config.BackendType = GpuRhiBackendType::DX12;
			}
#endif
			else {
				// invalid backend name, use default backend type.
			}
		}
		GpuRhi::InitGpuRhi(Config);
		GGpuRhi->InitApiEnv();
	}

	App::~App()
	{
		UE_ShutDown();
	}

	void App::Run()
	{
		double CurrentRealTime = FPlatformTime::Seconds();
		double LastRealTime = CurrentRealTime;
		while (!IsEngineExitRequested()) {
#if PLATFORM_MAC
            //Ensure that NSObjects autoreleased every frame are immediately released.
            SCOPED_AUTORELEASE_POOL;
#endif
			BeginExitIfRequested();

			CurrentRealTime = FPlatformTime::Seconds();
			double NewDeltaTime = CurrentRealTime - LastRealTime;

			LastRealTime = CurrentRealTime;
			DeltaTime = NewDeltaTime;

			bool bIdleMode = AreAllWindowsHidden();
			if (!bIdleMode)
			{
				GGpuRhi->BeginFrame();
				{
					Update(DeltaTime);
					Render();
					FSlateApplication::Get().PumpMessages();
					FSlateApplication::Get().Tick();
PRAGMA_DISABLE_DEPRECATION_WARNINGS
                    FTicker::GetCoreTicker().Tick((float)DeltaTime);
PRAGMA_DISABLE_DEPRECATION_WARNINGS
					//if not change GFrameCounter, slate texture may not update.
					GFrameCounter++;
				}
				GGpuRhi->EndFrame();
			}
			else
			{
				FSlateApplication::Get().PumpMessages();
			}
		}
	}

	bool App::AreAllWindowsHidden() const
	{
		if (!FSlateApplication::IsInitialized())
		{
			return true;
		}
		const TArray< TSharedRef<SWindow> > AllWindows = FSlateApplication::Get().GetInteractiveTopLevelWindows();

		bool bAllHidden = true;
		for (const TSharedRef<SWindow>& Window : AllWindows)
		{
			if (!Window->IsWindowMinimized() && Window->IsVisible())
			{
				bAllHidden = false;
				break;
			}
		}

		return bAllHidden;
	}

	void App::Update(double DeltaTime)
	{
		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();
		DirectoryWatcher->Tick((float)DeltaTime);
	}

	void App::Render()
	{
        if(AppRenderer)
        {
            AppRenderer->Render();
        }
	}

}
