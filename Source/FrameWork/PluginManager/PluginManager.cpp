#include "CommonHeader.h"
#include "PluginManager.h"
#include "AssetObject/Graph.h"
#include "AssetObject/Texture2D.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/AssetImporter/AssetImporter.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "GpuApi/GpuShader.h"

DEFINE_LOG_CATEGORY(LogPy);
class RedirectorStdout
{
public:
	void write(std::string text)
	{
		SH_LOG(LogPy, Display, TEXT("%s"), UTF8_TO_TCHAR(text.c_str()));
	}
	void flush() {}
};


class RedirectorStderr
{
public:
	void write(std::string text)
	{
		SH_LOG(LogPy, Error, TEXT("%s"), UTF8_TO_TCHAR(text.c_str()));
	}
	void flush() {}
};

PYBIND11_EMBEDDED_MODULE(ShRedirector, m)
{
	py::class_< RedirectorStdout >(m, "RedirectorStdout")
		.def(py::init<>())
		.def("write", &RedirectorStdout::write)
		.def("flush", &RedirectorStdout::flush);

	py::class_< RedirectorStderr >(m, "RedirectorStderr")
		.def(py::init<>())
		.def("write", &RedirectorStderr::write)
		.def("flush", &RedirectorStderr::flush);
}


std::vector<py::object> RegisteredMenuEntryExts;
std::vector<py::object> RegisteredPropertyExts;

