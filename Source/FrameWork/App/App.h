#pragma once
namespace FRAMEWORK {
	class FRAMEWORK_API App : public FNoncopyable
	{
	public:
		App(const TCHAR* CommandLine);
		virtual ~App() = default;
	public:
		virtual void Run();

		static double GetCurrentTime() {
			return CurrentTime;
		}

		static void SetCurrentTime(double InTime) {
			CurrentTime = InTime;
		}

		static double GetDeltaTime() {
			return DeltaTime;
		}

		static void SetDeltaTime(double InDeltaTime) {
			DeltaTime = InDeltaTime;
		}

		static double GetFixedDeltaTime() {
			return FixedDeltaTime;
		}

		static void SetFixedDeltaTime(double InFixedDeltaTime) {
			FixedDeltaTime = InFixedDeltaTime;
		}

		static FVector2D& GetClientSize() {
			return AppClientSize;
		}

		static void SetClientSize(FVector2D InClientSize) {
			AppClientSize = MoveTemp(InClientSize);
		}
		
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
		
		static inline FVector2D AppClientSize = FVector2D(1280, 720);
		static inline double CurrentTime = 0.0;
		static inline double DeltaTime = 1 / 30;
		static inline double FixedDeltaTime = 1 / 30;
	};
}


