#include "CommonHeaderForUE.h"
#include "ShaderHelperApp.h"
#include <HAL/PlatformOutputDevices.h>
#include <HAL/PlatformApplicationMisc.h>
#include <StandaloneRenderer/StandaloneRenderer.h>
#include "Misc/CommonPath/BaseResourcePath.h"
#include "UI/FShaderHelperStyle.h"
#include "UI/SShaderHelperWindow.h"
#include "SlateCore/Fonts/SlateFontInfo.h"

namespace {
	void UE_Init() {
		//Initializing OutputDevices for features like UE_LOG.
		FPlatformOutputDevices::SetupOutputDevices();

		FTaskGraphInterface::Startup(FPlatformMisc::NumberOfCores());

		//Some interfaces with uobject in slate module depend on CoreUobject module.
		FModuleManager::Get().LoadModule(TEXT("CoreUObject"));

		FPlatformMisc::PlatformPreInit();
		FPlatformApplicationMisc::PreInit();

		GConfig = new FConfigCacheIni(EConfigCacheType::DiskBacked);

		FCoreDelegates::OnInit.Broadcast();

		FPlatformMisc::PlatformInit();
		FPlatformApplicationMisc::Init();
		FPlatformMemory::Init();
		FPlatformApplicationMisc::PostInit();

		//This project uses slate as UI framework, so we need to initialize it.
		SetSlateFontPath(SH::BaseResourcePath::UE_SlateFontDir);
		FSlateApplication::SetCoreStylePath(SH::BaseResourcePath::UE_SlateResourceDir);
		FSlateApplication::InitializeAsStandaloneApplication(GetStandardStandaloneRenderer(SH::BaseResourcePath::UE_StandaloneRenderShaderDir));
		
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

namespace SH {
#pragma optimize("", off)
	ShaderHelperApp::ShaderHelperApp(const TCHAR* CommandLine)
	{
		FCommandLine::Set(CommandLine);
	}

	void ShaderHelperApp::Init()
	{
		UE_Init();
		FShaderHelperStyle::Init();
		FSlateApplication::Get().AddWindow(SNew(SShaderHelperWindow));
	}

	void ShaderHelperApp::ShutDown()
	{
		UE_ShutDown();
		FShaderHelperStyle::ShutDown();

	}

	void ShaderHelperApp::Update(double DeltaTime)
	{
		UE_Update();

	}

	void ShaderHelperApp::Run()
	{
		Init();
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
#pragma optimize("", on)
}
