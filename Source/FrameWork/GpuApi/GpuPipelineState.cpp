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

	TRefCountPtr<GpuRenderPipelineState> GpuPsoCacheManager::FindRenderPso(const GpuPsoCacheKey& Key)
	{
		FScopeLock Lock(&CacheLock);
		const TRefCountPtr<GpuRenderPipelineState>* Found = RenderPsoCache.FindAndTouch(Key);
		return Found ? *Found : nullptr;
	}

	TRefCountPtr<GpuComputePipelineState> GpuPsoCacheManager::FindComputePso(const GpuPsoCacheKey& Key)
	{
		FScopeLock Lock(&CacheLock);
		const TRefCountPtr<GpuComputePipelineState>* Found = ComputePsoCache.FindAndTouch(Key);
		return Found ? *Found : nullptr;
	}

	void GpuPsoCacheManager::AddRenderPso(const GpuPsoCacheKey& Key, TRefCountPtr<GpuRenderPipelineState> Pso)
	{
		FScopeLock Lock(&CacheLock);
		RenderPsoCache.Add(Key, MoveTemp(Pso));
	}

	void GpuPsoCacheManager::AddComputePso(const GpuPsoCacheKey& Key, TRefCountPtr<GpuComputePipelineState> Pso)
	{
		FScopeLock Lock(&CacheLock);
		ComputePsoCache.Add(Key, MoveTemp(Pso));
	}

	GpuPsoCacheKey GpuPsoCacheManager::ComputeRenderPsoKey(const GpuRenderPipelineStateDesc& Desc)
	{
		uint32 Hash = 0;
		
		// Hash shader pointers directly
		if (Desc.Vs)
		{
			Hash = HashCombine(Hash, PointerHash(Desc.Vs));
		}
		if (Desc.Ps)
		{
			Hash = HashCombine(Hash, PointerHash(Desc.Ps));
		}

		// Hash bind group layouts
		if (Desc.BindGroupLayout0) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout0->GetDesc())); }
		if (Desc.BindGroupLayout1) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout1->GetDesc())); }
		if (Desc.BindGroupLayout2) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout2->GetDesc())); }
		if (Desc.BindGroupLayout3) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout3->GetDesc())); }

		// Hash rasterizer state
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.RasterizerState.FillMode));
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.RasterizerState.CullMode));

		// Hash primitive type
		Hash = HashCombine(Hash, ::GetTypeHash(Desc.Primitive));

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

		return GpuPsoCacheKey{ Hash };
	}

	GpuPsoCacheKey GpuPsoCacheManager::ComputeComputePsoKey(const GpuComputePipelineStateDesc& Desc)
	{
		uint32 Hash = 0;
		
		// Hash shader pointer directly
		if (Desc.Cs)
		{
			Hash = HashCombine(Hash, PointerHash(Desc.Cs));
		}

		// Hash bind group layouts
		if (Desc.BindGroupLayout0) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout0->GetDesc())); }
		if (Desc.BindGroupLayout1) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout1->GetDesc())); }
		if (Desc.BindGroupLayout2) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout2->GetDesc())); }
		if (Desc.BindGroupLayout3) { Hash = HashCombine(Hash, GetTypeHash(Desc.BindGroupLayout3->GetDesc())); }

		return GpuPsoCacheKey{ Hash };
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
		GpuPsoCacheKey CacheKey = ComputeRenderPsoKey(InPipelineStateDesc);
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
		GpuPsoCacheKey CacheKey = ComputeComputePsoKey(InPipelineStateDesc);
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
