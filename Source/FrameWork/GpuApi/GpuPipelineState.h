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

	// PSO Cache Key - uniquely identifies a pipeline state based on shader bytecode and pipeline parameters
	struct GpuPsoCacheKey
	{
		uint32 Hash;

		bool operator==(const GpuPsoCacheKey& Other) const = default;
		friend uint32 GetTypeHash(const GpuPsoCacheKey& Key)
		{
			return ::GetTypeHash(Key.Hash);
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
		TRefCountPtr<GpuRenderPipelineState> FindRenderPso(const GpuPsoCacheKey& Key);
		TRefCountPtr<GpuComputePipelineState> FindComputePso(const GpuPsoCacheKey& Key);

		void AddRenderPso(const GpuPsoCacheKey& Key, TRefCountPtr<GpuRenderPipelineState> Pso);
		void AddComputePso(const GpuPsoCacheKey& Key, TRefCountPtr<GpuComputePipelineState> Pso);

		// Compute cache key from pipeline state desc
		static GpuPsoCacheKey ComputeRenderPsoKey(const GpuRenderPipelineStateDesc& Desc);
		static GpuPsoCacheKey ComputeComputePsoKey(const GpuComputePipelineStateDesc& Desc);

		// Create pipeline state with caching
		TRefCountPtr<GpuRenderPipelineState> CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc);
		TRefCountPtr<GpuComputePipelineState> CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc);

		void ClearCache();

		int32 GetRenderPsoCacheSize() const;
		int32 GetComputePsoCacheSize() const;

	private:
		GpuPsoCacheManager();

		TLruCache<GpuPsoCacheKey, TRefCountPtr<GpuRenderPipelineState>> RenderPsoCache;
		TLruCache<GpuPsoCacheKey, TRefCountPtr<GpuComputePipelineState>> ComputePsoCache;
		mutable FCriticalSection CacheLock;
	};
}

