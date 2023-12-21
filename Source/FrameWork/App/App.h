#pragma once
#include "Common/Util/SwizzleVector.h"
#include "Renderer/Renderer.h"
namespace FRAMEWORK {
	class FRAMEWORK_API App : public FNoncopyable
	{
	public:
		App(const Vector2D& InClientSize, const TCHAR* CommandLine);
		virtual ~App();

	public:
		void Run();
		bool AreAllWindowsHidden() const;
		
	protected:
		virtual void Update(double DeltaTime);
		virtual void Render();

	protected:		
		Vector2D AppClientSize;
		double DeltaTime;
		double FixedDeltaTime = 1 / 30;

		FString SavedCommandLine;
	};
}


