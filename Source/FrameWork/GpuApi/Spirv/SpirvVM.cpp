#include "CommonHeader.h"
#include "SpirvVM.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	TArray<uint8> GetPointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc)
	{
		check(InPointer);
		check(InPointer->Indexes.IsEmpty() || PointeeDesc);
		if(!InPointer->Pointee->Initialized)
		{
			return {};
		}
		
		TArray<uint8> RetValue;
		if(InPointer->Pointee->IsExternal())
		{
			RetValue = std::get<SpvObject::External>(InPointer->Pointee->Storage).Value;
		}
		else
		{
			RetValue = std::get<SpvObject::Internal>(InPointer->Pointee->Storage).Value;
		}
		
		SpvTypeDesc* BaseTypeDesc = PointeeDesc ? PointeeDesc->TypeDesc : nullptr;
		int32 OffsetBytes = 0;
		int32 ByteSize = RetValue.Num();
		for(int32 Index : InPointer->Indexes)
		{
			if(BaseTypeDesc->GetKind() == SpvTypeDescKind::Composite)
			{
				SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(BaseTypeDesc);
				SpvTypeDesc* IndexTypeDesc = CompositeTypeDesc->GetMemberTypeDescs()[Index];
				if(IndexTypeDesc->GetKind() == SpvTypeDescKind::Member)
				{
					SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(IndexTypeDesc);
					OffsetBytes += MemberTypeDesc->GetOffset();
					ByteSize = MemberTypeDesc->GetSize() / 8;
					BaseTypeDesc = MemberTypeDesc->GetTypeDesc();
				}
			}
			else if(BaseTypeDesc->GetKind() == SpvTypeDescKind::Vector)
			{
				SpvVectorTypeDesc* VectorTypeDesc = static_cast<SpvVectorTypeDesc*>(BaseTypeDesc);
				SpvBasicTypeDesc* BasicTypeDesc = VectorTypeDesc->GetBasicTypeDesc();
				OffsetBytes += Index * BasicTypeDesc->GetSize() / 8;
				ByteSize = BasicTypeDesc->GetSize();
			}
		}
		return {RetValue.GetData() + OffsetBytes, ByteSize};
	}

	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore, SpvVariableChange* OutVariableChange)
	{
		check(InPointer);
		check(InPointer->Indexes.IsEmpty() || PointeeDesc);
		
		TArray<uint8>* ValueRef;
		if(InPointer->Pointee->IsExternal())
		{
			ValueRef = &std::get<SpvObject::External>(InPointer->Pointee->Storage).Value;
		}
		else
		{
			ValueRef = &std::get<SpvObject::Internal>(InPointer->Pointee->Storage).Value;
		}
		
		if(OutVariableChange)
		{
			OutVariableChange->PreValue = *ValueRef;
		}
		
		int32 OffsetBytes = 0;
		int32 ByteSize = ValueRef->Num();
		for(int32 Index : InPointer->Indexes)
		{
			SpvTypeDesc* BaseTypeDesc = PointeeDesc->TypeDesc;
			if(BaseTypeDesc->GetKind() == SpvTypeDescKind::Composite)
			{
				SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(BaseTypeDesc);
				SpvTypeDesc* IndexTypeDesc = CompositeTypeDesc->GetMemberTypeDescs()[Index];
				if(IndexTypeDesc->GetKind() == SpvTypeDescKind::Member)
				{
					SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(IndexTypeDesc);
					OffsetBytes += MemberTypeDesc->GetOffset();
					ByteSize = MemberTypeDesc->GetSize() / 8;
					BaseTypeDesc = MemberTypeDesc->GetTypeDesc();
				}
			}
			else if(BaseTypeDesc->GetKind() == SpvTypeDescKind::Vector)
			{
				SpvVectorTypeDesc* VectorTypeDesc = static_cast<SpvVectorTypeDesc*>(BaseTypeDesc);
				SpvBasicTypeDesc* BasicTypeDesc = VectorTypeDesc->GetBasicTypeDesc();
				OffsetBytes += Index * BasicTypeDesc->GetSize() / 8;
				ByteSize = BasicTypeDesc->GetSize();
			}
		}
		check(ByteSize == ValueToStore.Num());
		FMemory::Memcpy(ValueRef->GetData() + OffsetBytes, ValueToStore.GetData(), ByteSize);
		InPointer->Pointee->Initialized = true;
		
		if(OutVariableChange)
		{
			OutVariableChange->Range = {OffsetBytes, ByteSize};
			OutVariableChange->NewValue = *ValueRef;
		}
	}

	void SpvVmVisitor::Visit(SpvDebugDeclare* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		SpvPointer* Pointer = nullptr;
		if(Context.GlobalPointers.Contains(Inst->GetPointer()))
		{
			Pointer = &Context.GlobalPointers[Inst->GetPointer()];
		}
		else
		{
			Pointer = &CurStackFrame.Pointers[Inst->GetPointer()];
		}
		Context.VariableDescMap.FindOrAdd(Pointer->Pointee->Id, VarDesc);
	}

	void SpvVmVisitor::Visit(SpvDebugLine* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		int32 OldLineNumber = CurStackFrame.CurLineNumber;
		CurStackFrame.CurLineNumber = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineStart()].Storage).Value.GetData();
		if(OldLineNumber != CurStackFrame.CurLineNumber)
		{
			ThreadState.RecordedInfo.LineDebugStates.Emplace(CurStackFrame.CurLineNumber);
		}
	}

	void SpvVmVisitor::Visit(SpvDebugScope* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
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

	void SpvVmVisitor::Visit(SpvOpFunctionParameter* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		CurStackFrame.Pointers.Add(Inst->GetId().value(), *CurStackFrame.Arguments.Pop());
	}

	void SpvVmVisitor::Visit(SpvOpFunctionCall* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
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
			SpvObject ReturnObject{Inst->GetId().value(), ReturnType};
			CurStackFrame.IntermediateObjects.Add(Inst->GetId().value(), MoveTemp(ReturnObject));
			NewFrame.ReturnObject = &CurStackFrame.IntermediateObjects[Inst->GetId().value()];
		}
		ThreadState.StackFrames.Add(MoveTemp(NewFrame));
	}

	void SpvVmVisitor::Visit(SpvOpVariable* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvId ResultId = Inst->GetId().value();
		
		if(!Context.Types.Contains(Inst->GetResultType()))
		{
			return;
		}
		
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{ResultId, PointerType->PointeeType}, false, StorageClass};
		if(StorageClass == SpvStorageClass::Uniform || StorageClass == SpvStorageClass::UniformConstant)
		{
			Var.Storage = SpvObject::External{};
		}
		else
		{
			TArray<uint8> Value;
			Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
			Var.Storage = SpvObject::Internal{MoveTemp(Value)};
		}
		
		ThreadState.RecordedInfo.AllVariables.FindOrAdd(ResultId, Var);
		CurStackFrame.Variables.Add(ResultId, MoveTemp(Var));
		
		SpvPointer Pointer{ &CurStackFrame.Variables[ResultId] };
		CurStackFrame.Pointers.Add(ResultId, MoveTemp(Pointer));
	}

	void SpvVmVisitor::Visit(SpvOpLabel* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		CurStackFrame.PreBasicBlock = CurStackFrame.CurBasicBlock;
		CurStackFrame.CurBasicBlock = Inst->GetId().value();
	}

	void SpvVmVisitor::Visit(SpvOpLoad* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		
		if(!Context.Types.Contains(Inst->GetResultType()))
		{
			return;
		}
		;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvPointer* Pointer = nullptr;
		SpvVariableDesc* PointeeDesc = nullptr;
		if(Context.GlobalPointers.Contains(Inst->GetPointer()))
		{
			Pointer = &Context.GlobalPointers[Inst->GetPointer()];
			PointeeDesc = Context.VariableDescMap.FindRef(Pointer->Pointee->Id);
		}
		else
		{
			Pointer = &CurStackFrame.Pointers[Inst->GetPointer()];
			PointeeDesc = Context.VariableDescMap.FindRef(Pointer->Pointee->Id);
		}
		
		TArray<uint8> Value = GetPointerValue(Pointer, PointeeDesc);
		if(Value.IsEmpty())
		{
			ThreadState.RecordedInfo.LineDebugStates.Last().UbError = FString::Printf(TEXT("Reading the uninitialized variable %s !"), *PointeeDesc->Name);
			AnyError = true;
			return;
		}
		
		SpvObject Object = {
			.Type = Context.Types[Inst->GetResultType()].Get(),
			.Storage = SpvObject::Internal{ MoveTemp(Value) }
		};
		CurStackFrame.IntermediateObjects.Add(Inst->GetId().value(), MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpStore* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
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
		}
		else
		{
			Pointer = &CurStackFrame.Pointers[Inst->GetPointer()];
		}
		SpvId VarId = Pointer->Pointee->Id;
		PointeeDesc = Context.VariableDescMap.FindRef(VarId);
		const TArray<uint8>& ValueToStore = std::get<SpvObject::Internal>(ObjectToStore->Storage).Value;
		SpvDebugState& CurDebugState = ThreadState.RecordedInfo.LineDebugStates.Last();
		
		SpvVariableChange VariableChange;
		VariableChange.VarId = VarId;
		WritePointerValue(Pointer, PointeeDesc, ValueToStore, &VariableChange);
		
		CurDebugState.VarChanges.Add(MoveTemp(VariableChange));
	}

	void SpvVmVisitor::Visit(SpvOpCompositeConstruct* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
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

	void SpvVmVisitor::Visit(SpvOpAccessChain* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		SpvPointer* BasePointer = &CurStackFrame.Pointers[Inst->GetBasePointer()];
		TArray<int32> Indexes = BasePointer->Indexes;
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

	void SpvVmVisitor::Visit(SpvOpIEqual* Inst)
	{
		
	}

	void SpvVmVisitor::Visit(SpvOpINotEqual* Inst)
	{
		
	}

	int32 SpvVmVisitor::GetInstIndex(SpvId Inst) const
	{
		return Insts->IndexOfByPredicate([this, Inst](const TUniquePtr<SpvInstruction>& InInst){
			return InInst->GetId() ? InInst->GetId().value() == Inst : false;
		});
	}

	void SpvVmVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts)
	{
		this->Insts = &Insts;
	}
}
