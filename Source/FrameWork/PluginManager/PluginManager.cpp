#include "CommonHeader.h"
#include "PluginManager.h"
#include "AssetObject/Graph.h"
#include "AssetObject/Texture2D.h"
#include "AssetManager/AssetImporter/AssetImporter.h"

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

void RegisterPyFW(py::module_& m)
{
	py::class_<FW::PathHelper>(m, "PathHelper")
		.def_property_readonly_static("ResourceDir", [](py::object) { return std::string(TCHAR_TO_UTF8(*FW::PathHelper::ResourceDir())); });

	py::class_< FW::MenuEntryExt, FW::PyMenuEntryExt >(m, "MenuEntryExt")
		.def(py::init<>())
		.def("CanExecute", &FW::MenuEntryExt::CanExecute)
		.def("OnExecute", &FW::MenuEntryExt::OnExecute);
	py::class_< FW::PropertyExt, FW::PyPropertyExt>(m, "PropertyExt")
		.def(py::init<>())
		.def("CreateWidget", &FW::PropertyExt::CreateWidget);

	py::class_<FW::MetaType>(m, "MetaType");
	py::class_<FW::ShObject>(m, "ShObject")
		.def_property_readonly("Id", [](const FW::ShObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.GetGuid().ToString())); })
		.def_property_readonly("DynamicMetaType", &FW::ShObject::DynamicMetaType)
		.def_property_readonly("Name", [](const FW::ShObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.ObjectName.ToString())); });
	py::class_<FW::AssetObject, FW::ShObject>(m, "AssetObject")
		.def_property_readonly("FileName", [](const FW::AssetObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.GetFileName())); })
		.def_property_readonly("FileExtension", [](const FW::AssetObject& Self) { return std::string(TCHAR_TO_UTF8(*Self.FileExtension())); });
	py::class_<FW::Texture2D, FW::AssetObject>(m, "Texture2D");
	py::class_<FW::Graph, FW::AssetObject>(m, "Graph")
		.def_property_readonly("Nodes", [](const FW::Graph& Self) {
			std::vector<FW::GraphNode*> RetNodes;
			for (const auto& Node : Self.GetNodes())
			{
				RetNodes.push_back(Node);
			}
			return RetNodes;
		});
	py::class_<FW::GraphNode, FW::ShObject>(m, "GraphNode")
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
	py::class_<FW::GraphPin, FW::ShObject>(m, "GraphPin")
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

	auto m_slate = m.def_submodule("Slate");
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

	py::class_<FW::Widget, py::smart_holder>(m_slate, "Widget");
	py::class_<FW::Button, FW::Widget, py::smart_holder>(m_slate, "Button")
		.def(py::init<>())
		.def_readwrite("Text", &FW::Button::Text)
		.def_readwrite("OnClicked", &FW::Button::OnClicked)
		.def_readwrite("HAlign", &FW::Button::HAlign)
		.def_readwrite("VAlign", &FW::Button::VAlign);
	py::class_<FW::Slot>(m_slate, "Slot");
	py::class_<FW::HBox, FW::Widget, py::smart_holder>(m_slate, "HBox");

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

