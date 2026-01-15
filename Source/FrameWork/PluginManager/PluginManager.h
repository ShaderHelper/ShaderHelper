#pragma once
#include <memory>
#include "Common/Path/PathHelper.h"
#include "App/App.h"
#include <Serialization/JsonSerializer.h>
#include <Misc/FileHelper.h>

#pragma push_macro("check")
#undef check
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/native_enum.h>
#include <python.h>
#pragma pop_macro("check")

FRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogPy, Log, All);
namespace py = pybind11;

FRAMEWORK_API void RegisterPyFW(py::module_& m, py::module_& m_slate);

namespace FW
{
	inline void PrintPyTraceback(const py::error_already_set& e)
	{
		py::module::import("traceback").attr("print_exception")(e.type(), e.value() ? e.value() : py::none(), e.trace() ? e.trace() : py::none());
	}

	struct ShPlugin
	{
		FString Name;
		FString Author;
		FString Version;
		FString ShVersion;
		FString Desc;
		FString Path;
		bool bActive{};
		bool bFailed{};
	};

	class PluginContext
	{
	public:
		virtual ~PluginContext() = default;
		TArray<ShPlugin> Plugins;
	};
	class FRAMEWORK_API MenuEntryExt
	{
	public:
		virtual ~MenuEntryExt() = default;
		virtual bool CanExecute() { return true;  }
		virtual void OnExecute() {}

		FString Label;
		FString TargetMenu;
	};
	class FRAMEWORK_API PyMenuEntryExt : public MenuEntryExt
	{
	public:
		bool CanExecute() override;
		void OnExecute() override;
	};

	class Widget
	{
	public:
		Widget() = default;
		Widget(TSharedPtr<SWidget> InSlateWidget) : SlateWidget(InSlateWidget) {}
		virtual ~Widget() = default;
		TSharedRef<SWidget> GetSlateWidget() {
			if (!SlateWidget)
			{
				SlateWidget = GetDefaultSlateWidget();
			}
			return SlateWidget.ToSharedRef();
		}
		virtual TSharedRef<SWidget> GetDefaultSlateWidget() = 0;

	protected:
		TSharedPtr<SWidget> SlateWidget;
	};

	class Window : public Widget
	{
	public:
		using Widget::Widget;
		Window(std::string InTitle) : Title(std::move(InTitle)) {}
		virtual TSharedRef<SWidget> GetDefaultSlateWidget() override {
			auto SlateWindow = SNew(SWindow).SupportsMinimize(false).SupportsMaximize(false).IsTopmostWindow(true)
				.ClientSize(Size).Title(FText::FromString(FString(Title.c_str()))).SizingRule(ESizingRule::FixedSize);
			if (Content)
			{
				SlateWindow->SetContent(Content->GetSlateWidget());
			}
			return SlateWindow;
		}

		Vector2D Size{ 10, 10 };
		std::string Title;
		std::unique_ptr<Widget> Content;
	};

	class TextBlock : public Widget
	{
	public:
		using Widget::Widget;
		TextBlock(std::string InText) : Text(std::move(InText)) {}
		virtual TSharedRef<SWidget> GetDefaultSlateWidget() override {
			return SNew(STextBlock).Text_Lambda([this] {
				return FText::FromString(FString(Text.c_str()));
			}).ColorAndOpacity(ColorAndOpacity);
		}

		FLinearColor ColorAndOpacity = FStyleColors::Foreground.GetSpecifiedColor();
		std::string Text;
	};

	class EditableTextBox : public Widget
	{
	public:
		using Widget::Widget;
		virtual TSharedRef<SWidget> GetDefaultSlateWidget() override {
			return SNew(SEditableTextBox);
		}

		std::string GetText() const
		{
			return TCHAR_TO_UTF8(*StaticCastSharedPtr<SEditableTextBox>(SlateWidget)->GetText().ToString());
		}
	};
	
