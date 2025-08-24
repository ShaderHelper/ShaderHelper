#include "CommonHeader.h"
#include "SpirvPixelVM.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	SpvVmPixelVisitor::SpvVmPixelVisitor(SpvVmPixelContext& InPixelContext)
	: PixelContext(InPixelContext)
	{}

	SpvVmContext& SpvVmPixelVisitor::GetActiveContext()
	{
		return PixelContext.Quad[PixelContext.CurActiveIndex];
	}

	void SpvVmPixelVisitor::ParseQuad(int32 QuadIndex, int32 StopIndex)
	{
		PixelContext.CurActiveIndex = QuadIndex;
		SpvThreadState& CurPixelThread = PixelContext.Quad[QuadIndex].ThreadState;
		while(CurPixelThread.InstIndex < Insts->Num())
		{
		   CurPixelThread.NextInstIndex = CurPixelThread.InstIndex + 1;
		   if(StopIndex == CurPixelThread.InstIndex)
		   {
			   break;
		   }
		   
		   (*Insts)[CurPixelThread.InstIndex]->Accpet(this);
		   if(bTerminate || CurPixelThread.StackFrames.empty())
		   {
			   break;
		   }
		   CurPixelThread.InstIndex = CurPixelThread.NextInstIndex;
		}
	}

	bool SpvVmPixelVisitor::FlushQuad(int32 InstIndex)
	{
		//Wait for other threads to reach current instruction location
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			if(QuadIndex != PixelContext.DebugIndex)
			{
				SpvVmContext& OtherContext = PixelContext.Quad[QuadIndex];
				SpvThreadState& OtherThreadState = OtherContext.ThreadState;
				ParseQuad(QuadIndex, InstIndex);
				if(OtherThreadState.InstIndex != InstIndex)
				{
					return false;
				}
				else
				{
					//Skip the instruction in other threads, and the results are processed by the debugindex thread.
					OtherThreadState.InstIndex++;
				}
			}
		}
		PixelContext.CurActiveIndex = PixelContext.DebugIndex;
		return true;
	}

	void SpvVmPixelVisitor::Visit(SpvOpDPdx* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		
		if(!FlushQuad(ThreadState.InstIndex) && EnableUbsan)
		{
			ThreadState.RecordedInfo.DebugStates.Last().UbError = "Derivative instruction in non-uniform control flow!";
			bTerminate = true;
			return;
		}
		
		int32 ResultTypeSize = GetTypeByteSize(ResultType);
		TArray<uint8> Datas;
		Datas.SetNumZeroed(ResultTypeSize * 4);

		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmContext& Context = PixelContext.Quad[QuadIndex];
			SpvObject* P = GetObject(&Context, Inst->GetP());
			Datas.Append(GetObjectValue(P));
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::DPdx));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("DPdx", ResultTypeSize * 4, Datas, ExtraArgs);
		
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmFrame& StackFrame = PixelContext.Quad[QuadIndex].ThreadState.StackFrames.back();
			TArray<uint8> QuadIndexResultValue = {ResultValue.GetData() + ResultTypeSize * QuadIndex, ResultTypeSize};
			
			SpvObject ResultObject{
				.Id = ResultId,
				.Type = ResultType,
				.Storage = SpvObject::Internal{ MoveTemp(QuadIndexResultValue) }
			};
			StackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		}
	}

	void SpvVmPixelVisitor::Visit(SpvOpDPdy* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		
		if(!FlushQuad(ThreadState.InstIndex) && EnableUbsan)
		{
			ThreadState.RecordedInfo.DebugStates.Last().UbError = "Derivative instruction in non-uniform control flow!";
			bTerminate = true;
			return;
		}
		
		int32 ResultTypeSize = GetTypeByteSize(ResultType);
		TArray<uint8> Datas;
		Datas.SetNumZeroed(ResultTypeSize * 4);
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmContext& Context = PixelContext.Quad[QuadIndex];
			SpvObject* P = GetObject(&Context, Inst->GetP());
			Datas.Append(GetObjectValue(P));
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::DPdy));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("DPdy", ResultTypeSize * 4, Datas, ExtraArgs);
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmFrame& StackFrame = PixelContext.Quad[QuadIndex].ThreadState.StackFrames.back();
			TArray<uint8> QuadIndexResultValue = {ResultValue.GetData() + ResultTypeSize * QuadIndex, ResultTypeSize};
			
			SpvObject ResultObject{
				.Id = ResultId,
				.Type = ResultType,
				.Storage = SpvObject::Internal{ MoveTemp(QuadIndexResultValue) }
			};
			StackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		}
	}

	void SpvVmPixelVisitor::Visit(SpvOpFwidth* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		
		if(!FlushQuad(ThreadState.InstIndex) && EnableUbsan)
		{
			ThreadState.RecordedInfo.DebugStates.Last().UbError = "Derivative instruction in non-uniform control flow!";
			bTerminate = true;
			return;
		}
		
		int32 ResultTypeSize = GetTypeByteSize(ResultType);
		TArray<uint8> Datas;
		Datas.SetNumZeroed(ResultTypeSize * 4);
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmContext& Context = PixelContext.Quad[QuadIndex];
			SpvObject* P = GetObject(&Context, Inst->GetP());
			Datas.Append(GetObjectValue(P));
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Fwidth));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFwidth", ResultTypeSize * 4, Datas, ExtraArgs);
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmFrame& StackFrame = PixelContext.Quad[QuadIndex].ThreadState.StackFrames.back();
			TArray<uint8> QuadIndexResultValue = {ResultValue.GetData() + ResultTypeSize * QuadIndex, ResultTypeSize};
			
			SpvObject ResultObject{
				.Id = ResultId,
				.Type = ResultType,
				.Storage = SpvObject::Internal{ MoveTemp(QuadIndexResultValue) }
			};
			StackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		}
	}

	void SpvVmPixelVisitor::Visit(SpvOpImageSampleImplicitLod* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		SpvObject* Coordinate = GetObject(&Context, Inst->GetCoordinate());
		SpvObject* SampledImage = GetObject(&Context, Inst->GetSampledImage());
		
		if(!FlushQuad(ThreadState.InstIndex) && EnableUbsan)
		{
			ThreadState.RecordedInfo.DebugStates.Last().UbError = "Texture sampling with implicit derivative instruction in non-uniform control flow!";
			bTerminate = true;
			return;
		}
		
		int32 ResultTypeSize = GetTypeByteSize(ResultType);
		TArray<uint8> Datas;
		Datas.SetNumZeroed(ResultTypeSize * 4);
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmContext& Context = PixelContext.Quad[QuadIndex];
			SpvObject* Coordinate = GetObject(&Context, Inst->GetCoordinate());
			Datas.Append(GetObjectValue(Coordinate));
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ImageSampleImplicitLod));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Coordinate->Type)));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpImageSampleImplicitLod", ResultTypeSize * 4, Datas, ExtraArgs, std::get<SpvObject::Internal>(SampledImage->Storage).Resources);
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmFrame& StackFrame = PixelContext.Quad[QuadIndex].ThreadState.StackFrames.back();
			TArray<uint8> QuadIndexResultValue = {ResultValue.GetData() + ResultTypeSize * QuadIndex, ResultTypeSize};
			
			SpvObject ResultObject{
				.Id = ResultId,
				.Type = ResultType,
				.Storage = SpvObject::Internal{ MoveTemp(QuadIndexResultValue) }
			};
			StackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		}
	}

	void SpvVmPixelVisitor::Visit(SpvOpKill* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;

		bTerminate = true;
		ThreadState.RecordedInfo.DebugStates.Last().bKill = true;
	}

	void SpvVmPixelVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts)
	{
		SpvVmVisitor::Parse(Insts);
		
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			SpvVmContext& Context = PixelContext.Quad[QuadIndex];
			
			//Get entry point loc
			int32 InstIndex = GetInstIndex(Context.EntryPoint);
			
			//Init global external/location variable
			for(auto& [Id, Var] : Context.GlobalVariables)
			{
				int32 Location = INDEX_NONE;
				std::optional<SpvBuiltIn> BuiltIn;
				int32 SetNumber = INDEX_NONE;
				int32 BindingNumber = INDEX_NONE;
				TArray<SpvDecoration> Decorations;
				Context.Decorations.MultiFind(Id, Decorations);
				for(const auto& Decoration : Decorations)
				{
					if(Decoration.Kind == SpvDecorationKind::DescriptorSet)
					{
						SetNumber = Decoration.DescriptorSet.Number;
					}
					else if(Decoration.Kind == SpvDecorationKind::Binding)
					{
						BindingNumber = Decoration.Binding.Number;
					}
					else if(Decoration.Kind == SpvDecorationKind::BuiltIn)
					{
						BuiltIn = Decoration.BuiltIn.Attribute;
					}
					else if(Decoration.Kind == SpvDecorationKind::Location)
					{
						Location = Decoration.Location.Number;
					}
				}
				if(SetNumber != INDEX_NONE && BindingNumber != INDEX_NONE ;
				   SpvVmBinding* VmBinding = PixelContext.Bindings.FindByPredicate([&](const SpvVmBinding& InItem) { return InItem.Binding == BindingNumber && InItem.DescriptorSet == SetNumber; }))
				{
					auto& Storage = std::get<SpvObject::External>(Var.Storage);
					Storage.Resource = VmBinding->Resource;
					if(VmBinding->Resource->GetType() == GpuResourceType::Buffer)
					{
						GpuBuffer* Buffer = static_cast<GpuBuffer*>(VmBinding->Resource.GetReference());
						uint8* Data = (uint8*)GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Read_Only);
						Storage.Value = {Data, (int)Buffer->GetByteSize()};
						GGpuRhi->UnMapGpuBuffer(Buffer);
						Var.InitializedRanges.AddUnique({0, (int)Buffer->GetByteSize()});
					}
				}
				else if(BuiltIn && Context.ThreadState.BuiltInInput.contains(BuiltIn.value()))
				{
					TArray<uint8>& Value = std::get<SpvObject::Internal>(Var.Storage).Value;
					Value = Context.ThreadState.BuiltInInput[BuiltIn.value()];
					Var.InitializedRanges.AddUnique({0, Value.Num()});
				}
				else if(auto Search = Context.ThreadState.LocationInput.find(Location) ; Search != Context.ThreadState.LocationInput.end())
				{
					auto LocationValue = Search->second;
					TArray<uint8>& Value = std::get<SpvObject::Internal>(Var.Storage).Value;
					Value = LocationValue;;
					Var.InitializedRanges.AddUnique({0, Value.Num()});
				}
			}
			
			Context.ThreadState.InstIndex = InstIndex;
			Context.ThreadState.RecordedInfo.AllVariables = Context.GlobalVariables;
			Context.ThreadState.RecordedInfo.DebugStates.SetNum(1);
			Context.ThreadState.StackFrames.reserve(100);
			Context.ThreadState.StackFrames.resize(1);
		}

		ParseQuad(PixelContext.DebugIndex);
	}
}
