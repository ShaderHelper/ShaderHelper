#pragma once
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class GizmoShader
	{
	public:
		GizmoShader(const std::set<FString>& VariantDefinitions = {});

		FW::GpuShader* GetMoveVS() const { return MoveVs; }
		FW::GpuShader* GetMoveArrowVS() const { return MoveArrowVs; }
		FW::GpuShader* GetMovePlaneVS() const { return MovePlaneVs; }
		FW::GpuShader* GetRotateVS() const { return RotateVs; }
		FW::GpuShader* GetScaleVS() const { return ScaleVs; }
		FW::GpuShader* GetScaleCubeVS() const { return ScaleCubeVs; }
		FW::GpuShader* GetScaleAllVS() const { return ScaleAllVs; }
		FW::GpuShader* GetPixelShader() const { return Ps; }

		TRefCountPtr<FW::GpuBindGroupLayout> GetBindGroupLayout() const { return BindGroupLayout; }
		TRefCountPtr<FW::GpuBindGroup> GetBindGroup(const FMatrix44f& ViewProjection,
			const FW::Vector3f& GizmoCenter, float GizmoScale, int32 HighlightAxis,
			const FMatrix44f& GizmoOrientation, const FW::Vector3f& CameraPos,
			const FVector2f& ViewportSize);

	private:
		TRefCountPtr<FW::GpuShader> MoveVs;
		TRefCountPtr<FW::GpuShader> MoveArrowVs;
		TRefCountPtr<FW::GpuShader> MovePlaneVs;
		TRefCountPtr<FW::GpuShader> RotateVs;
		TRefCountPtr<FW::GpuShader> ScaleVs;
		TRefCountPtr<FW::GpuShader> ScaleCubeVs;
		TRefCountPtr<FW::GpuShader> ScaleAllVs;
		TRefCountPtr<FW::GpuShader> Ps;
		TRefCountPtr<FW::GpuBindGroupLayout> BindGroupLayout;
		FW::UniformBufferBuilder UbBuilder{FW::UniformBufferUsage::Temp};
	};
}