	class Button : public Widget
	{
	public:
		using Widget::Widget;
		Button(std::string InText) : Text(std::move(InText)) {}
		virtual TSharedRef<SWidget> GetDefaultSlateWidget() override {
			return SNew(SButton).Text(FText::FromString(FString(Text.c_str()))).HAlign(HAlign).VAlign(VAlign)
				.OnClicked_Lambda([OnClicked = std::move(OnClicked)] { 
					if (OnClicked)
					{
						try
						{
							OnClicked();
						}
						catch (py::error_already_set& e)
						{
							PrintPyTraceback(e);
						}
					}
					return FReply::Handled(); 
				});
		}

		std::string Text;
		std::function<void()> OnClicked;
		EHorizontalAlignment HAlign = HAlign_Fill;
		EVerticalAlignment VAlign = VAlign_Fill;
	};

	class Slot
	{
	public:
		Slot& AutoWidth() { this->bAutoWidth = true; return *this; }
		Slot& AutoHeight() { this->bAutoHeight = true; return *this; }
		Slot& HAlign(EHorizontalAlignment InAlign) { this->mHAlign = InAlign; return *this; }
		Slot& VAlign(EVerticalAlignment InAlign) { this->mVAlign = InAlign; return *this; }
		Slot& Padding(float Left, float Top, float Right, float Bottom) { mPadding = FMargin{Left, Top, Right, Bottom}; return *this; }

	public:
		bool bAutoWidth{};
		bool bAutoHeight{};
		FMargin mPadding;
		std::shared_ptr<Widget> Content;
		EHorizontalAlignment mHAlign = HAlign_Fill;
		EVerticalAlignment mVAlign = VAlign_Fill;
	};

	class VBox : public Widget
	{
	public:
		using Widget::Widget;
		virtual TSharedRef<SWidget> GetDefaultSlateWidget() override {
			auto Box = SNew(SVerticalBox);
			for (const auto& Slot : Slots)
			{
				auto SlateSlot = Box->AddSlot();
				SlateSlot.HAlign(Slot->mHAlign);
				SlateSlot.VAlign(Slot->mVAlign);
				SlateSlot.Padding(Slot->mPadding);
				if (Slot->bAutoHeight)
				{
					SlateSlot.AutoHeight();
				}
				if (Slot->Content)
				{
					SlateSlot.AttachWidget(Slot->Content->GetSlateWidget());
				}
			}
			return Box;
		}

		Slot* AddSlot(std::unique_ptr<Widget> Content)
		{
			auto Item = std::make_unique<Slot>();
			Item->Content = std::move(Content);
			Slots.push_back(std::move(Item));
			return Slots.back().get();
		}

		std::vector<std::unique_ptr<Slot>> Slots;
	};

	class HBox : public Widget
	{
	public:
		using Widget::Widget;
		virtual TSharedRef<SWidget> GetDefaultSlateWidget() override {
			auto Box = SNew(SHorizontalBox);
			for(const auto& Slot : Slots)
			{
				auto SlateSlot = Box->AddSlot();
				SlateSlot.HAlign(Slot->mHAlign);
				SlateSlot.VAlign(Slot->mVAlign);
				SlateSlot.Padding(Slot->mPadding);
				if (Slot->bAutoWidth)
				{
					SlateSlot.AutoWidth();
				}
				if(Slot->Content)
				{
					SlateSlot.AttachWidget(Slot->Content->GetSlateWidget());
				}
			}
			return Box;
		}

		Slot* AddSlot(std::shared_ptr<Widget> Content)
		{
			auto Item = std::make_unique<Slot>();
			Item->Content = std::move(Content);
			Slots.push_back(std::move(Item));
			return Slots.back().get();
		}

		std::vector<std::unique_ptr<Slot>> Slots;
	};

	class FRAMEWORK_API PropertyExt
	{
	public:
		virtual ~PropertyExt() = default;
		virtual std::unique_ptr<Widget> CreateWidget() { return {}; }

