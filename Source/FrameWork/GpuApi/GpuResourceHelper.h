#pragma once

namespace FRAMEWORK::GpuResourceHelper
{
	FRAMEWORK_API TRefCountPtr<GpuTexture> GetGlobalBlackTex();
	FRAMEWORK_API TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat);

	const inline BlendStateDesc GDefaultBlendStateDesc{
		/*.RtDescs = */ {
			BlendRenderTargetDesc { false, BlendFactor::SrcAlpha, BlendOp::Add, BlendFactor::InvSrcAlpha, BlendFactor::One, BlendOp::Add, BlendFactor::One }
		}
	};

	const inline RasterizerStateDesc GDefaultRasterizerStateDesc{ RasterizerFillMode::Solid, RasterizerCullMode::None };

}