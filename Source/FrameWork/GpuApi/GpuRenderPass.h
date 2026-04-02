#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"

namespace FW
{
	class GpuQuerySet;

	struct GpuViewPortDesc
	{
		float Width;
		float Height;

		float ZMin = 0.0f;
		float ZMax = 1.0f;
		float TopLeftX = 0.0f;
		float TopLeftY = 0.0f;
	};

	struct GpuScissorRectDesc
	{
		uint32 Left;
		uint32 Top;
		uint32 Right;
		uint32 Bottom;
	};

	struct GpuRenderTargetInfo
	{
		GpuTextureView* View;
		RenderTargetLoadAction LoadAction = RenderTargetLoadAction::DontCare;
		RenderTargetStoreAction StoreAction = RenderTargetStoreAction::DontCare;

        //only valid when the load action is clear.
		Vector4f ClearColor = View->GetTexture()->GetResourceDesc().ClearValues;
		GpuTextureView* ResolveTarget = nullptr;
	};

	struct GpuDepthStencilTargetInfo
	{
		GpuTextureView* View;
		RenderTargetLoadAction LoadAction = RenderTargetLoadAction::Clear;
		RenderTargetStoreAction StoreAction = RenderTargetStoreAction::DontCare;
		float ClearDepth = 1.0f;
	};

	struct GpuRenderPassTimestampWrites
	{
		GpuQuerySet* QuerySet;
		uint32 BeginningOfPassWriteIndex;
		uint32 EndOfPassWriteIndex;
	};

	struct GpuRenderPassDesc
	{
		TArray<GpuRenderTargetInfo, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> ColorRenderTargets;
		TOptional<GpuDepthStencilTargetInfo> DepthStencilTarget;
		TOptional<GpuRenderPassTimestampWrites> TimestampWrites;
	};

}