		FW::MetaType* TargetType;
	};
	class FRAMEWORK_API PyPropertyExt : public PropertyExt
	{
	public:
		std::unique_ptr<Widget> CreateWidget() override;
	};


	FRAMEWORK_API extern std::vector<MenuEntryExt*> MenuEntryExts;
	FRAMEWORK_API extern std::vector<PropertyExt*> PropertyExts;
	FRAMEWORK_API void InitPy();

	inline bool IsBaseOf(py::type BaseType, py::type DerivedType)
	{
		return py::bool_(py::module_::import("builtins").attr("issubclass")(DerivedType, BaseType));
	}

	template<typename ContextType>
	class PluginManager
	{
	public:
		PluginManager()
		{
			InitPy();
			TArray<FString> PluginFolders;
			IFileManager::Get().FindFiles(PluginFolders, *(PathHelper::PluginDir() / TEXT("*")), false, true);

			TArray<FString> ActivePlugins;
			Editor::GetEditorConfig()->GetArray(TEXT("Common"), TEXT("ActivePlugins"), ActivePlugins);
			for (const auto& PluginFolder : PluginFolders)
			{
				FString PluginMetaData = PathHelper::PluginDir() / PluginFolder / "Meta.json";
				if (IFileManager::Get().FileExists(*PluginMetaData))
				{
					ShPlugin Plugin{ .Name = PluginFolder, .Path = PathHelper::PluginDir() / PluginFolder, .bActive = ActivePlugins.Contains(PluginFolder) };

					TSharedPtr<FJsonObject> RootJsonObject;
					FString JsonContent;
					FFileHelper::LoadFileToString(JsonContent, *PluginMetaData);

					TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
					FJsonSerializer::Deserialize(Reader, RootJsonObject);

					RootJsonObject->TryGetStringField("description", Plugin.Desc);
					RootJsonObject->TryGetStringField("author", Plugin.Author);
					RootJsonObject->TryGetStringField("version", Plugin.Version);
					RootJsonObject->TryGetStringField("shaderhelper", Plugin.ShVersion);
					RootJsonObject->TryGetStringField("description", Plugin.Desc);
					Context.Plugins.Add(MoveTemp(Plugin));
				}
				else
				{
					SH_LOG(LogPy, Warning, TEXT("failed to find the plugin(%s):"), *PluginFolder);
				}
			}
		}

		void RegisterActivePlugins()
		{
			for (auto& Plugin : Context.Plugins)
			{
				if (Plugin.bActive)
				{
					RegisterPlugin(Plugin);
				}
			}
		}
		void RegisterPlugin(ShPlugin& InPlugin)
		{
			try
			{
				py::module_ PluginModule = py::module_::import(TCHAR_TO_UTF8(*InPlugin.Name));
				PluginModule.reload();
				PluginModule.attr("Register")();
				InPlugin.bFailed = false;
			}
			catch (py::error_already_set& e)
			{
				SH_LOG(LogPy, Error, TEXT("failed to register the plugin(%s):"), *InPlugin.Name);
				PrintPyTraceback(e);
				InPlugin.bFailed = true;
			}
		}
		void UnregisterPlugin(ShPlugin& InPlugin)
		{
			try
			{
				py::module_ PluginModule = py::module_::import(TCHAR_TO_UTF8(*InPlugin.Name));
				PluginModule.reload();
				PluginModule.attr("Unregister")();
			}
			catch (py::error_already_set& e)
			{
				SH_LOG(LogPy, Error, TEXT("failed to unregister the plugin(%s):"), *InPlugin.Name);
				PrintPyTraceback(e);
			}
		}

		TArray<ShPlugin>& GetPlugins() { return Context.Plugins; };
		const ContextType& GetContext() const { return Context; }

	private:
		ContextType Context;
	};
}
