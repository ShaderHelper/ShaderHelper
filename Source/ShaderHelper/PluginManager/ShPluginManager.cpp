#include "CommonHeader.h"
#include "ShPluginManager.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "AssetObject/Nodes/Texture2dNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPassNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyOutputNode.h"
#include "AssetObject/Pins/Pins.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

PYBIND11_EMBEDDED_MODULE(ShaderHelper, m)
{
	RegisterPyFW(m);
	py::class_<SH::ShaderToy, FW::Graph>(m, "ShaderToy")
		.def(py::init<>());;
	py::class_<SH::ShaderToyPassNode, FW::GraphNode>(m, "ShaderToyPassNode")
		.def(py::init<>())
		.def("GetShaderToyCode", &SH::ShaderToyPassNode::GetShaderToyCode);
	py::class_<SH::ShaderToyOutputNode, FW::GraphNode>(m, "ShaderToyOutputNode")
		.def(py::init<>());
	py::class_<SH::Texture2dNode, FW::GraphNode>(m, "Texture2dNode")
		.def(py::init<>());

	py::class_<SH::GpuTexturePin, FW::GraphPin>(m, "GpuTexturePin");

	py::class_<SH::ShPluginContext>(m, "Context")
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

