#include "CommonHeader.h"
#include "AssetObject/Shader.h"
#include "AssetObject/Material.h"
#include "AssetObject/Model.h"
#include "AssetObject/Render/Nodes/ComputePassNode.h"
#include "AssetObject/Render/Nodes/MeshPassNode.h"
#include "AssetObject/Render/Nodes/RenderOutputNode.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "AssetObject/Render/MeshRenderObject.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "Renderer/ShRenderer.h"
#include "AssetObject/Render/SceneObject.h"
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
#include "Editor/AssetEditor/AssetEditor.h"
#include "AssetObject/ShaderHeader.h"
#include "AssetManager/AssetManager.h"
#include "GpuApi/GpuRhi.h"
#include <IImageWrapperModule.h>
#include <IImageWrapper.h>
#include <Async/Async.h>
#include <Misc/FileHelper.h>
#include "magic_enum.hpp"
#include <future>
#include <stdexcept>

namespace
{
	FW::Vector3f Vector3fFromPython(const py::object& InValue, const char* FieldName)
	{
		py::sequence Sequence = InValue.cast<py::sequence>();
		if (py::len(Sequence) < 3)
		{
			throw std::runtime_error(std::string(FieldName) + " must contain x, y, and z.");
		}
		return FW::Vector3f{
			Sequence[0].cast<float>(),
			Sequence[1].cast<float>(),
			Sequence[2].cast<float>()
		};
	}

	py::tuple Vector3fToPython(const FW::Vector3f& InValue)
	{
		return py::make_tuple(InValue.X, InValue.Y, InValue.Z);
	}

	SH::ShaderStageFlag ShaderStageFlagFromTypes(const std::vector<FW::ShaderType>& InStages)
	{
		SH::ShaderStageFlag Result = SH::ShaderStageFlag::None;
		for (FW::ShaderType Stage : InStages)
		{
			Result |= SH::Shader::StageToFlag(Stage);
		}
		return Result;
	}

	class ScopedLogCapture : public FOutputDevice
	{
	public:
		ScopedLogCapture()
		{
			GLog->AddOutputDevice(this);
		}

		~ScopedLogCapture() override
		{
			GLog->RemoveOutputDevice(this);
		}

		void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			Messages.Add({ FString(V), Verbosity, Category });
		}

		py::list ToPythonList() const
		{
			py::list Result;
			for (const CapturedMessage& Message : Messages)
			{
				auto VerbosityName = magic_enum::enum_name(Message.Verbosity);
				py::dict Item;
				Item["category"] = std::string(TCHAR_TO_UTF8(*Message.Category.ToString()));
				Item["verbosity"] = std::string(VerbosityName.data(), VerbosityName.size());
				Item["message"] = std::string(TCHAR_TO_UTF8(*Message.Text));
				Result.append(Item);
			}
			return Result;
		}

	private:
		struct CapturedMessage
		{
			FString Text;
			ELogVerbosity::Type Verbosity;
			FName Category;
		};

