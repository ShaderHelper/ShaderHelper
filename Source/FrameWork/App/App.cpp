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
#include <HAL/ExceptionHandling.h>
#include <HAL/PlatformOutputDevices.h>
#include <Misc/OutputDeviceFile.h>

namespace FW {
	TUniquePtr<App> GApp;

	static void UE_Init(const TCHAR* CommandLine)
    {
		GUseCrashReportClient = false;
		//Debug check() crsah report
		//GIgnoreDebugger = true;
#if PLATFORM_WINDOWS
		IFileManager::Get().MakeDirectory(*PathHelper::ErrorDir(), true);
		FCString::Strcpy(MiniDumpFilenameW, *(PathHelper::ErrorDir() / FString::Printf(TEXT("%s.dmp"), *GAppName)));

		FOutputDeviceFile* LogDevice = static_cast<FOutputDeviceFile*>(FPlatformOutputDevices::GetLog());
		LogDevice->SetFilename(*(PathHelper::SavedLogDir() / FString::Printf(TEXT("%s.log"), *GAppName)));

		GError = FPlatformApplicationMisc::GetErrorOutputDevice();
#endif

		FCommandLine::Set(CommandLine);
		
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
		SetSlateFontPath(FW::BaseResourcePath::UE_SlateFontDir);
		FSlateApplication::SetCoreStylePath(FW::BaseResourcePath::UE_CoreStyleDir);
		FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer(FW::BaseResourcePath::UE_StandaloneRenderShaderDir));
	
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

	App::App(const Vector2D& InClientSize, const TCHAR* InCommandLine)
		: AppClientSize(InClientSize), CommandLine(InCommandLine)
	{
	
	}

	App::~App()
	{
		UE_ShutDown();
	}


	void App::Init()
	{
		UE_Init(*CommandLine);

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

	void App::Run()
	{
		Init();

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
            
            //Reduce cpu usage
            FPlatformProcess::Sleep(0.01);
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
        
        if(AppEditor)
        {
            AppEditor->Update(DeltaTime);
        }
	}

	void App::Render()
	{
        if(AppRenderer)
        {
            AppRenderer->Render();
        }
	}
}
