#include "CommonHeader.h"
#include "AssetObject/Shader.h"
#include "AssetObject/Material.h"
#include "AssetObject/Render/Nodes/ComputePassNode.h"
#include "AssetObject/Render/Nodes/MeshPassNode.h"
#include "AssetObject/Render/Nodes/RenderOutputNode.h"
#include "AssetObject/Render/Render.h"
#include "ShPluginManager.h"
#include "AssetObject/ShaderToy/ShaderToy.h"
#include "AssetObject/Nodes/Texture2dNode.h"
#include "AssetObject/Nodes/TextureCubeNode.h"
#include "AssetObject/Nodes/Texture3dNode.h"
#include "AssetObject/ShaderToy/StShader.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPassNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyOutputNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyKeyboardNode.h"
#include "AssetObject/ShaderToy/Nodes/ShaderToyPreviousFrameNode.h"
#include "AssetObject/Pins/Pins.h"
#include "Editor/ShaderHelperEditor.h"
#include "AssetObject/ShaderHeader.h"
#include <Async/Async.h>
#include <future>
#include <stdexcept>

namespace
{
	SH::ShaderStageFlag ShaderStageFlagFromTypes(const std::vector<FW::ShaderType>& InStages)
	{
		SH::ShaderStageFlag Result = SH::ShaderStageFlag::None;
		for (FW::ShaderType Stage : InStages)
		{
			Result |= SH::Shader::StageToFlag(Stage);
		}
		return Result;
	}

}

