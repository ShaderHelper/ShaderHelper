#include "CommonHeader.h"
#include "App.h"
#include <HAL/PlatformOutputDevices.h>
#include <HAL/PlatformApplicationMisc.h>
#include <StandaloneRenderer/StandaloneRenderer.h>
#include "Common/Path/BaseResourcePath.h"
#include "UI/Styles/FAppCommonStyle.h"
#include <SlateCore/Fonts/SlateFontInfo.h>
#include <Misc/OutputDeviceConsole.h>

namespace FRAMEWORK {

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
		FSlateApplication::SetCoreStylePath(FRAMEWORK::BaseResourcePath::UE_SlateResourceDir);
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

	void App::Run()
	{
		Init();
		PostInit();
		while (!IsEngineExitRequested()) {
#if PLATFORM_MAC
            //Ensure that NSObjects autoreleased every frame are immediately released.
            SCOPED_AUTORELEASE_POOL;
#endif
			BeginExitIfRequested();

			double CurrentRealTime = FPlatformTime::Seconds();
			double LastRealTime = GetCurrentTime();
			double DeltaTime = CurrentRealTime - LastRealTime;
			SetCurrentTime(CurrentRealTime);
			SetDeltaTime(DeltaTime);

			FSlateApplication::Get().PumpMessages();

			bool bIdleMode = AreAllWindowsHidden();
			if (!bIdleMode)
			{
				Update(DeltaTime);

				if (AppRenderer.IsValid()) {
					AppRenderer->Render();
				}
				FSlateApplication::Get().Tick();
				//if not change GFrameCounter, slate texture may not update.
				GFrameCounter++;
			}
		}
		ShutDown();
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

	void App::Init()
	{
		UE_Init(*SavedCommandLine);
		FAppCommonStyle::Init();
	}

	void App::PostInit()
	{

	}

	void App::ShutDown()
	{
		UE_ShutDown();
		FAppCommonStyle::ShutDown();
	}

}