		TArray<CapturedMessage> Messages;
	};

	class ScopedGraphPreviewMode
	{
	public:
		ScopedGraphPreviewMode()
		{
			TargetProject = FW::TSingleton<SH::ShProjectManager>::Get().GetProject().Get();
			PrevScenePreview = TargetProject->bScenePreview;
			TargetProject->bScenePreview = false;
		}

		~ScopedGraphPreviewMode()
		{
			TargetProject->bScenePreview = PrevScenePreview;
		}

		ScopedGraphPreviewMode(const ScopedGraphPreviewMode&) = delete;
		ScopedGraphPreviewMode& operator=(const ScopedGraphPreviewMode&) = delete;

	private:
		SH::ShProject* TargetProject = nullptr;
		bool PrevScenePreview = true;
	};

	FW::AssetPtr<FW::AssetObject> LoadAssetOrThrow(const std::string& InPath)
	{
		auto Asset = FW::TSingleton<FW::AssetManager>::Get().LoadAssetByPath<FW::AssetObject>(UTF8_TO_TCHAR(InPath.c_str()));
		if (!Asset)
		{
			throw std::runtime_error("Failed to load asset.");
		}
		return Asset;
	}

	FW::GpuTexture* GetViewportTexture()
	{
		auto ShEditor = static_cast<SH::ShaderHelperEditor*>(FW::GApp->GetEditor());
		FW::GpuTexture* Texture = ShEditor->GetViewPort()->GetViewPortGpuTexture();
		if (!Texture)
		{
			throw std::runtime_error("ShaderHelper viewport has no rendered texture.");
		}
		return Texture;
	}

	ERGBFormat TextureFormatToRawImageFormat(FW::GpuFormat Format)
	{
		if (Format == FW::GpuFormat::B8G8R8A8_UNORM || Format == FW::GpuFormat::B8G8R8A8_UNORM_SRGB)
		{
			return ERGBFormat::BGRA;
		}
		if (Format == FW::GpuFormat::R8G8B8A8_UNORM)
		{
			return ERGBFormat::RGBA;
		}
		throw std::runtime_error("Viewport capture only supports 8-bit RGBA/BGRA render targets.");
	}

	py::dict CaptureViewportToPng(const std::string& InCapturePath)
	{
		FW::GpuTexture* Texture = GetViewportTexture();
		const uint32 Width = Texture->GetWidth();
		const uint32 Height = Texture->GetHeight();
		const uint32 BytesPerPixel = FW::GetFormatByteSize(Texture->GetFormat());

		ERGBFormat RawFormat = TextureFormatToRawImageFormat(Texture->GetFormat());
		TArray<uint8> RawData;
		const uint32 RowByteSize = Width * BytesPerPixel;
		RawData.SetNumUninitialized(RowByteSize * Height);

		uint32 PaddedRowPitch = 0;
		uint8* PaddedData = static_cast<uint8*>(FW::GGpuRhi->MapGpuTexture(Texture, FW::GpuResourceMapMode::Read_Only, PaddedRowPitch));

		for (uint32 Y = 0; Y < Height; ++Y)
		{
			FMemory::Memcpy(RawData.GetData() + Y * RowByteSize, PaddedData + Y * PaddedRowPitch, RowByteSize);
		}
		FW::GGpuRhi->UnMapGpuTexture(Texture);

		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (!ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), Width, Height, RawFormat, 8))
		{
			throw std::runtime_error("Failed to encode viewport capture as PNG.");
		}

		FString CapturePath = UTF8_TO_TCHAR(InCapturePath.c_str());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(CapturePath), true);
		TArray64<uint8> PngData = ImageWrapper->GetCompressed();
		FFileHelper::SaveArrayToFile(PngData, *CapturePath);

		py::dict Result;
		Result["path"] = InCapturePath;
		Result["width"] = Width;
		Result["height"] = Height;
		return Result;
	}

	py::object TryCaptureViewport(const std::string& InCapturePath, py::dict& InOutResult)
	{
		if (InCapturePath.empty())
		{
			return py::none();
		}

		try
		{
			return CaptureViewportToPng(InCapturePath);
		}
		catch (const std::exception& e)
		{
			InOutResult["success"] = false;
			InOutResult["viewportError"] = std::string(e.what());
			return py::none();
		}
	}

	int RenderFrames(int Frames, float TimeStep)
	{
		auto ShEditor = static_cast<SH::ShaderHelperEditor*>(FW::GApp->GetEditor());
		int RenderedFrames = 0;
		ScopedGraphPreviewMode GraphPreviewMode;
		SH::ScopedTimelineResume TimelineResume;
		auto Project = FW::TSingleton<SH::ShProjectManager>::Get().GetProject();
		for (int Index = 0; Index < FMath::Max(1, Frames); ++Index)
		{
			ShEditor->GetRenderer()->Render();
			Project->TimelineCurTime += TimeStep;
			++RenderedFrames;
		}
		FW::GGpuRhi->WaitGpu();
		return RenderedFrames;
	}

	py::list CollectGraphPassTimes(const FW::Graph& Graph)
	{
		py::list Passes;

		for (FW::GraphNode* Node : Graph.GetNodes())
		{
			if (!Node || Node->GpuTimeMs < 0.0)
			{
				continue;
			}

			py::dict Pass;
			Pass["id"] = std::string(TCHAR_TO_UTF8(*Node->GetGuid().ToString()));
			Pass["name"] = std::string(TCHAR_TO_UTF8(*Node->ObjectName.ToString()));
			Pass["type"] = std::string(TCHAR_TO_UTF8(*FW::GetRegisteredName(Node->DynamicMetaType())));
			Pass["gpuTimeMs"] = Node->GpuTimeMs;
			Passes.append(Pass);
		}

		return Passes;
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

	m.def("OpenAsset", [](const std::string& InPath) {
		FW::AssetOp::OpenAsset(LoadAssetOrThrow(InPath).Get());
		return true;
	}, "Open a ShaderHelper asset in the editor.");

	m.def("RenderFrames", &RenderFrames, py::arg("frames") = 1, py::arg("time_step") = 1.0f / 60.0f,
		"Render the current graph for a fixed number of frames.");

	m.def("CaptureViewport", &CaptureViewportToPng, "Capture the current ShaderHelper viewport render target as a PNG.");

	m.def("RenderGraphFeedback", [](const std::string& InGraphPath, int Frames, const std::string& InCapturePath, float TimeStep) {
		ScopedLogCapture LogCapture;
		auto Asset = LoadAssetOrThrow(InGraphPath);
		FW::Graph* GraphAsset = dynamic_cast<FW::Graph*>(Asset.Get());
		if (!GraphAsset)
		{
			throw std::runtime_error("RenderGraphFeedback requires a graph asset.");
		}

		FW::AssetOp::OpenAsset(Asset.Get());
		py::dict Result;
		Result["success"] = true;
		Result["graph"] = InGraphPath;
		Result["error"] = "";
		const bool bTimestampEnabled = FW::ShowTimestampMs();
		const bool bTimestampSupported = FW::GGpuRhi && FW::GGpuRhi->GetFeature().SupportTimestampQuery();
		bool bResolveFrameRendered = false;
		int RenderedFrames = 0;
		py::list PassTimes;
		try
		{
			RenderedFrames = RenderFrames(Frames, TimeStep);
			PassTimes = CollectGraphPassTimes(*GraphAsset);
			if (py::len(PassTimes) == 0 && bTimestampEnabled && bTimestampSupported)
			{
				RenderedFrames += RenderFrames(1, TimeStep);
				PassTimes = CollectGraphPassTimes(*GraphAsset);
				bResolveFrameRendered = true;
			}
		}
		catch (const std::exception& e)
		{
			Result["success"] = false;
			Result["error"] = std::string(e.what());
			PassTimes = CollectGraphPassTimes(*GraphAsset);
		}
		Result["framesRendered"] = RenderedFrames;
		py::dict Performance;
		Performance["timestampEnabled"] = bTimestampEnabled;
		Performance["timestampSupported"] = bTimestampSupported;
		Performance["resolveFrameRendered"] = bResolveFrameRendered;
		Performance["passes"] = PassTimes;
		Result["performance"] = Performance;
		Result["viewport"] = TryCaptureViewport(InCapturePath, Result);
		Result["logs"] = LogCapture.ToPythonList();
		return Result;
	}, py::arg("graph_path"), py::arg("frames") = 1, py::arg("capture_path") = "", py::arg("time_step") = 1.0f / 60.0f,
		"Open a graph, render frames, capture the viewport, and return logs plus per-pass GPU times.");

	py::class_<FW::Model, FW::AssetObject, FW::ObjectPtr<FW::Model>>(m, "Model")
		.def_property_readonly("SubMeshCount", [](const FW::Model& Self) { return Self.SubMeshCount; })
		.def_property_readonly("VertexCount", [](const FW::Model& Self) { return Self.VertexCount; })
		.def_property_readonly("TriangleFaceCount", [](const FW::Model& Self) { return Self.TriangleFaceCount; });
	py::class_<SH::SceneObject, FW::ShObject, FW::ObjectPtr<SH::SceneObject>>(m, "SceneObject")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::SceneObject>(Outer); }), py::arg("Outer") = nullptr)
		.def("SetName", [](SH::SceneObject& Self, const std::string& InName) {
			Self.ObjectName = FText::FromString(UTF8_TO_TCHAR(InName.c_str()));
		})
		.def("SetParent", &SH::SceneObject::SetParent)
		.def_property("Position",
			[](const SH::SceneObject& Self) { return Vector3fToPython(Self.Position); },
			[](SH::SceneObject& Self, const py::object& InValue) { Self.Position = Vector3fFromPython(InValue, "Position"); })
		.def_property("Rotation",
			[](const SH::SceneObject& Self) { return Vector3fToPython(Self.Rotation); },
			[](SH::SceneObject& Self, const py::object& InValue) { Self.Rotation = Vector3fFromPython(InValue, "Rotation"); })
		.def_property("Scale",
			[](const SH::SceneObject& Self) { return Vector3fToPython(Self.Scale); },
			[](SH::SceneObject& Self, const py::object& InValue) { Self.Scale = Vector3fFromPython(InValue, "Scale"); })
		.def_property_readonly("Parent", [](const SH::SceneObject& Self) { return Self.Parent.Get(); })
		.def_property_readonly("Children", [](const SH::SceneObject& Self) {
			std::vector<SH::SceneObject*> Result;
			for (SH::SceneObject* Child : Self.Children)
			{
				Result.push_back(Child);
			}
			return Result;
		});
	py::class_<SH::MeshSceneObject, SH::SceneObject, FW::ObjectPtr<SH::MeshSceneObject>>(m, "MeshSceneObject")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::MeshSceneObject>(Outer); }), py::arg("Outer") = nullptr)
		.def_property("ModelAsset",
			[](const SH::MeshSceneObject& Self) { return Self.ModelAsset.Get(); },
			[](SH::MeshSceneObject& Self, FW::Model* InModel) { Self.ModelAsset = InModel; })
		.def_readwrite("VertexCount", &SH::MeshSceneObject::VertexCount)
		.def_readwrite("InstanceCount", &SH::MeshSceneObject::InstanceCount);
	py::class_<SH::CameraSceneObject, SH::SceneObject, FW::ObjectPtr<SH::CameraSceneObject>>(m, "CameraSceneObject")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::CameraSceneObject>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("Orthographic", &SH::CameraSceneObject::bOrthographic)
		.def_readwrite("OrthoSize", &SH::CameraSceneObject::OrthoSize)
		.def_readwrite("VerticalFov", &SH::CameraSceneObject::VerticalFov)
		.def_readwrite("NearPlane", &SH::CameraSceneObject::NearPlane)
		.def_readwrite("FarPlane", &SH::CameraSceneObject::FarPlane);
	py::class_<SH::ShaderToy, FW::Graph, FW::ObjectPtr<SH::ShaderToy>>(m, "ShaderToy")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::ShaderToy>(Outer); }), py::arg("Outer") = nullptr)
		.def_readwrite("FlipY", &SH::ShaderToy::FlipY);
	py::class_<SH::Render, FW::Graph, FW::ObjectPtr<SH::Render>>(m, "Render")
		.def(py::init([](FW::ShObject* Outer) { return NewShObject<SH::Render>(Outer); }), py::arg("Outer") = nullptr)
		.def("AddSceneObject", [](SH::Render& Self, SH::SceneObject* Parent) {
			return Self.AddSceneObject<SH::SceneObject>(Parent);
		}, py::arg("parent") = nullptr)
		.def("AddMeshSceneObject", [](SH::Render& Self, SH::SceneObject* Parent) {
			return Self.AddSceneObject<SH::MeshSceneObject>(Parent);
		}, py::arg("parent") = nullptr)
		.def("AddCameraSceneObject", [](SH::Render& Self, SH::SceneObject* Parent) {
			return Self.AddSceneObject<SH::CameraSceneObject>(Parent);
		}, py::arg("parent") = nullptr)
		.def_property_readonly("SceneObjects", [](const SH::Render& Self) {
			std::vector<SH::SceneObject*> Result;
			for (const auto& Object : Self.SceneObjects)
			{
				Result.push_back(Object.Get());
			}
			return Result;
		})
		.def_property("PreviewCamera",
			[](const SH::Render& Self) { return Self.PreviewCamera.Get(); },
			[](SH::Render& Self, SH::CameraSceneObject* InCamera) { Self.PreviewCamera = InCamera; });
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
			Self.MarkDirty(Self.EditorContent != Self.SavedEditorContent);
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
		.def("RefreshShader", [](SH::ShaderAsset& Self) {
			Self.OnShaderRefreshed.Broadcast();
		})
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
	py::class_<SH::MeshRenderObject, FW::ShObject, FW::ObjectPtr<SH::MeshRenderObject>>(m, "MeshRenderObject")
		.def_property("MeshSceneObjectRef",
			[](const SH::MeshRenderObject& Self) { return Self.MeshSceneObjectRef.Get(); },
			[](SH::MeshRenderObject& Self, SH::MeshSceneObject* InMesh) { Self.MeshSceneObjectRef = InMesh; })
		.def_property("MaterialAsset",
			[](const SH::MeshRenderObject& Self) { return Self.MaterialAsset.Get(); },
			[](SH::MeshRenderObject& Self, SH::Material* InMaterial) { Self.MaterialAsset = InMaterial; })
		.def_readwrite("SubMeshIndex", &SH::MeshRenderObject::SubMeshIndex);
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
		.def_property("CameraRef",
			[](const SH::MeshPassNode& Self) { return Self.CameraRef.Get(); },
			[](SH::MeshPassNode& Self, SH::CameraSceneObject* InCamera) { Self.CameraRef = InCamera; })
		.def_property_readonly("MeshRenderObjects", [](const SH::MeshPassNode& Self) {
			std::vector<SH::MeshRenderObject*> Result;
			for (const auto& Object : Self.MeshRenderObjects)
			{
				Result.push_back(Object.Get());
			}
			return Result;
		})
		.def("SetRenderTargetSize", [](SH::MeshPassNode& Self, uint32 Width, uint32 Height) {
			Self.RTSize = { Width, Height };
			Self.OnRenderTargetsChanged();
		})
		.def("SetColorTargetCount", [](SH::MeshPassNode& Self, int32 Count) {
			Self.ColorRTs.SetNum(FMath::Max(Count, 0));
			Self.OnRenderTargetsChanged();
		})
		.def("AddMeshRenderObject", [](SH::MeshPassNode& Self, SH::MeshSceneObject* InMeshSceneObject) {
			const int32 FirstNewIndex = Self.MeshRenderObjects.Num();
			Self.AddMeshRenderObject(InMeshSceneObject);
			std::vector<SH::MeshRenderObject*> Result;
			for (int32 Index = FirstNewIndex; Index < Self.MeshRenderObjects.Num(); ++Index)
			{
				Result.push_back(Self.MeshRenderObjects[Index].Get());
			}
			return Result;
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