void RegisterPyFW(py::module_& m, py::module_& m_slate)
{
	py::class_<FW::PathHelper>(m, "PathHelper")
		.def_property_readonly_static("BuiltinDir", [](py::object) { return std::string(TCHAR_TO_UTF8(*FW::PathHelper::BuiltinDir())); })
		.def_property_readonly_static("ResourceDir", [](py::object) { return std::string(TCHAR_TO_UTF8(*FW::PathHelper::ResourceDir())); });

	py::class_< FW::MenuEntryExt, FW::PyMenuEntryExt >(m, "MenuEntryExt")
		.def(py::init<>())
		.def("CanExecute", &FW::MenuEntryExt::CanExecute)
		.def("OnExecute", &FW::MenuEntryExt::OnExecute);
	py::class_< FW::PropertyExt, FW::PyPropertyExt>(m, "PropertyExt")
		.def(py::init<>())
		.def("CreateWidget", &FW::PropertyExt::CreateWidget);

	py::class_<FW::MetaType>(m, "MetaType");
	py::class_<FW::ShObject, FW::ObjectPtr<FW::ShObject>>(m, "ShObject")
		.def_property_readonly("Id", [](const FW::ShObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.GetGuid().ToString())); })
		.def_property_readonly("DynamicMetaType", &FW::ShObject::DynamicMetaType)
		.def_property_readonly("Name", [](const FW::ShObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.ObjectName.ToString())); });
	py::class_<FW::AssetObject, FW::ShObject, FW::ObjectPtr<FW::AssetObject>>(m, "AssetObject")
		.def_property_readonly("FileName", [](const FW::AssetObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.GetFileName())); })
		.def_property_readonly("FileExtension", [](const FW::AssetObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.FileExtension())); });
	py::class_<FW::Texture2D, FW::AssetObject, FW::ObjectPtr<FW::Texture2D>>(m, "Texture2D");
	py::class_<FW::Graph, FW::AssetObject, FW::ObjectPtr<FW::Graph>>(m, "Graph")
		.def("AddNode", &FW::Graph::AddNode)
		.def("AddLink", &FW::Graph::AddLink)
		.def_property_readonly("Nodes", [](const FW::Graph& Self) {
			std::vector<FW::GraphNode*> RetNodes;
			for (const auto& Node : Self.GetNodes())
			{
				RetNodes.push_back(Node);
			}
			return RetNodes;
		});
	py::class_<FW::GraphNode, FW::ShObject, FW::ObjectPtr<FW::GraphNode>>(m, "GraphNode")
		.def("GetPin", [](FW::GraphNode& Self, const std::string& InName) { return Self.GetPin(UTF8_TO_TCHAR(InName.c_str())); })
		.def_property_readonly("InputPins", [](const FW::GraphNode& Self) {
			std::vector<FW::GraphPin*> RetPins;
			for (const auto& Pin : Self.Pins)
			{
				if (Pin->Direction == FW::PinDirection::Input)
				{
					RetPins.push_back(Pin);
				}
			}
			return RetPins;
		});
	py::class_<FW::GraphPin, FW::ShObject, FW::ObjectPtr<FW::GraphPin>>(m, "GraphPin")
		.def_property_readonly("SourceNode", [](const FW::GraphPin& Self) { return Self.GetSourceNode(); });

	m.def("RegisterExt", [](py::object ClassType) {
		if (FW::IsBaseOf(py::type::of<FW::MenuEntryExt>(), ClassType))
		{
			RegisteredMenuEntryExts.push_back(ClassType());
			FW::MenuEntryExt* MenuEntryExt = RegisteredMenuEntryExts.back().cast<FW::MenuEntryExt*>();
			std::string Label = ClassType.attr("__dict__")["Label"].cast<std::string>();
			std::string TargetMenu = ClassType.attr("__dict__")["TargetMenu"].cast<std::string>();
			MenuEntryExt->Label = FString(Label.c_str());
			MenuEntryExt->TargetMenu = FString(TargetMenu.c_str());
			FW::MenuEntryExts.push_back(MenuEntryExt);
		}
		else if (FW::IsBaseOf(py::type::of<FW::PropertyExt>(), ClassType))
		{
			RegisteredPropertyExts.push_back(ClassType());
			FW::PropertyExt* PropertyExt = RegisteredPropertyExts.back().cast<FW::PropertyExt*>();
			py::type TargetType = ClassType.attr("__dict__")["TargetType"];
			PropertyExt->TargetType = TargetType().attr("DynamicMetaType").cast<FW::MetaType*>();
			FW::PropertyExts.push_back(PropertyExt);
		}
	});

	m.def("UnregisterExt", [](py::object ClassType) {
		if (FW::IsBaseOf(py::type::of<FW::MenuEntryExt>(), ClassType)) {
			for (auto it = RegisteredMenuEntryExts.begin(); it != RegisteredMenuEntryExts.end(); ) {
				FW::MenuEntryExt* ptr = it->cast<FW::MenuEntryExt*>();
				auto ext_it = std::find(FW::MenuEntryExts.begin(), FW::MenuEntryExts.end(), ptr);
				if (ext_it != FW::MenuEntryExts.end()) {
					FW::MenuEntryExts.erase(ext_it);
				}
				it = RegisteredMenuEntryExts.erase(it);
			}
		}
		else if (FW::IsBaseOf(py::type::of<FW::PropertyExt>(), ClassType))
		{
			for (auto it = RegisteredPropertyExts.begin(); it != RegisteredPropertyExts.end(); ) {
				FW::PropertyExt* ptr = it->cast<FW::PropertyExt*>();
				auto ext_it = std::find(FW::PropertyExts.begin(), FW::PropertyExts.end(), ptr);
				if (ext_it != FW::PropertyExts.end()) {
					FW::PropertyExts.erase(ext_it);
				}
				it = RegisteredPropertyExts.erase(it);
			}
		}
	});
	py::native_enum<FW::GpuShaderLanguage>(m, "GpuShaderLanguage", "enum.Enum")
		.value("HLSL", FW::GpuShaderLanguage::HLSL)
		.value("GLSL", FW::GpuShaderLanguage::GLSL)
		.export_values()
		.finalize();
	py::native_enum<EHorizontalAlignment>(m_slate, "EHorizontalAlignment", "enum.Enum")
		.value("HAlign_Fill", EHorizontalAlignment::HAlign_Fill)
		.value("HAlign_Center", EHorizontalAlignment::HAlign_Center)
		.value("HAlign_Left", EHorizontalAlignment::HAlign_Left)
		.value("HAlign_Right", EHorizontalAlignment::HAlign_Right)
		.export_values()
		.finalize();
	py::native_enum<EVerticalAlignment>(m_slate, "EVerticalAlignment", "enum.Enum")
		.value("VAlign_Fill", EVerticalAlignment::VAlign_Fill)
		.value("VAlign_Top", EVerticalAlignment::VAlign_Top)
		.value("VAlign_Center", EVerticalAlignment::VAlign_Center)
		.value("VAlign_Bottom", EVerticalAlignment::VAlign_Bottom)
		.export_values()
		.finalize();
	m_slate.def("AddWindow", [](FW::Window* Window, FW::Window* Parent) {
			if (Parent)
			{
				FSlateApplication::Get().AddWindowAsNativeChild(StaticCastSharedRef<SWindow>(Window->GetSlateWidget()),
					StaticCastSharedRef<SWindow>(Parent->GetSlateWidget()), true);
			}
			else
			{
				FSlateApplication::Get().AddWindow(StaticCastSharedRef<SWindow>(Window->GetSlateWidget()), true);
			}
	});
	m_slate.def("DestroyWindow", [](FW::Window* Window) {
		StaticCastSharedRef<SWindow>(Window->GetSlateWidget())->RequestDestroyWindow();
	});
	
	py::class_<FLinearColor>(m, "LinearColor")
		.def(py::init<float, float, float, float >())
		.def_readwrite("R", &FLinearColor::R)
		.def_readwrite("G", &FLinearColor::G)
		.def_readwrite("B", &FLinearColor::B)
		.def_readwrite("A", &FLinearColor::A);
	py::class_<FW::Vector2D>(m, "Vector2D")
		.def(py::init<double, double>())
		.def_readwrite("X", &FW::Vector2D::X)
		.def_readwrite("Y", &FW::Vector2D::Y);
	py::class_<FMargin>(m, "Margin")
		.def(py::init<float, float, float, float > ())
		.def_readwrite("Left", &FMargin::Left)
		.def_readwrite("Top", &FMargin::Top)
		.def_readwrite("Right", &FMargin::Right)
		.def_readwrite("Bottom", &FMargin::Bottom);

	py::class_<FW::Widget, py::smart_holder>(m_slate, "Widget");
	py::class_<FW::Window, FW::Widget, py::smart_holder>(m_slate, "Window")
		.def(py::init<>())
		.def(py::init<std::string>())
		.def_readwrite("Content", &FW::Window::Content)
		.def_readwrite("Size", &FW::Window::Size);
	py::class_<FW::TextBlock, FW::Widget, py::smart_holder>(m_slate, "TextBlock")
		.def(py::init<>())
		.def(py::init<std::string>())
		.def_readwrite("ColorAndOpacity", &FW::TextBlock::ColorAndOpacity)
		.def_readwrite("Text", &FW::TextBlock::Text);
	py::class_<FW::EditableTextBox, FW::Widget, py::smart_holder>(m_slate, "EditableTextBox")
		.def(py::init<>())
		.def("GetText", &FW::EditableTextBox::GetText);
	py::class_<FW::Button, FW::Widget, py::smart_holder>(m_slate, "Button")
		.def(py::init<>())
		.def(py::init<std::string>())
		.def_readwrite("Text", &FW::Button::Text)
		.def_readwrite("OnClicked", &FW::Button::OnClicked)
		.def_readwrite("HAlign", &FW::Button::HAlign)
		.def_readwrite("VAlign", &FW::Button::VAlign);
	py::class_<FW::Slot>(m_slate, "Slot")
		.def("Padding", &FW::Slot::Padding, py::return_value_policy::reference)
		.def("VAlign", &FW::Slot::VAlign, py::return_value_policy::reference)
		.def("HAlign", &FW::Slot::HAlign, py::return_value_policy::reference)
		.def("AutoWidth", &FW::Slot::AutoWidth, py::return_value_policy::reference)
		.def("AutoHeight", &FW::Slot::AutoHeight, py::return_value_policy::reference)
		.def_readwrite("Content", &FW::Slot::Content);
	py::class_<FW::VBox, FW::Widget, py::smart_holder>(m_slate, "VBox")
		.def(py::init<>())
		.def("AddSlot", &FW::VBox::AddSlot, py::return_value_policy::reference);
	py::class_<FW::HBox, FW::Widget, py::smart_holder>(m_slate, "HBox")
		.def(py::init<>())
		.def("AddSlot", &FW::HBox::AddSlot, py::return_value_policy::reference);

	auto m_asset = m.def_submodule("Asset");
	py::class_<FW::AssetImporter>(m_asset, "Importer")
		.def("CreateAsset", [](FW::AssetImporter& Self, const std::string& InFilePath) { 
			return Self.CreateAssetObject(InFilePath.c_str()).Release();
		});
	m_asset.def("SaveToFile", [](FW::AssetObject* InAsset, const std::string& FilePath) {
			if (InAsset)
			{
				FString SavedFilePath = FilePath.c_str();
				TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*SavedFilePath));
				InAsset->ObjectName = FText::FromString(FPaths::GetBaseFilename(SavedFilePath));
				InAsset->Serialize(*Ar);
			}
	});
	m_asset.def("GetImporter", [](const std::string& InPath) { return FW::GetAssetImporter(InPath.c_str()); }, py::return_value_policy::reference);
	m_asset.def("CreateAsset", [](const std::string& InName, py::object ClassType) {
		FW::ObjectPtr<FW::AssetObject> NewAsset = nullptr;
		if (FW::IsBaseOf(py::type::of<FW::AssetObject>(), ClassType))
		{
			FW::MetaType* MetaType = ClassType().attr("DynamicMetaType").cast<FW::MetaType*>();
			NewAsset = NewShObject<FW::AssetObject>(MetaType, nullptr);
			NewAsset->ObjectName = FText::FromString(UTF8_TO_TCHAR(InName.c_str()));
			if (FW::AssetOp* AssetOp = FW::GetAssetOp(MetaType))
			{
				AssetOp->OnCreate(NewAsset);
			}
		}
		return NewAsset;
	});
	m_asset.def("LoadAsset", [](const std::string& InPath) {
		return FW::TSingleton<FW::AssetManager>::Get().LoadAssetByPath<FW::AssetObject>(UTF8_TO_TCHAR(InPath.c_str()));
	});
}


