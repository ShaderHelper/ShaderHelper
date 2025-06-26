#include "CommonHeader.h"
#include "SpirvVM.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	SpvVmPixelVisitor::SpvVmPixelVisitor(SpvVmPixelContext& InContext)
	: Context(InContext)
	{}

	TArray<uint8> GetPointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc)
	{
		check(InPointer);
		check(InPointer->Indexes.IsEmpty() || PointeeDesc);
		
		TArray<uint8> RetValue;
		if(std::holds_alternative<SpvObject::External>(InPointer->Pointee->Storage))
		{
			RetValue = std::get<SpvObject::External>(InPointer->Pointee->Storage).Value;
		}
		else
		{
			RetValue = std::get<SpvObject::Internal>(InPointer->Pointee->Storage).Value;
		}
		
		if(InPointer->Indexes.IsEmpty())
		{
			return RetValue;
		}
		else
		{
			
		}
	}

	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore)
	{
		check(InPointer);
		check(InPointer->Indexes.IsEmpty() || PointeeDesc);
		
		TArray<uint8>* ValueRef;
		if(std::holds_alternative<SpvObject::External>(InPointer->Pointee->Storage))
		{
			ValueRef = &std::get<SpvObject::External>(InPointer->Pointee->Storage).Value;
		}
		else
		{
			ValueRef = &std::get<SpvObject::Internal>(InPointer->Pointee->Storage).Value;
		}
		
		if(!InPointer->Pointee->Initialized)
		{
			
		}
	}

	void SpvVmPixelVisitor::Visit(SpvDebugDeclare* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		ThreadState.RecordedInfo.AllVariableDescMap.FindOrAdd(Inst->GetVarId(), VarDesc);
		CurStackFrame.VariableDescMap.Add(Inst->GetVarId(), VarDesc);
	}

	void SpvVmPixelVisitor::Visit(SpvDebugLine* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		int32 OldLineNumber = CurStackFrame.CurLineNumber;
		CurStackFrame.CurLineNumber = Inst->GetLineStart();
		if(OldLineNumber != CurStackFrame.CurLineNumber)
		{
			ThreadState.RecordedInfo.LineDebugStates.Emplace(CurStackFrame.CurLineNumber);
		}
	}

	void SpvVmPixelVisitor::Visit(SpvDebugScope* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		int32 OldLineNumber = CurStackFrame.CurLineNumber;
		SpvLexicalScope* OldScope = CurStackFrame.CurScope;
		SpvLexicalScope* NewScope = Context.LexicalScopes[Inst->GetScope()].Get();
		CurStackFrame.CurScope = NewScope;
		CurStackFrame.CurLineNumber = NewScope->GetLineNumber();

		if(OldLineNumber != CurStackFrame.CurLineNumber)
		{
			SpvDebugState Debugstate {
				.LineNumber = CurStackFrame.CurLineNumber,
				.ScopeChange = {OldScope, NewScope}
			};
			ThreadState.RecordedInfo.LineDebugStates.Add(MoveTemp(Debugstate));
		}
	}

	void SpvVmPixelVisitor::Visit(SpvOpFunctionParameter* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		CurStackFrame.Pointers.Add(Inst->GetId().value(), *CurStackFrame.Arguments.Pop());
	}

	void SpvVmPixelVisitor::Visit(SpvOpFunctionCall* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		ThreadState.NextInstIndex = GetInstIndex(Inst->GetFunction());
		TArray<SpvPointer*> Arguments;
		for(SpvId ArgumentId : Inst->GetArguments())
		{
			Arguments.Add(&CurStackFrame.Pointers[ArgumentId]);
		}
	
		SpvVmFrame NewFrame{
			.ReturnPointIndex = ThreadState.InstIndex + 1,
			.Arguments = MoveTemp(Arguments)
		};
		
		SpvType* ReturnType = Context.Types[Inst->GetResultType()].Get();
		if(ReturnType->GetKind() != SpvTypeKind::Void)
		{
			SpvObject ReturnObject{ReturnType};
			CurStackFrame.IntermediateObjects.Add(Inst->GetId().value(), MoveTemp(ReturnObject));
			NewFrame.ReturnObject = &CurStackFrame.IntermediateObjects[Inst->GetId().value()];
		}
		ThreadState.StackFrames.Add(MoveTemp(NewFrame));
	}

	void SpvVmPixelVisitor::Visit(SpvOpVariable* Inst)
	{
		if(!Context.Types.Contains(Inst->GetResultType()))
		{
			return;
		}
		
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{PointerType->PointeeType}, false, StorageClass};
		SpvPointer Pointer{&Var};
		
		ThreadState.RecordedInfo.AllVariables.FindOrAdd(Inst->GetId().value(), Var);
		CurStackFrame.Variables.Add(Inst->GetId().value(), MoveTemp(Var));
		CurStackFrame.Pointers.Add(Inst->GetId().value(), MoveTemp(Pointer));
	}

	void SpvVmPixelVisitor::Visit(SpvOpLabel* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		CurStackFrame.PreBasicBlock = CurStackFrame.CurBasicBlock;
		CurStackFrame.CurBasicBlock = Inst->GetId().value();
	}

	void SpvVmPixelVisitor::Visit(SpvOpLoad* Inst)
	{
		if(!Context.Types.Contains(Inst->GetResultType()))
		{
			return;
		}
		
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvPointer* Pointer = nullptr;
		SpvVariableDesc* PointeeDesc = nullptr;
		if(Context.GlobalPointers.Contains(Inst->GetPointer()))
		{
			Pointer = &Context.GlobalPointers[Inst->GetPointer()];
			SpvId VarId = *Context.GlobalVariables.FindKey(*Pointer->Pointee);
			PointeeDesc = Context.GlobalVariableDescMap.FindRef(VarId);
		}
		else
		{
			Pointer = &CurStackFrame.Pointers[Inst->GetPointer()];
			SpvId VarId = *CurStackFrame.Variables.FindKey(*Pointer->Pointee);
			PointeeDesc = CurStackFrame.VariableDescMap.FindRef(VarId);
		}
		TArray<uint8> Value = GetPointerValue(Pointer, PointeeDesc);
		if(Value.IsEmpty())
		{
			ThreadState.RecordedInfo.LineDebugStates.Last().UbError = "Reading the uninitialized variable:";
			AnyError = true;
			return;
		}
		
		SpvObject Object = {
			.Type = Context.Types[Inst->GetResultType()].Get(),
			.Storage = SpvObject::Internal{ MoveTemp(Value) }
		};
		CurStackFrame.IntermediateObjects.Add(Inst->GetId().value(), MoveTemp(Object));
	}

	void SpvVmPixelVisitor::Visit(SpvOpStore* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		if(!Context.GlobalPointers.Contains(Inst->GetPointer()) &&
		   !CurStackFrame.Pointers.Contains(Inst->GetPointer()))
		{
			return;
		}
		
		SpvObject* ObjectToStore = nullptr;
		//The ObjectToStore is always an internal object.
		if(Context.Constants.Contains(Inst->GetObject()))
		{
			ObjectToStore = &Context.Constants[Inst->GetObject()];
		}
		else
		{
			ObjectToStore = &CurStackFrame.IntermediateObjects[Inst->GetObject()];
		}
		
		SpvPointer* Pointer = nullptr;
		SpvVariableDesc* PointeeDesc = nullptr;
		if(Context.GlobalPointers.Contains(Inst->GetPointer()))
		{
			Pointer = &Context.GlobalPointers[Inst->GetPointer()];
			SpvId VarId = *Context.GlobalVariables.FindKey(*Pointer->Pointee);
			PointeeDesc = Context.GlobalVariableDescMap.FindRef(VarId);
		}
		else
		{
			Pointer = &CurStackFrame.Pointers[Inst->GetPointer()];
			SpvId VarId = *CurStackFrame.Variables.FindKey(*Pointer->Pointee);
			PointeeDesc = CurStackFrame.VariableDescMap.FindRef(VarId);
		}
		const TArray<uint8>& ValueToStore = std::get<SpvObject::Internal>(ObjectToStore->Storage).Value;
		WritePointerValue(Pointer, PointeeDesc, ValueToStore);
		Pointer->Pointee->Initialized = true;
	}

	void SpvVmPixelVisitor::Visit(SpvOpCompositeConstruct* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvType* Type = Context.Types[Inst->GetResultType()].Get();
		TArray<uint8> CompositeValue;
		for(SpvId ConstituentId : Inst->GetConstituents())
		{
			SpvObject* ConstituentObj = nullptr;
			if(Context.Constants.Contains(ConstituentId))
			{
				ConstituentObj = &Context.Constants[ConstituentId];
			}
			else
			{
				ConstituentObj = &CurStackFrame.IntermediateObjects[ConstituentId];
			}
			CompositeValue.Append(std::get<SpvObject::Internal>(ConstituentObj->Storage).Value);
		}
		SpvObject Object = {
			.Type = Type,
			.Storage = SpvObject::Internal{ MoveTemp(CompositeValue) }
		};
		CurStackFrame.IntermediateObjects.Add(Inst->GetId().value(), MoveTemp(Object));
	}

	void SpvVmPixelVisitor::Visit(SpvOpAccessChain* Inst)
	{
		PixelThreadState& ThreadState = Context.Quad[Context.CurActiveIndex];
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		SpvPointer* BasePointer = &CurStackFrame.Pointers[Inst->GetBasePointer()];
		TArray<int32> Indexes;
		for(SpvId IndexId : Inst->GetIndexes())
		{
			int32 IndexValue = *(int32*)std::get<SpvObject::Internal>(Context.Constants[IndexId].Storage).Value.GetData();
			Indexes.Add(IndexValue);
		}
		
		SpvPointer Pointer{
			.Pointee = BasePointer->Pointee,
			.Indexes = MoveTemp(Indexes)
		};
		CurStackFrame.Pointers.Add(Inst->GetId().value(), MoveTemp(Pointer));
	}

	void SpvVmPixelVisitor::ParseQuad(int32 QuadIndex, int32 StopIndex)
	{
		Context.CurActiveIndex = QuadIndex;
		PixelThreadState& CurPixelThread = Context.Quad[QuadIndex];
		while(CurPixelThread.InstIndex < Insts->Num())
		{
			CurPixelThread.NextInstIndex = CurPixelThread.InstIndex + 1;
			if(StopIndex == CurPixelThread.InstIndex)
			{
				break;
			}
			
			(*Insts)[CurPixelThread.InstIndex]->Accpet(this);
			if(AnyError)
			{
				break;
			}
			CurPixelThread.InstIndex = CurPixelThread.NextInstIndex;
		}
	}

	int32 SpvVmPixelVisitor::GetInstIndex(SpvId Inst) const
	{
		return Insts->IndexOfByPredicate([this, Inst](const TUniquePtr<SpvInstruction>& InInst){
			return InInst->GetId() ? InInst->GetId().value() == Inst : false;
		});
	}

	void SpvVmPixelVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts)
	{
		this->Insts = &Insts;
		//Get entry point loc
		int32 InstIndex = GetInstIndex(Context.EntryPoint);
		
		//Init global external variable
		for(auto& [Id, Var] : Context.GlobalVariables)
		{
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
			}
			if(SetNumber != INDEX_NONE && BindingNumber != INDEX_NONE ;
			   SpvVmBinding* VmBinding = Context.Bindings.FindByPredicate([&](const SpvVmBinding& InItem) { return InItem.Binding == BindingNumber && InItem.DescriptorSet == SetNumber; }))
			{
				auto& Storage = std::get<SpvObject::External>(Var.Storage);
				Storage.Resource = VmBinding->Resource;
				if(VmBinding->Resource->GetType() == GpuResourceType::Buffer)
				{
					GpuBuffer* Buffer = static_cast<GpuBuffer*>(VmBinding->Resource);
					uint8* Data = (uint8*)GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Read_Only);
					Storage.Value = {Data, Buffer->GetByteSize()};
					GGpuRhi->UnMapGpuBuffer(Buffer);
				}
				Var.Initialized = true;
			}
		}
		
		for(int32 QuadIndex = 0; QuadIndex < 4; QuadIndex++)
		{
			Context.Quad[QuadIndex].InstIndex = InstIndex;
			Context.Quad[QuadIndex].RecordedInfo.AllVariables.Append(Context.GlobalVariables);
			Context.Quad[QuadIndex].RecordedInfo.AllVariableDescMap.Append(Context.GlobalVariableDescMap);
			Context.Quad[QuadIndex].StackFrames.SetNum(1);
		}
		
		ParseQuad(Context.DebugIndex);
	}
}
