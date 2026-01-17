#include "CommonHeader.h"
#include "ShPluginManager.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "AssetObject/Nodes/Texture2dNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPassNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyOutputNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyKeyboardNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPreviousFrameNode.h"
#include "AssetObject/Pins/Pins.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

PYBIND11_EMBEDDED_MODULE(ShaderHelper, m)
{
	auto m_slate = m.def_submodule("Slate");
	RegisterPyFW(m, m_slate);

	py::class_<SH::ShaderToy, FW::Graph, FW::ObjectPtr<SH::ShaderToy>>(m, "ShaderToy")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToy>(Outer); }), py::arg("Outer") = nullptr);
	py::class_<SH::ShaderAsset, FW::AssetObject, FW::ObjectPtr<SH::ShaderAsset>>(m, "ShaderAsset")
		.def_property("EditorContent", 
		[](const SH::ShaderAsset& Self) -> std::string {
			return TCHAR_TO_UTF8(*Self.EditorContent);
		}, 
		[](SH::ShaderAsset& Self, const std::string& InContent) {
			Self.EditorContent = UTF8_TO_TCHAR(InContent.c_str());
		});
	py::class_<SH::StShader, SH::ShaderAsset, FW::ObjectPtr<SH::StShader>>(m, "StShader")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::StShader>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("Language", &SH::StShader::Language);
	py::native_enum<SH::ShaderToyFilterMode>(m, "ShaderToyFilterMode", "enum.Enum")
		.value("Nearest", SH::ShaderToyFilterMode::Nearest)
		.value("Linear", SH::ShaderToyFilterMode::Linear)
		.finalize();
	py::native_enum<SH::ShaderToyWrapMode>(m, "ShaderToyWrapMode", "enum.Enum")
		.value("Clamp", SH::ShaderToyWrapMode::Clamp)
		.value("Repeat", SH::ShaderToyWrapMode::Repeat)
		.finalize();
	py::class_<SH::ShaderToyChannelDesc>(m, "ShaderToyChannel")
		.def_readwrite("Filter", &SH::ShaderToyChannelDesc::Filter)
		.def_readwrite("Wrap", &SH::ShaderToyChannelDesc::Wrap);
	py::class_<SH::ShaderToyPassNode, FW::GraphNode, FW::ObjectPtr<SH::ShaderToyPassNode>>(m, "ShaderToyPassNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToyPassNode>(Outer); }), py::arg("Outer") = nullptr)
		.def(py::init([](FW::ShObject* Outer, FW::AssetPtr<SH::StShader> InShader) { return NewShObject<SH::ShaderToyPassNode>(Outer, InShader); }))
		.def("GetShaderToyCode", &SH::ShaderToyPassNode::GetShaderToyCode)
		.def_readwrite("iChannel0", &SH::ShaderToyPassNode::iChannelDesc0)
		.def_readwrite("iChannel1", &SH::ShaderToyPassNode::iChannelDesc1)
		.def_readwrite("iChannel2", &SH::ShaderToyPassNode::iChannelDesc2)
		.def_readwrite("iChannel3", &SH::ShaderToyPassNode::iChannelDesc3);
	py::class_<SH::ShaderToyOutputNode, FW::GraphNode, FW::ObjectPtr<SH::ShaderToyOutputNode>>(m, "ShaderToyOutputNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToyOutputNode>(Outer); }), py::arg("Outer") = nullptr);
	py::class_<SH::ShaderToyKeyboardNode, FW::GraphNode, FW::ObjectPtr<SH::ShaderToyKeyboardNode>>(m, "ShaderToyKeyboardNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToyKeyboardNode>(Outer); }), py::arg("Outer") = nullptr);
	py::class_<SH::ShaderToyPreviousFrameNode, FW::GraphNode, FW::ObjectPtr<SH::ShaderToyPreviousFrameNode>>(m, "ShaderToyPreviousFrameNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToyPreviousFrameNode>(Outer); }), py::arg("Outer") = nullptr)
		.def("GetPassNode", &SH::ShaderToyPreviousFrameNode::GetPassNode, py::return_value_policy::reference);
	py::class_<SH::Texture2dNode, FW::GraphNode, FW::ObjectPtr<SH::Texture2dNode>>(m, "Texture2dNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::Texture2dNode>(Outer); }), py::arg("Outer") = nullptr)
		.def(py::init([](FW::ShObject* Outer, FW::AssetPtr<FW::Texture2D> InTex) { return NewShObject<SH::Texture2dNode>(Outer, InTex); }))
		.def_property_readonly("Texture", [](const SH::Texture2dNode& Self) { return Self.Texture.Get(); });

	py::class_<SH::GpuTexturePin, FW::GraphPin, FW::ObjectPtr<SH::GpuTexturePin>>(m, "GpuTexturePin")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::GpuTexturePin>(Outer); }), py::arg("Outer") = nullptr);

	py::class_<SH::ShPluginContext>(m, "Context")
		.def_property_readonly_static("CurAssetBrowserDir", [](py::object) { 
			auto ShEditor = static_cast<SH::ShaderHelperEditor*>(FW::GApp->GetEditor());
			return std::string(TCHAR_TO_UTF8(*ShEditor->GetAssetBrowser()->GetCurrentDisplayPath()));
		})
		.def_property_readonly_static("MainWindow", [](py::object) { return SH::ShPluginContext::GetMainWindow(); }, py::return_value_policy::take_ownership)
		.def_property_readonly_static("Graph", [](py::object) { return SH::ShPluginContext::GetGraph(); })
		.def_property_readonly_static("PropertyObject", [](py::object) { return SH::ShPluginContext::GetPropertyObject(); });

}

namespace SH
{
	FW::Graph* ShPluginContext::GetGraph()
	{
		return FW::TSingleton<SH::ShProjectManager>::Get().GetProject()->Graph.Get();
	}

	FW::ShObject* ShPluginContext::GetPropertyObject()
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(FW::GApp->GetEditor());
		return ShEditor->GetCurPropertyObject();
	}

	std::unique_ptr<FW::Window> ShPluginContext::GetMainWindow()
	{
		auto ShEditor = static_cast<SH::ShaderHelperEditor*>(FW::GApp->GetEditor());
		auto Window = std::make_unique<FW::Window>(ShEditor->GetMainWindow());
		return Window;
	}
}

