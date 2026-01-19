#pragma once
#include "Common/Util/SwizzleVector.h"
#include "Editor/Editor.h"
#include "Renderer/Renderer.h"
#include <IDirectoryWatcher.h>
#include <Framework/Application/IInputProcessor.h>

namespace FW {
	class BusyInputBlocker : public IInputProcessor
	{
	public:
		virtual ~BusyInputBlocker() = default;
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override { return true; }
		virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override { return true; }

		virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return true; }
		virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return true; }
		virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return true; }
		virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& WheelEvent, const FPointerEvent* GestureEvent) override { return true; }

		virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override { return true; }
		virtual bool HandleMotionDetectedEvent(FSlateApplication& SlateApp, const FMotionEvent& MotionEvent) override { return true; }
	};

	class FRAMEWORK_API App : public FNoncopyable
	{
	public:
		App(const Vector2D& InClientSize, const TCHAR* CommandLine);
		virtual ~App() = default;

	public:
		void Run();
		bool AreAllWindowsHidden() const;

		Editor* GetEditor() const { return AppEditor.Get(); }
		Renderer* GetRenderer() const { return AppRenderer.Get(); }
		Vector2D GetClientSize() const { return AppClientSize; }
		float GetDeltaTime() const { return DeltaTime; }

		void EnableBusyBlocker();
		void DisableBusyBlocker();

	protected:
		virtual void Update(float DeltaTime);
		virtual void Render();
		virtual void Init();

	public:
		TUniquePtr<Editor> AppEditor;
		TUniquePtr<Renderer> AppRenderer;
		TSharedPtr<BusyInputBlocker> BusyBlocker;

	protected:
		Vector2D AppClientSize;
		FString CommandLine;
		float DeltaTime = 0.01f;
		double FixedDeltaTime = 1 / 30;

		IDirectoryWatcher* DirectoryWatcher{};
	};

    FRAMEWORK_API extern TUniquePtr<App> GApp;
}


