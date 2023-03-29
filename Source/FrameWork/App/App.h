#pragma once
#include "Common/Util/SwizzleVector.h"
#include "Renderer/Renderer.h"
namespace FRAMEWORK {
	class FRAMEWORK_API App : public FNoncopyable
	{
	public:
		App(const TCHAR* CommandLine);
		virtual ~App() = default;
	public:
		virtual void Run();

		Renderer* GetRenderer() const { return AppRenderer.Get();}
		void SetRenderer(TUniquePtr<Renderer> InRenderer) { AppRenderer = MoveTemp(InRenderer); }

		static double GetCurrentTime() { return CurrentTime; }
		static void SetCurrentTime(double InTime) { CurrentTime = InTime; }

		static double GetDeltaTime() { return DeltaTime; }
		static void SetDeltaTime(double InDeltaTime) { DeltaTime = InDeltaTime; }

		static double GetFixedDeltaTime() { return FixedDeltaTime; }
		static void SetFixedDeltaTime(double InFixedDeltaTime) { FixedDeltaTime = InFixedDeltaTime;}

		static Vector2D GetClientSize() { return AppClientSize; }
		static void SetClientSize(Vector2D InClientSize) {AppClientSize = MoveTemp(InClientSize);}
		
	protected:
		virtual void Init();
		virtual void PostInit();
		virtual void ShutDown();
		virtual void Update(double DeltaTime);

	private:
		void UE_Init();
		void UE_ShutDown();
		void UE_Update();

	protected:
		TSharedPtr<SWindow> AppWindow;
		TUniquePtr<Renderer> AppRenderer;
		
		static inline Vector2D AppClientSize = Vector2D(1280, 720);
		static inline double CurrentTime = 0.0;
		static inline double DeltaTime = 1 / 30;
		static inline double FixedDeltaTime = 1 / 30;
	};
}


