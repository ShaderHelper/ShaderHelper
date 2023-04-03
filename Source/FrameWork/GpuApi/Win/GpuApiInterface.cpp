#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12Texture.h"
#include "D3D12Shader.h"
#include "D3D12PSO.h"

namespace FRAMEWORK
{
namespace GpuApi
{
	void InitApiEnv()
	{	
		InitDx12Core();
		InitFrameResource();
	}

	void FlushGpu()
	{
		DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurCpuFrame, CpuSyncGpuEvent));
		WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
		CurGpuFrame = CurCpuFrame;
	}

	void StartRenderFrame()
	{
		//StaticFrameResource
		uint32 FrameResourceIndex = GetCurFrameSourceIndex();
		GCommandListContext->ResetStaticFrameResource(FrameResourceIndex);
		GCommandListContext->BindStaticFrameResource(FrameResourceIndex);

		GDynamicFrameResourceManager.AllocateOneFrame();
	}

	void EndRenderFrame()
	{
		check(CurCpuFrame >= CurGpuFrame);
		CurCpuFrame++;
		DxCheck(GGraphicsQueue->Signal(CpuSyncGpuFence, CurCpuFrame));
		
		CurGpuFrame = CpuSyncGpuFence->GetCompletedValue();
		const uint64 CurLag = CurCpuFrame - CurGpuFrame;
		if (CurLag >= AllowableLag) {
			//Cpu is waiting for gpu to catch up a frame.
			DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurGpuFrame + 1, CpuSyncGpuEvent));
			WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
			CurGpuFrame = CurGpuFrame + 1;
		}

		GDynamicFrameResourceManager.ReleaseCompletedResources();
	}

	TRefCountPtr<GpuTexture> CreateGpuTexture(const GpuTextureDesc& InTexDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuTexture>(CreateDx12Texture(InTexDesc));
	}

	void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode)
	{
		Dx12Texture* Texture = static_cast<Dx12Texture*>(InGpuTexture);
		void* Data{};
		if (InMapMode == GpuResourceMapMode::Write_Only) {
			const uint64 UploadBufferSize = GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
			Texture->UploadBuffer = new Dx12UploadBuffer(UploadBufferSize);
			Data = Texture->UploadBuffer->Map();
			Texture->bIsMappingForWriting = true;
		}
		return Data;
	}

	void UnMapGpuTexture(GpuTexture* InGpuTexture)
	{
		Dx12Texture* Texture = static_cast<Dx12Texture*>(InGpuTexture);
		if (Texture->bIsMappingForWriting) {
			Texture->UploadBuffer->Unmap();
			ScopedBarrier Barrier{ Texture, D3D12_RESOURCE_STATE_COPY_DEST };
			ID3D12GraphicsCommandList* CommandListHandle = GCommandListContext->GetCommandListHandle();

			CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ Texture->GetResource() };
			CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ Texture->UploadBuffer->GetResource() };
			CommandListHandle->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc,nullptr);
				
			Texture->bIsMappingForWriting = false;
		}
	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName)
	{
		return new Dx12Shader(InType, MoveTemp(InSourceText), MoveTemp(InShaderName));
	}

	bool CompilerShader(GpuShader* InShader)
	{
		return GShaderCompiler.Compile(static_cast<Dx12Shader*>(InShader));
	}


	TRefCountPtr<RenderPipelineState> CreateRenderPipelineState(const PipelineStateDesc& InPipelineStateDesc)
	{
		CD3DX12_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		TRefCountPtr<ID3DBlob> Signature;
		TRefCountPtr<ID3DBlob> Error;
		TRefCountPtr<ID3D12RootSignature> RootSignature;
		DxCheck(D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, Signature.GetInitReference(), Error.GetInitReference()));
		DxCheck(GDevice->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(RootSignature.GetInitReference())));
		//TODO RootSignature Manager.

		TRefCountPtr<Dx12Shader> Vs = AUX::StaticCastRefCountPtr<Dx12Shader>(InPipelineStateDesc.Vs);
		TRefCountPtr<Dx12Shader> Ps = AUX::StaticCastRefCountPtr<Dx12Shader>(InPipelineStateDesc.Ps);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc{};
		PsoDesc.pRootSignature = RootSignature;
		PsoDesc.VS = { Vs->GetCompilationResult()->GetBufferPointer(), Vs->GetCompilationResult()->GetBufferSize() };
		PsoDesc.PS = { Ps->GetCompilationResult()->GetBufferPointer(), Ps->GetCompilationResult()->GetBufferSize() };
		PsoDesc.RasterizerState = MapRasterizerState(InPipelineStateDesc.RasterizerState);
		PsoDesc.BlendState = MapBlendState(InPipelineStateDesc.BlendState);
		PsoDesc.DepthStencilState.DepthEnable = false;
		PsoDesc.DepthStencilState.StencilEnable = false;
		PsoDesc.SampleMask = UINT_MAX;
		PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PsoDesc.NumRenderTargets = 1;
		PsoDesc.RTVFormats[0] = MapTextureFormat(InPipelineStateDesc.RtFormat);
		PsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
		PsoDesc.SampleDesc.Count = 1;
		PsoDesc.SampleDesc.Quality = 0;

		TRefCountPtr<ID3D12PipelineState> Pso;
		DxCheck(GDevice->CreateGraphicsPipelineState(&PsoDesc, IID_PPV_ARGS(Pso.GetInitReference())));
		return new Dx12Pso(MoveTemp(Pso), MoveTemp(RootSignature),MoveTemp(Vs), MoveTemp(Ps));
	}

	void BindRenderPipelineState(RenderPipelineState* InPipelineState)
	{
		GCommandListContext->SetPipeline(static_cast<Dx12Pso*>(InPipelineState));
		GCommandListContext->MarkPipelineDirty(true);
	}

	void BindVertexBuffer(GpuBuffer* InVertexBuffer)
	{
		Dx12VertexBuffer* Vb = static_cast<Dx12VertexBuffer*>(InVertexBuffer);
		GCommandListContext->SetVertexBuffer(Vb);
		GCommandListContext->MarkVertexBufferDirty(true);
	}

	void SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
		D3D12_VIEWPORT ViewPort{};
		ViewPort.Width = (float)InViewPortDesc.Width;
		ViewPort.Height = (float)InViewPortDesc.Height;
		ViewPort.MinDepth = InViewPortDesc.ZMin;
		ViewPort.MaxDepth = InViewPortDesc.ZMax;
		ViewPort.TopLeftX = InViewPortDesc.TopLeftX;
		ViewPort.TopLeftY = InViewPortDesc.TopLeftY;

		D3D12_RECT ScissorRect = CD3DX12_RECT(0, 0, InViewPortDesc.Width, InViewPortDesc.Height);
		GCommandListContext->SetViewPort(MakeUnique<D3D12_VIEWPORT>(ViewPort), MakeUnique<D3D12_RECT>(ScissorRect));
		GCommandListContext->MarkViewportDirty(true);
	}


	void SetRenderTarget(GpuTexture* InGpuTexture)
	{
		Dx12Texture* Rt = static_cast<Dx12Texture*>(InGpuTexture);
		GCommandListContext->SetRenderTarget(Rt);
		GCommandListContext->MarkRenderTartgetDirty(true);
	}

	void SetClearColorValue(Vector4f ClearColor)
	{
		GCommandListContext->SetClearColor(MakeUnique<Vector4f>(ClearColor));
	}

	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount, PrimitiveType InType)
	{
		GCommandListContext->SetPrimitiveType(InType);
		GCommandListContext->PrepareDrawingEnv();
		GCommandListContext->GetCommandListHandle()->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	void Submit()
	{
		check(GGraphicsQueue);
		ID3D12GraphicsCommandList* GraphicsCommandList = GCommandListContext->GetCommandListHandle();
		//The commandLists must be closed before executing them.
		DxCheck(GraphicsCommandList->Close());
		ID3D12CommandList* CmdLists[] = { GraphicsCommandList };
		GGraphicsQueue->ExecuteCommandLists(1, CmdLists);
	}

	void BeginGpuCapture(const FString& SavedFileName)
	{
#if USE_PIX
		if (GCanGpuCapture) {
			PIXCaptureParameters params = {};
			FString FileName = FString::Format(TEXT("{0}.wpix"), { SavedFileName});
			params.GpuCaptureParameters.FileName = *FileName;
			FlushGpu();
			PIXBeginCapture(PIX_CAPTURE_GPU, &params);
		}
#endif
	}

	void EndGpuCapture()
	{
#if USE_PIX
		if (GCanGpuCapture) {
			FlushGpu();
			PIXEndCapture(false);
		}
#endif
	}
	
}
}