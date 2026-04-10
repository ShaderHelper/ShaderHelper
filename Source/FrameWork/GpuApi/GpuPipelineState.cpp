#include "CommonHeader.h"
#include "GpuPipelineState.h"
#include "GpuRhi.h"

namespace FW
{

	GpuPsoCacheManager& GpuPsoCacheManager::Get()
	{
		static GpuPsoCacheManager Instance;
		return Instance;
	}

	GpuPsoCacheManager::GpuPsoCacheManager()
		: RenderPsoCache(DefaultRenderPsoCacheSize)
		, ComputePsoCache(DefaultComputePsoCacheSize)
	{
	}

	TRefCountPtr<GpuRenderPipelineState> GpuPsoCacheManager::FindRenderPso(const GpuRenderPsoCacheKey& Key)
	{
		FScopeLock Lock(&CacheLock);
		const TRefCountPtr<GpuRenderPipelineState>* Found = RenderPsoCache.FindAndTouch(Key);
		return Found ? *Found : nullptr;
	}

	TRefCountPtr<GpuComputePipelineState> GpuPsoCacheManager::FindComputePso(const GpuComputePsoCacheKey& Key)
	{
		FScopeLock Lock(&CacheLock);
		const TRefCountPtr<GpuComputePipelineState>* Found = ComputePsoCache.FindAndTouch(Key);
		return Found ? *Found : nullptr;
	}

	void GpuPsoCacheManager::AddRenderPso(const GpuRenderPsoCacheKey& Key, TRefCountPtr<GpuRenderPipelineState> Pso)
	{
		FScopeLock Lock(&CacheLock);
		RenderPsoCache.Add(Key, MoveTemp(Pso));
	}

	void GpuPsoCacheManager::AddComputePso(const GpuComputePsoCacheKey& Key, TRefCountPtr<GpuComputePipelineState> Pso)
	{
		FScopeLock Lock(&CacheLock);
		ComputePsoCache.Add(Key, MoveTemp(Pso));
	}

