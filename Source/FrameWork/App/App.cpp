#include "CommonHeader.h"
#include "App.h"
#include <HAL/PlatformOutputDevices.h>
#include <HAL/PlatformApplicationMisc.h>
#include <StandaloneRenderer.h>
#include "Common/Path/BaseResourcePath.h"
#include <Fonts/SlateFontInfo.h>
#include <Misc/OutputDeviceConsole.h>
#include <DirectoryWatcherModule.h>
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
        FFileHelper::SaveStringToFile(CommandLine, *(PathHelper::SavedDir() / TEXT("Cmd.txt")));
		
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

	App::App(const Vector2D& InClientSize, const TCHAR* InCommandLine)
		: AppClientSize(InClientSize), CommandLine(InCommandLine)
	{
	
	}

	void App::Init()
	{
		UE_Init(*CommandLine);

		// Create Rhi backend.
		GpuRhiConfig Config;
#if SH_SHIPPING
		Config.EnableValidationCheck = false;
#else
		Config.EnableValidationCheck = true;
#endif
		Config.BackendType = GpuRhiBackendType::Default;

		if (FParse::Param(FCommandLine::Get(), TEXT("disableValidation"))) {
			Config.EnableValidationCheck = false;
		}
		else if (FParse::Param(FCommandLine::Get(), TEXT("enableValidation"))) {
			Config.EnableValidationCheck = true;
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

		FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
		DirectoryWatcher = DirectoryWatcherModule.Get();
	}

	void App::Run()
	{
		Init();

        while (!IsEngineExitRequested()) {
#if PLATFORM_MAC
            //Ensure that NSObjects autoreleased every frame are immediately released.
            SCOPED_AUTORELEASE_POOL;
#endif
            BeginExitIfRequested();
            
            //Note: FPlatformTime::Seconds must be passed to double. see FApplePlatformTime
            double LastRealTime = FPlatformTime::Seconds();
            
			bool bIdleMode = AreAllWindowsHidden();

			if (!bIdleMode)
			{
				GGpuRhi->BeginFrame();
				{
					Update(DeltaTime);
					Render();
					FSlateApplication::Get().PumpMessages();
					FSlateApplication::Get().Tick();
                    FTicker::GetCoreTicker().Tick(DeltaTime);
					//if not change GFrameCounter, slate texture may not update.
					GFrameCounter++;
				}
				GGpuRhi->EndFrame();
			}
			else
			{
				FSlateApplication::Get().PumpMessages();
			}
            
            double CurrentRealTime = FPlatformTime::Seconds();
            float NewDeltaTime = float(CurrentRealTime - LastRealTime);
            DeltaTime = NewDeltaTime;
            
            //Lock fps to reduce cpu usage
            const float MaxDeltaTime = 1.0f / 60.0f;
            if(NewDeltaTime < MaxDeltaTime)
            {
                const float WaitTime = MaxDeltaTime - NewDeltaTime;
                double WaitEndTime = CurrentRealTime + WaitTime;
                if (WaitTime > 0.004f)
                {
                    FPlatformProcess::SleepNoStats(WaitTime - 0.001f);
                }
                while (FPlatformTime::Seconds() < WaitEndTime)
                {
                    FPlatformProcess::SleepNoStats(0);
                }
                CurrentRealTime = FPlatformTime::Seconds();
                DeltaTime = float(CurrentRealTime - LastRealTime);
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

	void App::Update(float DeltaTime)
	{
		DirectoryWatcher->Tick(DeltaTime);

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
