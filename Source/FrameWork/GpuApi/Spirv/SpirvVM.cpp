#include "CommonHeader.h"
#include "SpirvVM.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32> Indexes)
	{
		check(InObject);
		
		const TArray<uint8>& ObjectValue = std::get<SpvObject::Internal>(InObject->Storage).Value;
		int32 OffsetBytes = 0;
		int32 ByteSize = ObjectValue.Num();
		for(uint32 Index : Indexes)
		{
			if(InObject->Type->GetKind() == SpvTypeKind::Vector)
			{
				SpvVectorType* VectorType = static_cast<SpvVectorType*>(InObject->Type);
				OffsetBytes += Index * GetTypeByteSize(VectorType->ElementType);
				ByteSize = GetTypeByteSize(VectorType->ElementType);
			}
			else if(InObject->Type->GetKind() == SpvTypeKind::Struct)
			{
				SpvStructType* StructType = static_cast<SpvStructType*>(InObject->Type);
				int MemberIndex = 0;
				for(; MemberIndex < Index; MemberIndex++)
				{
					SpvType* MemberType = StructType->MemberTypes[MemberIndex];
					OffsetBytes += GetTypeByteSize(MemberType);
				}
				ByteSize = GetTypeByteSize(StructType->MemberTypes[MemberIndex]);
			}
		}
		
		return {ObjectValue.GetData() + OffsetBytes, ByteSize};
	}

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
			//External objects like uniform buffers may have different alignments and sizes,
			//so can't extract them directly from OpType* but rely on SpvVariableDesc to do it
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
				ByteSize = BasicTypeDesc->GetSize() / 8;
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
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		Context.VariableDescMap[Pointer->Pointee->Id] = VarDesc;
	}

	void SpvVmVisitor::Visit(SpvDebugLine* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		int32 OldLine = CurStackFrame.CurLine;
		CurStackFrame.CurLine = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineStart()].Storage).Value.GetData();
		if(OldLine != CurStackFrame.CurLine)
		{
			ThreadState.RecordedInfo.LineDebugStates.Emplace(CurStackFrame.CurLine);
		}
	}

	void SpvVmVisitor::Visit(SpvDebugScope* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		int32 OldLine = CurStackFrame.CurLine;
		SpvLexicalScope* OldScope = CurStackFrame.CurScope;
		SpvLexicalScope* NewScope = Context.LexicalScopes[Inst->GetScope()].Get();
		CurStackFrame.CurScope = NewScope;
		CurStackFrame.CurLine = NewScope->GetLine();

		if(OldLine != CurStackFrame.CurLine)
		{
			SpvDebugState Debugstate {
				.Line = CurStackFrame.CurLine,
				.ScopeChange = SpvLexicalScopeChange{OldScope, NewScope}
			};
			ThreadState.RecordedInfo.LineDebugStates.Add(MoveTemp(Debugstate));
		}
	}

	void SpvVmVisitor::Visit(SpvOpFunctionParameter* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvPointer* Argument = CurStackFrame.Arguments.front();
		CurStackFrame.Arguments.pop();
		CurStackFrame.Pointers.emplace(Inst->GetId().value(), *Argument);
	}

	void SpvVmVisitor::Visit(SpvOpFunctionCall* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		SpvDebugState& CurDebugState = ThreadState.RecordedInfo.LineDebugStates.Last();
		CurDebugState.bFuncCall = true;
		
		ThreadState.NextInstIndex = GetInstIndex(Inst->GetFunction());
		std::queue<SpvPointer*> Arguments;
		for(SpvId ArgumentId : Inst->GetArguments())
		{
			Arguments.push(&CurStackFrame.Pointers[ArgumentId]);
		}
	
		SpvVmFrame NewFrame{
			.ReturnPointIndex = ThreadState.InstIndex + 1,
			.Arguments = MoveTemp(Arguments)
		};
		
		SpvType* ReturnType = Context.Types[Inst->GetResultType()].Get();
		if(ReturnType->GetKind() != SpvTypeKind::Void)
		{
			SpvObject ReturnObject{Inst->GetId().value(), ReturnType};
			CurStackFrame.IntermediateObjects.emplace(Inst->GetId().value(), MoveTemp(ReturnObject));
			NewFrame.ReturnObject = &CurStackFrame.IntermediateObjects[Inst->GetId().value()];
		}
		ThreadState.StackFrames.Add(MoveTemp(NewFrame));
	}

	void SpvVmVisitor::Visit(SpvOpVariable* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvId ResultId = Inst->GetId().value();
		
		if(!Context.Types.contains(Inst->GetResultType()))
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
		
		ThreadState.RecordedInfo.AllVariables[ResultId] =  Var;
		CurStackFrame.Variables.emplace(ResultId, MoveTemp(Var));
		
		SpvPointer Pointer{ &CurStackFrame.Variables[ResultId] };
		CurStackFrame.Pointers.emplace(ResultId, MoveTemp(Pointer));
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
		
		if(!Context.Types.contains(Inst->GetResultType()))
		{
			return;
		}
		;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		SpvVariableDesc* PointeeDesc = Context.VariableDescMap[Pointer->Pointee->Id];
		TArray<uint8> Value = GetPointerValue(Pointer, PointeeDesc);
		if(Value.IsEmpty())
		{
			ThreadState.RecordedInfo.LineDebugStates.Last().UbError = FString::Printf(TEXT("Reading the uninitialized variable %s !"), *PointeeDesc->Name);
			AnyError = true;
			return;
		}
		SpvId ResultId = Inst->GetId().value();
		SpvObject Object = {
			.Id = ResultId,
			.Type = Context.Types[Inst->GetResultType()].Get(),
			.Storage = SpvObject::Internal{ MoveTemp(Value) }
		};
		CurStackFrame.IntermediateObjects.emplace(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpStore* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		if(!Context.GlobalPointers.contains(Inst->GetPointer()) &&
		   !CurStackFrame.Pointers.contains(Inst->GetPointer()))
		{
			return;
		}
		
		SpvObject* ObjectToStore = GetObject(Inst->GetObject());
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		SpvId VarId = Pointer->Pointee->Id;
		SpvVariableDesc* PointeeDesc = Context.VariableDescMap[VarId];
		const TArray<uint8>& ValueToStore = GetObjectValue(ObjectToStore);
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
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		TArray<uint8> CompositeValue;
		for(SpvId ConstituentId : Inst->GetConstituents())
		{
			SpvObject* ConstituentObj = GetObject(ConstituentId);
			CompositeValue.Append(GetObjectValue(ConstituentObj));
		}
		SpvId ResultId = Inst->GetId().value();
		SpvObject Object = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ MoveTemp(CompositeValue) }
		};
		CurStackFrame.IntermediateObjects.emplace(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpCompositeExtract* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvId ResultId = Inst->GetId().value();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* CompositeObject = GetObject(Inst->GetComposite());
		
		SpvObject Object = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ GetObjectValue(CompositeObject, Inst->GetIndexes()) }
		};
		CurStackFrame.IntermediateObjects.emplace(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpAccessChain* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvPointer* BasePointer = GetPointer(Inst->GetBasePointer());
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
		CurStackFrame.Pointers.emplace(Inst->GetId().value(), MoveTemp(Pointer));
	}

	void SpvVmVisitor::Visit(SpvOpFDiv* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FDiv));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteOp("OpFDiv", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.emplace(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpBranchConditional* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvObject* Condition = GetObject(Inst->GetCondition());
		uint32 ConditionValue = *(uint32*)GetObjectValue(Condition).GetData();
		if(ConditionValue == 1)
		{
			ThreadState.NextInstIndex = GetInstIndex(Inst->GetTrueLabel());
		}
		else
		{
			ThreadState.NextInstIndex = GetInstIndex(Inst->GetFalseLabel());
		}
	}

	void SpvVmVisitor::Visit(SpvOpReturn* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		
		ThreadState.NextInstIndex = ThreadState.StackFrames.Last().ReturnPointIndex;
		SpvLexicalScope* OldScope = ThreadState.StackFrames.Last().CurScope;
		ThreadState.StackFrames.Pop();
	
		if(!ThreadState.StackFrames.IsEmpty())
		{
			SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
			SpvLexicalScope* NewScope = CurStackFrame.CurScope;
			
			SpvDebugState Debugstate {
				.Line = CurStackFrame.CurLine,
				.ScopeChange = SpvLexicalScopeChange{OldScope, NewScope}
			};
			ThreadState.RecordedInfo.LineDebugStates.Add(MoveTemp(Debugstate));
		}
	}

	void SpvVmVisitor::Visit(SpvOpReturnValue* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		ThreadState.StackFrames.Last().ReturnObject->Storage = GetObject(Inst->GetValue())->Storage;
		ThreadState.NextInstIndex = ThreadState.StackFrames.Last().ReturnPointIndex;
		SpvLexicalScope* OldScope = ThreadState.StackFrames.Last().CurScope;
		ThreadState.StackFrames.Pop();
		
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		SpvLexicalScope* NewScope = CurStackFrame.CurScope;
		
		SpvDebugState Debugstate {
			.Line = CurStackFrame.CurLine,
			.ScopeChange = SpvLexicalScopeChange{OldScope, NewScope}
		};
		ThreadState.RecordedInfo.LineDebugStates.Add(MoveTemp(Debugstate));
	}

	void SpvVmVisitor::Visit(SpvOpIEqual* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> OperandValue1 = GetObjectValue(Operand1);
		TArray<uint8> OperandValue2 = GetObjectValue(Operand2);
		SpvType* OperandType = Operand1->Type;
		
		TArray<uint8> ResultValue;
		ResultValue.SetNumZeroed(GetTypeByteSize(ResultType));
		
		auto IntegerEqualComparison = [&](SpvIntegerType* IntegerType, int32 Index = 0){
			check(IntegerType->GetWidth() == 32);
			if(IntegerType->IsSigend())
			{
				int32 OperandTypedValue1 = *(int32*)OperandValue1.GetData();
				int32 OperandTypedValue2 = *(int32*)OperandValue2.GetData();
				*((uint32*)ResultValue.GetData() + Index) = OperandTypedValue1 == OperandTypedValue2;
			}
			else
			{
				uint32 OperandTypedValue1 = *(uint32*)OperandValue1.GetData();
				uint32 OperandTypedValue2 = *(uint32*)OperandValue2.GetData();
				*((uint32*)ResultValue.GetData() + Index) = OperandTypedValue1 == OperandTypedValue2;
			}
		};
		
		if(OperandType->GetKind() == SpvTypeKind::Vector)
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(OperandType);
			check(VectorType->ElementType->GetKind() == SpvTypeKind::Integer);
			for(int32 Index = 0; Index < VectorType->ElementCount; Index++)
			{
				SpvIntegerType* IntegerType = static_cast<SpvIntegerType*>(VectorType->ElementType);
				IntegerEqualComparison(IntegerType, Index);
			}
		}
		else
		{
			check(OperandType->GetKind() == SpvTypeKind::Integer);
			SpvIntegerType* IntegerType = static_cast<SpvIntegerType*>(OperandType);
			IntegerEqualComparison(IntegerType);
		}
		
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.emplace(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpINotEqual* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.Last();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> OperandValue1 = GetObjectValue(Operand1);
		TArray<uint8> OperandValue2 = GetObjectValue(Operand2);
		SpvType* OperandType = Operand1->Type;
		
		TArray<uint8> ResultValue;
		ResultValue.SetNumZeroed(GetTypeByteSize(ResultType));
		
		auto IntegerNotEqualComparison = [&](SpvIntegerType* IntegerType, int32 Index = 0){
			//TODO
			check(IntegerType->GetWidth() == 32);
			if(IntegerType->IsSigend())
			{
				int32 OperandTypedValue1 = *(int32*)OperandValue1.GetData();
				int32 OperandTypedValue2 = *(int32*)OperandValue2.GetData();
				*((uint32*)ResultValue.GetData() + Index) = OperandTypedValue1 != OperandTypedValue2;
			}
			else
			{
				uint32 OperandTypedValue1 = *(uint32*)OperandValue1.GetData();
				uint32 OperandTypedValue2 = *(uint32*)OperandValue2.GetData();
				*((uint32*)ResultValue.GetData() + Index) = OperandTypedValue1 != OperandTypedValue2;
			}
		};
		
		if(OperandType->GetKind() == SpvTypeKind::Vector)
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(OperandType);
			check(VectorType->ElementType->GetKind() == SpvTypeKind::Integer);
			for(int32 Index = 0; Index < VectorType->ElementCount; Index++)
			{
				SpvIntegerType* IntegerType = static_cast<SpvIntegerType*>(VectorType->ElementType);
				IntegerNotEqualComparison(IntegerType, Index);
			}
		}
		else
		{
			check(OperandType->GetKind() == SpvTypeKind::Integer);
			SpvIntegerType* IntegerType = static_cast<SpvIntegerType*>(OperandType);
			IntegerNotEqualComparison(IntegerType);
		}
		
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.emplace(ResultId, MoveTemp(ResultObject));
	}

	int32 SpvVmVisitor::GetInstIndex(SpvId Inst) const
	{
		return Insts->IndexOfByPredicate([this, Inst](const TUniquePtr<SpvInstruction>& InInst){
			return InInst->GetId() ? InInst->GetId().value() == Inst : false;
		});
	}

	SpvPointer* SpvVmVisitor::GetPointer(SpvId PointerId)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvVmFrame& CurStackFrame = Context.ThreadState.StackFrames.Last();
		
		if(Context.GlobalPointers.contains(PointerId))
		{
			return &Context.GlobalPointers[PointerId];
		}
		else
		{
			return &CurStackFrame.Pointers[PointerId];
		}
	}

	SpvObject* SpvVmVisitor::GetObject(SpvId ObjectId)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvVmFrame& CurStackFrame = Context.ThreadState.StackFrames.Last();
		if(Context.Constants.contains(ObjectId))
		{
			return &Context.Constants[ObjectId];
		}
		else
		{
			return &CurStackFrame.IntermediateObjects[ObjectId];
		}
	}

	TArray<uint8> SpvVmVisitor::ExecuteOp(const FString& Name, int32 ResultSize, const TArray<uint8>& InputData, const TArray<FString>& Args)
	{
		static TRefCountPtr<GpuShader> OpShader = GGpuRhi->CreateShaderFromFile({
					.FileName = PathHelper::ShaderDir() / "ShaderHelper/Debugger/Op.hlsl",
					.Type = ShaderType::ComputeShader,
					.EntryPoint = "MainCS"
		});
		static TRefCountPtr<GpuBindGroupLayout> OpBindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
			.AddExistingBinding(0, BindingType::RWStorageBuffer, BindingShaderStage::Compute)
			.Build();
		
		FString ErrorInfo;
		GGpuRhi->CompileShader(OpShader, ErrorInfo, Args);
		check(ErrorInfo.IsEmpty());
		
		TRefCountPtr<GpuBuffer> Input = GGpuRhi->CreateBuffer({
			.ByteSize = InputData.Num(),
			.Usage = GpuBufferUsage::RWStorage,
			.Stride = InputData.Num(),
			.InitialData = InputData
		});

		TRefCountPtr<GpuBindGroup> OpFDivBindGroup = GpuBindGroupBuilder{ OpBindGroupLayout }
			.SetExistingBinding(0, Input)
			.Build();
		
		TRefCountPtr<GpuComputePipelineState> Pipeline = GGpuRhi->CreateComputePipelineState({
			.Cs = OpShader,
			.BindGroupLayout0 = OpBindGroupLayout
		});
		
		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginComputePass(Name);
			{
				PassRecorder->SetComputePipelineState(Pipeline);
				PassRecorder->SetBindGroups(OpFDivBindGroup, nullptr, nullptr, nullptr);
				PassRecorder->Dispatch(1, 1, 1);
			}
			CmdRecorder->EndComputePass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
		
		TArray<uint8> ResultValue;
		uint8* InputValue = (uint8*)GGpuRhi->MapGpuBuffer(Input, GpuResourceMapMode::Read_Only);
		ResultValue = {InputValue, ResultSize};
		GGpuRhi->UnMapGpuBuffer(Input);
		return ResultValue;
	}

	void SpvVmVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts)
	{
		this->Insts = &Insts;
	}
}
