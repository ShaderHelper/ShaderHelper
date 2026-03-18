#pragma once
#include "GpuResourceCommon.h"
#include "GpuShader.h"
#include "GpuBindGroupLayout.h"
#include "Containers/LruCache.h"

namespace FW
{
    struct RasterizerStateDesc
    {
        RasterizerFillMode FillMode;
        RasterizerCullMode CullMode;
    };

    struct PipelineTargetDesc
    {
		GpuTextureFormat TargetFormat;

		bool BlendEnable = false;
		BlendFactor SrcFactor = BlendFactor::SrcAlpha;
		BlendOp ColorOp = BlendOp::Add;
		BlendFactor DestFactor = BlendFactor::InvSrcAlpha;
		BlendFactor SrcAlphaFactor = BlendFactor::One;
		BlendOp AlphaOp = BlendOp::Add;
		BlendFactor DestAlphaFactor = BlendFactor::Zero;
		BlendMask Mask = BlendMask::All;

		bool operator==(const PipelineTargetDesc& Other) const = default;
    };

	struct GpuComputePipelineStateDesc
	{
		bool CheckLayout = false;
		GpuShader* Cs;

		GpuBindGroupLayout* BindGroupLayout0 = nullptr;
		GpuBindGroupLayout* BindGroupLayout1 = nullptr;
		GpuBindGroupLayout* BindGroupLayout2 = nullptr;
		GpuBindGroupLayout* BindGroupLayout3 = nullptr;
	};

    struct GpuRenderPipelineStateDesc
    {
		bool CheckLayout = false;
        GpuShader* Vs;
        GpuShader* Ps;
		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;

		GpuBindGroupLayout* BindGroupLayout0 = nullptr;
		GpuBindGroupLayout* BindGroupLayout1 = nullptr;
		GpuBindGroupLayout* BindGroupLayout2 = nullptr;
		GpuBindGroupLayout* BindGroupLayout3 = nullptr;

		RasterizerStateDesc RasterizerState{ RasterizerFillMode::Solid, RasterizerCullMode::None };
		PrimitiveType Primitive = PrimitiveType::TriangleList;
    };

    class GpuRenderPipelineState : public GpuResource
    {
	public:
		GpuRenderPipelineState(GpuRenderPipelineStateDesc InDesc) 
			: GpuResource(GpuResourceType::RenderPipelineState)
			, Desc(MoveTemp(InDesc))
		{}
		const GpuRenderPipelineStateDesc& GetDesc() const { return Desc; }

	private:
		GpuRenderPipelineStateDesc Desc;
    };

	class GpuComputePipelineState : public GpuResource
	{
	public:
		GpuComputePipelineState(GpuComputePipelineStateDesc InDesc) 
			: GpuResource(GpuResourceType::ComputePipelineState)
			, Desc(MoveTemp(InDesc))
		{
		}
		const GpuComputePipelineStateDesc& GetDesc() const { return Desc; }

	private:
		GpuComputePipelineStateDesc Desc;
	};

	struct GpuRenderPsoCacheKey
	{
		uint32 Hash;
		TRefCountPtr<GpuShader> Vs;
		TRefCountPtr<GpuShader> Ps;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout0;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout1;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout2;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout3;
		RasterizerStateDesc RasterizerState;
		PrimitiveType Primitive;
		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;

		bool operator==(const GpuRenderPsoCacheKey& Other) const
		{
			const bool VsEqual =  Vs && Other.Vs
					&& Vs->GetSourceText() == Other.Vs->GetSourceText()
					&& Vs->CompileExtraArgs == Other.Vs->CompileExtraArgs;
			const bool PsEqual = (Ps == Other.Ps)
				|| (Ps && Other.Ps
					&& Ps->GetSourceText() == Other.Ps->GetSourceText()
					&& Ps->CompileExtraArgs == Other.Ps->CompileExtraArgs);
			return VsEqual && PsEqual
				&& BindGroupLayout0 == Other.BindGroupLayout0
				&& BindGroupLayout1 == Other.BindGroupLayout1
				&& BindGroupLayout2 == Other.BindGroupLayout2
				&& BindGroupLayout3 == Other.BindGroupLayout3
				&& RasterizerState.FillMode == Other.RasterizerState.FillMode
				&& RasterizerState.CullMode == Other.RasterizerState.CullMode
				&& Primitive == Other.Primitive
				&& Targets == Other.Targets;
		}

		friend uint32 GetTypeHash(const GpuRenderPsoCacheKey& Key)
		{
			return Key.Hash;
		}
	};

	struct GpuComputePsoCacheKey
	{
		uint32 Hash;
		TRefCountPtr<GpuShader> Cs;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout0;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout1;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout2;
		TOptional<GpuBindGroupLayoutDesc> BindGroupLayout3;

		bool operator==(const GpuComputePsoCacheKey& Other) const
		{
			const bool CsEqual = Cs && Other.Cs
					&& Cs->GetSourceText() == Other.Cs->GetSourceText()
					&& Cs->CompileExtraArgs == Other.Cs->CompileExtraArgs;
			return CsEqual && BindGroupLayout0 == Other.BindGroupLayout0
				&& BindGroupLayout1 == Other.BindGroupLayout1
				&& BindGroupLayout2 == Other.BindGroupLayout2
				&& BindGroupLayout3 == Other.BindGroupLayout3;
		}

		friend uint32 GetTypeHash(const GpuComputePsoCacheKey& Key)
		{
			return Key.Hash;
		}
	};

	// Generic PSO Cache Manager - manages LRU cache for pipeline states
	class FRAMEWORK_API GpuPsoCacheManager
	{
	public:
		static constexpr int32 DefaultRenderPsoCacheSize = 128;
		static constexpr int32 DefaultComputePsoCacheSize = 64;

		static GpuPsoCacheManager& Get();

		// Try to find cached PSO, returns nullptr if not found
		TRefCountPtr<GpuRenderPipelineState> FindRenderPso(const GpuRenderPsoCacheKey& Key);
		TRefCountPtr<GpuComputePipelineState> FindComputePso(const GpuComputePsoCacheKey& Key);

		void AddRenderPso(const GpuRenderPsoCacheKey& Key, TRefCountPtr<GpuRenderPipelineState> Pso);
		void AddComputePso(const GpuComputePsoCacheKey& Key, TRefCountPtr<GpuComputePipelineState> Pso);

		// Compute cache key from pipeline state desc
		static GpuRenderPsoCacheKey ComputeRenderPsoKey(const GpuRenderPipelineStateDesc& Desc);
		static GpuComputePsoCacheKey ComputeComputePsoKey(const GpuComputePipelineStateDesc& Desc);

		// Create pipeline state with caching
		TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
		void CreateRenderPipelineStateAsync(const GpuRenderPipelineStateDesc& InPipelineStateDesc,
			TFunction<void(TRefCountPtr<GpuRenderPipelineState>)> Callback);
		TRefCountPtr<GpuComputePipelineState> CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc);

		void ClearCache();

		int32 GetRenderPsoCacheSize() const;
		int32 GetComputePsoCacheSize() const;

	private:
		GpuPsoCacheManager();

		TLruCache<GpuRenderPsoCacheKey, TRefCountPtr<GpuRenderPipelineState>> RenderPsoCache;
		TLruCache<GpuComputePsoCacheKey, TRefCountPtr<GpuComputePipelineState>> ComputePsoCache;
		mutable FCriticalSection CacheLock;
	};
}