	GpuRenderPsoCacheKey GpuPsoCacheManager::ComputeRenderPsoKey(const GpuRenderPipelineStateDesc& Desc)
	{
		uint32 Hash = 0;
		
		uint32 VsHash = Desc.Vs ? GetTypeHash(*Desc.Vs) : 0;
		uint32 PsHash = Desc.Ps ? GetTypeHash(*Desc.Ps) : 0;
		Hash = HashCombine(Hash, VsHash);
		Hash = HashCombine(Hash, PsHash);

		// Hash bind group layouts
		for (GpuBindGroupLayout* Layout : Desc.BindGroupLayouts)
		{
			if (Layout) {
				Hash = HashCombine(Hash, ::GetTypeHash(Layout->GetGroupNumber()));
				Hash = HashCombine(Hash, GetTypeHash(Layout->GetDesc()));
			}
		}

		// Hash rasterizer state
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.RasterizerState.FillMode));
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.RasterizerState.CullMode));

		Hash = HashCombine(Hash, ::GetTypeHash(Desc.VertexLayout.Num()));
		for (const auto& BufferLayout : Desc.VertexLayout)
		{
			Hash = HashCombine(Hash, ::GetTypeHash(BufferLayout.ByteStride));
			Hash = HashCombine(Hash, ::GetTypeHash(BufferLayout.StepMode));
			Hash = HashCombine(Hash, ::GetTypeHash(BufferLayout.Attributes.Num()));
			for (const auto& Attribute : BufferLayout.Attributes)
			{
				Hash = HashCombine(Hash, ::GetTypeHash(Attribute.Location));
				Hash = HashCombine(Hash, GetTypeHash(Attribute.SemanticName));
				Hash = HashCombine(Hash, ::GetTypeHash(Attribute.SemanticIndex));
				Hash = HashCombine(Hash, ::GetTypeHash(Attribute.Format));
				Hash = HashCombine(Hash, ::GetTypeHash(Attribute.ByteOffset));
			}
		}

		// Hash primitive type
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.Primitive));
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.SampleCount));

		// Hash render targets
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.Targets.Num()));
		for (const auto& Target : Desc.Targets)
		{
			Hash = HashCombine(Hash, ::GetTypeHash(Target.TargetFormat));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.BlendEnable));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.SrcFactor));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.DestFactor));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.ColorOp));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.SrcAlphaFactor));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.DestAlphaFactor));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.AlphaOp));
			Hash = HashCombine(Hash, ::GetTypeHash(Target.Mask));
		}

		// Hash depth stencil state
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.DepthStencilState.IsSet()));
		if (Desc.DepthStencilState)
		{
			Hash = HashCombine(Hash, ::GetTypeHash(Desc.DepthStencilState->DepthFormat));
			Hash = HashCombine(Hash, ::GetTypeHash(Desc.DepthStencilState->DepthWriteEnable));
			Hash = HashCombine(Hash, ::GetTypeHash(Desc.DepthStencilState->DepthCompare));
		}

		TArray<TPair<BindingGroupSlot, GpuBindGroupLayoutDesc>> CacheBindGroupLayouts;
		for (GpuBindGroupLayout* Layout : Desc.BindGroupLayouts)
		{
			if (Layout) { CacheBindGroupLayouts.Add({Layout->GetGroupNumber(), Layout->GetDesc()}); }
		}

		return GpuRenderPsoCacheKey{
			.Hash = Hash,
			.Vs = Desc.Vs,
			.Ps = Desc.Ps,
			.BindGroupLayouts = MoveTemp(CacheBindGroupLayouts),
			.VertexLayout = Desc.VertexLayout,
			.RasterizerState = Desc.RasterizerState,
			.Primitive = Desc.Primitive,
			.SampleCount = Desc.SampleCount,
			.Targets = Desc.Targets,
			.DepthStencilState = Desc.DepthStencilState,
		};
	}

	GpuComputePsoCacheKey GpuPsoCacheManager::ComputeComputePsoKey(const GpuComputePipelineStateDesc& Desc)
	{
		uint32 Hash = 0;
		
		uint32 CsHash = Desc.Cs ? GetTypeHash(*Desc.Cs) : 0;
		Hash = HashCombine(Hash, CsHash);

		// Hash bind group layouts
		for (GpuBindGroupLayout* Layout : Desc.BindGroupLayouts)
		{
			if (Layout) {
				Hash = HashCombine(Hash, ::GetTypeHash(Layout->GetGroupNumber()));
				Hash = HashCombine(Hash, GetTypeHash(Layout->GetDesc()));
			}
		}

		TArray<TPair<BindingGroupSlot, GpuBindGroupLayoutDesc>> CacheBindGroupLayouts;
		for (GpuBindGroupLayout* Layout : Desc.BindGroupLayouts)
		{
			if (Layout) { CacheBindGroupLayouts.Add({Layout->GetGroupNumber(), Layout->GetDesc()}); }
		}

		return GpuComputePsoCacheKey{
			.Hash = Hash,
			.Cs = Desc.Cs,
			.BindGroupLayouts = MoveTemp(CacheBindGroupLayouts),
		};
	}

	void GpuPsoCacheManager::ClearCache()
	{
		FScopeLock Lock(&CacheLock);
		RenderPsoCache.Empty(DefaultRenderPsoCacheSize);
		ComputePsoCache.Empty(DefaultComputePsoCacheSize);
	}

	int32 GpuPsoCacheManager::GetRenderPsoCacheSize() const
	{
		FScopeLock Lock(&CacheLock);
		return RenderPsoCache.Num();
	}

	int32 GpuPsoCacheManager::GetComputePsoCacheSize() const
	{
		FScopeLock Lock(&CacheLock);
		return ComputePsoCache.Num();
	}

	TRefCountPtr<GpuRenderPipelineState> GpuPsoCacheManager::CreateRenderPipelineState(const GpuRenderPipelineStateDesc& InPipelineStateDesc)
	{
		GpuRenderPsoCacheKey CacheKey = ComputeRenderPsoKey(InPipelineStateDesc);
		TRefCountPtr<GpuRenderPipelineState> CachedPso = FindRenderPso(CacheKey);
		if (CachedPso)
		{
			return CachedPso;
		}
		
		TRefCountPtr<GpuRenderPipelineState> NewPso = GGpuRhi->CreateRenderPipelineState(InPipelineStateDesc);
		AddRenderPso(CacheKey, NewPso);
		return NewPso;
	}

	TRefCountPtr<GpuComputePipelineState> GpuPsoCacheManager::CreateComputePipelineState(const GpuComputePipelineStateDesc& InPipelineStateDesc)
	{
		GpuComputePsoCacheKey CacheKey = ComputeComputePsoKey(InPipelineStateDesc);
		TRefCountPtr<GpuComputePipelineState> CachedPso = FindComputePso(CacheKey);
		if (CachedPso)
		{
			return CachedPso;
		}
		
		TRefCountPtr<GpuComputePipelineState> NewPso = GGpuRhi->CreateComputePipelineState(InPipelineStateDesc);
		AddComputePso(CacheKey, NewPso);
		return NewPso;
	}

}
