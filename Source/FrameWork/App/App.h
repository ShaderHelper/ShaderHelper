#pragma once
#include "Common/Util/SwizzleVector.h"
#include "Renderer/Renderer.h"
namespace FRAMEWORK {
	class FRAMEWORK_API App : public FNoncopyable
	{
	public:
		App(const TCHAR* CommandLine) : SavedCommandLine(CommandLine) {}
		virtual ~App() = default;
	public:
		void Run();

		Renderer* GetRenderer() const { return AppRenderer.Get();}

		double GetCurrentTime() { return CurrentTime; }
		void SetCurrentTime(double InTime) { CurrentTime = InTime; }

		double GetDeltaTime() { return DeltaTime; }
		void SetDeltaTime(double InDeltaTime) { DeltaTime = InDeltaTime; }

		double GetFixedDeltaTime() { return FixedDeltaTime; }
		void SetFixedDeltaTime(double InFixedDeltaTime) { FixedDeltaTime = InFixedDeltaTime;}

		Vector2D GetClientSize() { return AppClientSize; }
		void SetClientSize(Vector2D InClientSize) {AppClientSize = MoveTemp(InClientSize);}

		bool AreAllWindowsHidden() const;
		
	protected:
		virtual void Init();
		virtual void PostInit();
		virtual void ShutDown();
		virtual void Update(double DeltaTime);

	protected:
		TUniquePtr<Renderer> AppRenderer;
		
		Vector2D AppClientSize = Vector2D(1280, 720);
		double CurrentTime = 0.0;
		double DeltaTime = 1 / 30;
		double FixedDeltaTime = 1 / 30;

		FString SavedCommandLine;
	};
}


