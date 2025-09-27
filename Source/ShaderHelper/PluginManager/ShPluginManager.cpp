#include "CommonHeader.h"
#include "ShPluginManager.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "AssetObject/Nodes/Texture2dNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPassNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyOutputNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyKeyboardNode.h"
#include "AssetObject/Pins/Pins.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

PYBIND11_EMBEDDED_MODULE(ShaderHelper, m)
{
	RegisterPyFW(m);
	py::class_<SH::ShaderToy, FW::Graph>(m, "ShaderToy")
		.def(py::init<>());;
	py::native_enum<SH::ShaderToyFilterMode>(m, "ShaderToyFilterMode", "enum.Enum")
		.value("Nearest", SH::ShaderToyFilterMode::Nearest)
		.value("Linear", SH::ShaderToyFilterMode::Linear)
		.finalize();
	py::native_enum<SH::ShaderToyWrapMode>(m, "ShaderToyWrapMode", "enum.Enum")
		.value("Clamp", SH::ShaderToyWrapMode::Clamp)
		.value("Repeat", SH::ShaderToyWrapMode::Repeat)
		.finalize();
	py::class_<SH::ShaderToyChannelDesc>(m, "ShaderToyChannel")
		.def_readonly("Filter", &SH::ShaderToyChannelDesc::Filter)
		.def_readonly("Wrap", &SH::ShaderToyChannelDesc::Wrap);
	py::class_<SH::ShaderToyPassNode, FW::GraphNode>(m, "ShaderToyPassNode")
		.def(py::init<>())
		.def("GetShaderToyCode", &SH::ShaderToyPassNode::GetShaderToyCode)
		.def_readonly("iChannel0", &SH::ShaderToyPassNode::iChannelDesc0)
		.def_readonly("iChannel1", &SH::ShaderToyPassNode::iChannelDesc1)
		.def_readonly("iChannel2", &SH::ShaderToyPassNode::iChannelDesc2)
		.def_readonly("iChannel3", &SH::ShaderToyPassNode::iChannelDesc3);
	py::class_<SH::ShaderToyOutputNode, FW::GraphNode>(m, "ShaderToyOutputNode")
		.def(py::init<>());
	py::class_<SH::ShaderToyKeyboardNode, FW::GraphNode>(m, "ShaderToyKeyboardNode")
		.def(py::init<>());
	py::class_<SH::Texture2dNode, FW::GraphNode>(m, "Texture2dNode")
		.def(py::init<>())
		.def_property_readonly("Texture", [](const SH::Texture2dNode& Self) { return Self.Texture.Get(); });

	py::class_<SH::GpuTexturePin, FW::GraphPin>(m, "GpuTexturePin");

	py::class_<SH::ShPluginContext>(m, "Context")
		.def_property_readonly_static("CurAssetBrowserDir", [](py::object) { 
			auto ShEditor = static_cast<SH::ShaderHelperEditor*>(FW::GApp->GetEditor());
			return std::string(TCHAR_TO_UTF8(*ShEditor->GetAssetBrowser()->GetCurrentDisplayPath()));
		})
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
}

