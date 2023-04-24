#pragma once
#include "Common/Util/SwizzleVector.h"
#include "Renderer/Renderer.h"
namespace FRAMEWORK {
	class FRAMEWORK_API App : public FNoncopyable
	{
	public:
		App() = default;
		App(TSharedPtr<SWindow> InWindow);
		App(TSharedPtr<SWindow> InWindow, TUniquePtr<Renderer> InRenderer);
		virtual ~App() = default;
	public:
		void Run();

		Renderer* GetRenderer() const { return AppRenderer.Get();}

		static double GetCurrentTime() { return CurrentTime; }
		static void SetCurrentTime(double InTime) { CurrentTime = InTime; }

		static double GetDeltaTime() { return DeltaTime; }
		static void SetDeltaTime(double InDeltaTime) { DeltaTime = InDeltaTime; }

		static double GetFixedDeltaTime() { return FixedDeltaTime; }
		static void SetFixedDeltaTime(double InFixedDeltaTime) { FixedDeltaTime = InFixedDeltaTime;}

		static Vector2D GetClientSize() { return AppClientSize; }
		static void SetClientSize(Vector2D InClientSize) {AppClientSize = MoveTemp(InClientSize);}

		bool AreAllWindowsHidden() const;
		
	protected:
		virtual void Init() {}
		virtual void PostInit() {}
		virtual void ShutDown() {}
		virtual void Update(double DeltaTime) {}

	protected:
		TSharedPtr<SWindow> AppWindow;
		TUniquePtr<Renderer> AppRenderer;
		
		static inline Vector2D AppClientSize = Vector2D(1280, 720);
		static inline double CurrentTime = 0.0;
		static inline double DeltaTime = 1 / 30;
		static inline double FixedDeltaTime = 1 / 30;
	};

	FRAMEWORK_API void UE_Init(const TCHAR* CommandLine);
	FRAMEWORK_API void UE_ShutDown();
}