namespace FW
{
	std::vector<MenuEntryExt*> MenuEntryExts;
	std::vector<PropertyExt*> PropertyExts;

	void InitPy()
	{
		FString PyHome = PathHelper::BinariesDir() / "Python";

		PyConfig config;
		PyConfig_InitPythonConfig(&config);
		PyConfig_SetString(&config, &config.home, TCHAR_TO_WCHAR(*PyHome));
		py::initialize_interpreter(&config, 0, nullptr, false);

		py::exec(R"(
import sys
import ShRedirector
sys.stdout = ShRedirector.RedirectorStdout()
sys.stderr = ShRedirector.RedirectorStderr()
		)");

		FString AddPluginPackagePath = FString::Printf(TEXT("sys.path.append('%s')"), *PathHelper::PluginDir());
		py::exec(TCHAR_TO_UTF8(*AddPluginPackagePath));
	}

	bool PyMenuEntryExt::CanExecute()
	{
		try
		{
			PYBIND11_OVERRIDE(bool, MenuEntryExt, CanExecute);
		}
		catch (py::error_already_set& e)
		{
			PrintPyTraceback(e);
			return false;
		}

	}

	void PyMenuEntryExt::OnExecute()
	{
		try
		{
			PYBIND11_OVERRIDE(void, MenuEntryExt, OnExecute);
		}
		catch (py::error_already_set& e)
		{
			PrintPyTraceback(e);
		}

	}

	std::unique_ptr<Widget> PyPropertyExt::CreateWidget()
	{
		try
		{
			PYBIND11_OVERRIDE(std::unique_ptr<Widget>, PropertyExt, CreateWidget);
		}
		catch (py::error_already_set& e)
		{
			PrintPyTraceback(e);
			return {};
		}
	}

}

