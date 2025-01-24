#pragma once
#include "Common/Util/SwizzleVector.h"
#include "Editor/Editor.h"
#include "Renderer/Renderer.h"
namespace FW {
	class FRAMEWORK_API App : public FNoncopyable
	{
	public:
		App(const Vector2D& InClientSize, const TCHAR* CommandLine);
		virtual ~App();

	public:
		void Run();
		bool AreAllWindowsHidden() const;
        
        Editor* GetEditor() const { return AppEditor.Get(); }
        Renderer* GetRenderer() const { return AppRenderer.Get(); }
		Vector2D GetClientSize() const { return AppClientSize; }
		double GetDeltaTime() const { return DeltaTime; }
		
	protected:
		virtual void Update(double DeltaTime);
		virtual void Render();
		virtual void Init();
        
    public:
        TUniquePtr<Editor> AppEditor;
        TUniquePtr<Renderer> AppRenderer;

	protected:
		Vector2D AppClientSize;
		FString CommandLine;
		double DeltaTime;
		double FixedDeltaTime = 1 / 30;
	};

    FRAMEWORK_API extern TUniquePtr<App> GApp;
}


