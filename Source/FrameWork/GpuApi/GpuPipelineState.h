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

	// PSO Cache Key - uniquely identifies a pipeline state based on shader bytecode and pipeline parameters
	// Note: Hash is used for bucket lookup (GetTypeHash), but operator== compares the full descriptor
	// to avoid hash collisions (e.g. PointerHash truncates 64-bit pointers to 32-bit on 64-bit platforms).
	struct GpuRenderPsoCacheKey
	{
		uint32 Hash;
		GpuRenderPipelineStateDesc Desc;

		bool operator==(const GpuRenderPsoCacheKey& Other) const
		{
			return Desc.Vs == Other.Desc.Vs
				&& Desc.Ps == Other.Desc.Ps
				&& Desc.BindGroupLayout0 == Other.Desc.BindGroupLayout0
				&& Desc.BindGroupLayout1 == Other.Desc.BindGroupLayout1
				&& Desc.BindGroupLayout2 == Other.Desc.BindGroupLayout2
				&& Desc.BindGroupLayout3 == Other.Desc.BindGroupLayout3
				&& Desc.RasterizerState.FillMode == Other.Desc.RasterizerState.FillMode
				&& Desc.RasterizerState.CullMode == Other.Desc.RasterizerState.CullMode
				&& Desc.Primitive == Other.Desc.Primitive
				&& Desc.Targets == Other.Desc.Targets;
		}

		friend uint32 GetTypeHash(const GpuRenderPsoCacheKey& Key)
		{
			return Key.Hash;
		}
	};

	struct GpuComputePsoCacheKey
	{
		uint32 Hash;
		GpuComputePipelineStateDesc Desc;

		bool operator==(const GpuComputePsoCacheKey& Other) const
		{
			return Desc.Cs == Other.Desc.Cs
				&& Desc.BindGroupLayout0 == Other.Desc.BindGroupLayout0
				&& Desc.BindGroupLayout1 == Other.Desc.BindGroupLayout1
				&& Desc.BindGroupLayout2 == Other.Desc.BindGroupLayout2
				&& Desc.BindGroupLayout3 == Other.Desc.BindGroupLayout3;
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

