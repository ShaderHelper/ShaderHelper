#pragma once
#include "CommonHeader.h"

namespace SH {
	
	class ShaderHelperApp
	{
	public:
		ShaderHelperApp(const TCHAR* CommandLine);
	public:

		void Init();
		void ShutDown();

		void Update(double DeltaTime);

		void Run();

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
	public:
		static inline const FVector2D DefaultClientSize = FVector2D(1280, 720);

	private:
		static inline double CurrentTime = 0.0;
		static inline double DeltaTime = 1 / 30;
		static inline double FixedDeltaTime = 1 / 30;
	};
	
}


