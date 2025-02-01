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
		float GetDeltaTime() const { return DeltaTime; }
		
	protected:
		virtual void Update(float DeltaTime);
		virtual void Render();
		virtual void Init();
        
    public:
        TUniquePtr<Editor> AppEditor;
        TUniquePtr<Renderer> AppRenderer;

	protected:
		Vector2D AppClientSize;
		FString CommandLine;
		float DeltaTime = 0.01f;
		double FixedDeltaTime = 1 / 30;
	};

    FRAMEWORK_API extern TUniquePtr<App> GApp;
}