PYBIND11_EMBEDDED_MODULE(ShaderHelper, m)
{
	auto m_slate = m.def_submodule("Slate");
	RegisterPyFW(m, m_slate);
	m.def("Run", [](py::object Callable) -> py::object {
		auto CallableObject = std::make_shared<py::object>(MoveTemp(Callable));
		auto Promise = std::make_shared<std::promise<std::shared_ptr<py::object>>>();
		std::future<std::shared_ptr<py::object>> Future = Promise->get_future();
		AsyncTask(ENamedThreads::GameThread, [CallableObject, Promise]() mutable {
			py::gil_scoped_acquire Acquire;
			try
			{
				py::object Result = (*CallableObject)();
				CallableObject.reset();
				Promise->set_value(std::make_shared<py::object>(MoveTemp(Result)));
			}
			catch (...)
			{
				CallableObject.reset();
				Promise->set_exception(std::current_exception());
			}
		});

		std::shared_ptr<py::object> ResultObject;
		{
			py::gil_scoped_release Release;
			ResultObject = Future.get();
		}
		py::object Result = MoveTemp(*ResultObject);
		ResultObject.reset();
		return Result;
	}, "Run a Python callable on ShaderHelper's main thread and return its result.");

	py::class_<SH::ShaderToy, FW::Graph, FW::ObjectPtr<SH::ShaderToy>>(m, "ShaderToy")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToy>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("FlipY", &SH::ShaderToy::FlipY);
	py::class_<SH::Render, FW::Graph, FW::ObjectPtr<SH::Render>>(m, "Render")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::Render>(Outer); }), py::arg("Outer") = nullptr);
	py::native_enum<FW::ShaderType>(m, "ShaderType", "enum.Enum")
		.value("Vertex", FW::ShaderType::Vertex)
		.value("Pixel", FW::ShaderType::Pixel)
		.value("Compute", FW::ShaderType::Compute)
		.finalize();
	py::class_<SH::ShaderAsset, FW::AssetObject, FW::ObjectPtr<SH::ShaderAsset>>(m, "ShaderAsset")
		.def_property("EditorContent", 
		[](const SH::ShaderAsset& Self) -> std::string {
			return TCHAR_TO_UTF8(*Self.EditorContent);
		}, 
		[](SH::ShaderAsset& Self, const std::string& InContent) {
			Self.EditorContent = UTF8_TO_TCHAR(InContent.c_str());
		})
		.def("IsCompilationSuccessful", &SH::ShaderAsset::IsCompilationSuccessful)
		.def("GetEnabledStages", [](const SH::ShaderAsset& Self) {
			std::vector<FW::ShaderType> Result;
			for (FW::ShaderType Stage : Self.GetEnabledStageList())
			{
				Result.push_back(Stage);
			}
			return Result;
		})
		.def("GetFullContent", [](const SH::ShaderAsset& Self) {
			return std::string(TCHAR_TO_UTF8(*Self.GetFullContent()));
		})
		.def("GetExtraLineNum", &SH::ShaderAsset::GetExtraLineNum)
		.def("CompileShader", [](SH::ShaderAsset& Self) {
			FString ErrorInfo;
			FString WarnInfo;
			bool bSucceeded = Self.CompileShader(ErrorInfo, WarnInfo);
			py::dict Result;
			Result["success"] = bSucceeded;
			Result["error"] = std::string(TCHAR_TO_UTF8(*ErrorInfo));
			Result["warning"] = std::string(TCHAR_TO_UTF8(*WarnInfo));
			return Result;
		});
	py::class_<SH::StShader, SH::ShaderAsset, FW::ObjectPtr<SH::StShader>>(m, "StShader")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::StShader>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("Language", &SH::StShader::Language)
		.def_property("ChannelSlotTypes",
			[](const SH::StShader& Self) {
				return py::make_tuple(Self.ChannelSlotTypes[0], Self.ChannelSlotTypes[1], Self.ChannelSlotTypes[2], Self.ChannelSlotTypes[3]);
			},
			[](SH::StShader& Self, py::tuple t) {
				for (int i = 0; i < 4 && i < py::len(t); i++)
					Self.ChannelSlotTypes[i] = t[i].cast<SH::ShaderToySlotType>();
			});
	py::class_<SH::Shader, SH::ShaderAsset, FW::ObjectPtr<SH::Shader>>(m, "Shader")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::Shader>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("Language", &SH::Shader::Language)
		.def("SetStages", [](SH::Shader& Self, const std::vector<FW::ShaderType>& InStages) {
			Self.EnabledStages = ShaderStageFlagFromTypes(InStages);
		})
		.def("GetStages", [](const SH::Shader& Self) {
			std::vector<FW::ShaderType> Result;
			for (FW::ShaderType Stage : Self.GetEnabledStageList())
			{
				Result.push_back(Stage);
			}
			return Result;
		})
		.def("SetEntryPoint", [](SH::Shader& Self, FW::ShaderType InStage, const std::string& InEntryPoint) {
			Self.EntryPoints[(int32)InStage] = UTF8_TO_TCHAR(InEntryPoint.c_str());
		})
		.def("SetStageMacros", [](SH::Shader& Self, FW::ShaderType InStage, const std::vector<std::string>& InMacros) {
			TArray<FString>& Macros = Self.StageMacros[(int32)InStage];
			Macros.Empty();
			for (const std::string& Macro : InMacros)
			{
				Macros.Add(UTF8_TO_TCHAR(Macro.c_str()));
			}
		});
	py::class_<SH::ShaderHeader, SH::ShaderAsset, FW::ObjectPtr<SH::ShaderHeader>>(m, "ShaderHeader")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderHeader>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("Language", &SH::ShaderHeader::Language);
	py::class_<SH::Material, FW::AssetObject, FW::ObjectPtr<SH::Material>>(m, "Material")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::Material>(Outer); }), py::arg("Outer") = nullptr)
		.def_property("VertexShaderAsset",
			[](const SH::Material& Self) { return Self.VertexShaderAsset.Get(); },
			[](SH::Material& Self, SH::Shader* InShader) {
				Self.VertexShaderAsset = InShader;
				Self.RefreshShaderBindings();
			})
		.def_property("PixelShaderAsset",
			[](const SH::Material& Self) { return Self.PixelShaderAsset.Get(); },
			[](SH::Material& Self, SH::Shader* InShader) {
				Self.PixelShaderAsset = InShader;
				Self.RefreshShaderBindings();
			})
		.def("RefreshShaderBindings", &SH::Material::RefreshShaderBindings);
	py::native_enum<SH::ShaderToySlotType>(m, "ShaderToySlotType", "enum.Enum")
		.value("Texture2D", SH::ShaderToySlotType::Texture2D)
		.value("TextureCube", SH::ShaderToySlotType::TextureCube)
		.value("Texture3D", SH::ShaderToySlotType::Texture3D)
		.finalize();
	py::native_enum<SH::ShaderToyFilterMode>(m, "ShaderToyFilterMode", "enum.Enum")
		.value("Nearest", SH::ShaderToyFilterMode::Nearest)
		.value("Linear", SH::ShaderToyFilterMode::Linear)
		.value("Mipmap", SH::ShaderToyFilterMode::Mipmap)
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
	py::class_<SH::RenderOutputNode, FW::GraphNode, FW::ObjectPtr<SH::RenderOutputNode>>(m, "RenderOutputNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::RenderOutputNode>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("Layer", &SH::RenderOutputNode::Layer)
		.def_readwrite("AreaFraction", &SH::RenderOutputNode::AreaFraction);
	py::class_<SH::MeshPassNode, FW::GraphNode, FW::ObjectPtr<SH::MeshPassNode>>(m, "MeshPassNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::MeshPassNode>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("DepthEnabled", &SH::MeshPassNode::bDepthEnabled)
		.def("SetRenderTargetSize", [](SH::MeshPassNode& Self, uint32 Width, uint32 Height) {
			Self.RTSize = { Width, Height };
			Self.OnRenderTargetsChanged();
		})
		.def("SetColorTargetCount", [](SH::MeshPassNode& Self, int32 Count) {
			Self.ColorRTs.SetNum(FMath::Max(Count, 0));
			Self.OnRenderTargetsChanged();
		});
	py::class_<SH::ComputePassNode, FW::GraphNode, FW::ObjectPtr<SH::ComputePassNode>>(m, "ComputePassNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ComputePassNode>(Outer); }), py::arg("Outer") = nullptr)
		.def(py::init([](FW::ShObject* Outer, FW::AssetPtr<SH::Shader> InShader) { return NewShObject<SH::ComputePassNode>(Outer, InShader); }))
		.def_property("ShaderAsset",
			[](const SH::ComputePassNode& Self) { return Self.ShaderAsset.Get(); },
			[](SH::ComputePassNode& Self, SH::Shader* InShader) {
				Self.ShaderAsset = InShader;
				Self.Init();
			})
		.def("SetThreadGroupCount", [](SH::ComputePassNode& Self, uint32 X, uint32 Y, uint32 Z) {
			Self.ThreadGroupCount = { X, Y, Z };
		});
	py::class_<SH::ShaderToyKeyboardNode, FW::GraphNode, FW::ObjectPtr<SH::ShaderToyKeyboardNode>>(m, "ShaderToyKeyboardNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToyKeyboardNode>(Outer); }), py::arg("Outer") = nullptr);
	py::class_<SH::ShaderToyPreviousFrameNode, FW::GraphNode, FW::ObjectPtr<SH::ShaderToyPreviousFrameNode>>(m, "ShaderToyPreviousFrameNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToyPreviousFrameNode>(Outer); }), py::arg("Outer") = nullptr)
		.def(py::init([](FW::ShObject* Outer, SH::ShaderToyPassNode* InPassNode) { return NewShObject<SH::ShaderToyPreviousFrameNode>(Outer, InPassNode); }))
		.def("GetPassNode", &SH::ShaderToyPreviousFrameNode::GetPassNode, py::return_value_policy::reference);
	py::class_<SH::Texture2dNode, FW::GraphNode, FW::ObjectPtr<SH::Texture2dNode>>(m, "Texture2dNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::Texture2dNode>(Outer); }), py::arg("Outer") = nullptr)
		.def(py::init([](FW::ShObject* Outer, FW::AssetPtr<FW::Texture2D> InTex) { return NewShObject<SH::Texture2dNode>(Outer, InTex); }))
		.def_property_readonly("Texture", [](const SH::Texture2dNode& Self) { return Self.Texture.Get(); });
	py::class_<SH::TextureCubeNode, FW::GraphNode, FW::ObjectPtr<SH::TextureCubeNode>>(m, "TextureCubeNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::TextureCubeNode>(Outer); }), py::arg("Outer") = nullptr)
		.def(py::init([](FW::ShObject* Outer, FW::AssetPtr<FW::TextureCube> InTexture) { return NewShObject<SH::TextureCubeNode>(Outer, InTexture); }))
		.def_property_readonly("Texture", [](const SH::TextureCubeNode& Self) { return Self.Texture.Get(); });
	py::class_<SH::Texture3dNode, FW::GraphNode, FW::ObjectPtr<SH::Texture3dNode>>(m, "Texture3dNode")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::Texture3dNode>(Outer); }), py::arg("Outer") = nullptr)
		.def(py::init([](FW::ShObject* Outer, FW::AssetPtr<FW::Texture3D> InTexture) { return NewShObject<SH::Texture3dNode>(Outer, InTexture); }))
		.def_property_readonly("Texture", [](const SH::Texture3dNode& Self) { return Self.Texture.Get(); });

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
