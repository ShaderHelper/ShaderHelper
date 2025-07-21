#include "CommonHeader.h"
#include "SpirvVM.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32>& Indexes)
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

	TArray<uint8> GetPointerValue(SpvVmContext* InContext, SpvPointer* InPointer)
	{
		SpvVariableDesc* PointeeDesc = InContext->VariableDescMap[InPointer->Pointee->Id];
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
		
		int32 OffsetBytes = 0;
		int32 ByteSize = RetValue.Num();
		if(!InPointer->Indexes.IsEmpty())
		{
			if(PointeeDesc && PointeeDesc->TypeDesc)
			{
				SpvTypeDesc* CurTypeDesc = PointeeDesc->TypeDesc;
				for(int32 Index : InPointer->Indexes)
				{
					//External objects like uniform buffers may have different alignments and sizes,
					//so can't extract them directly from OpType* but rely on SpvVariableDesc to do it
					if(CurTypeDesc->GetKind() == SpvTypeDescKind::Composite)
					{
						SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(CurTypeDesc);
						SpvTypeDesc* IndexTypeDesc = CompositeTypeDesc->GetMemberTypeDescs()[Index];
						if(IndexTypeDesc->GetKind() == SpvTypeDescKind::Member)
						{
							SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(IndexTypeDesc);
							OffsetBytes += MemberTypeDesc->GetOffset() / 8;
							ByteSize = MemberTypeDesc->GetSize() / 8;
							CurTypeDesc = MemberTypeDesc->GetTypeDesc();
						}
					}
					else if(CurTypeDesc->GetKind() == SpvTypeDescKind::Vector)
					{
						SpvVectorTypeDesc* VectorTypeDesc = static_cast<SpvVectorTypeDesc*>(CurTypeDesc);
						SpvBasicTypeDesc* BasicTypeDesc = VectorTypeDesc->GetBasicTypeDesc();
						OffsetBytes += Index * BasicTypeDesc->GetSize() / 8;
						ByteSize = BasicTypeDesc->GetSize() / 8;
					}
				}
			}
			else
			{
				//In some cases like StructuredBuffer,
				//it may not be able to get the value correctly according to SpvVariableDesc, try getting the value from SpvType
				check(InPointer->Pointee->IsExternal());
				SpvType* CurType = InPointer->Pointee->Type;
				for(int32 Index : InPointer->Indexes)
				{
					TArray<SpvDecoration> Decorations;
					InContext->Decorations.MultiFind(CurType->GetId(), Decorations);
					
					if(CurType->GetKind() == SpvTypeKind::Struct)
					{
						SpvStructType* StructType = static_cast<SpvStructType*>(CurType);
						const SpvDecoration* CurMemberIndexDecoration = Decorations.FindByPredicate([Index](const SpvDecoration& Item){
							return Item.Kind == SpvDecorationKind::Offset && Item.Offset.MemberIndex == Index;
						});
						const SpvDecoration* NextMemberIndexDecoration = Decorations.FindByPredicate([Index](const SpvDecoration& Item){
							return Item.Kind == SpvDecorationKind::Offset && Item.Offset.MemberIndex == Index + 1;
						});
						if(Index == StructType->MemberTypes.Num() - 1)
						{
							ByteSize = ByteSize - CurMemberIndexDecoration->Offset.ByteOffset;
						}
						else
						{
							ByteSize = NextMemberIndexDecoration->Offset.ByteOffset - CurMemberIndexDecoration->Offset.ByteOffset;
						}
						OffsetBytes = CurMemberIndexDecoration->Offset.ByteOffset;
						CurType = StructType->MemberTypes[Index];
					}
					else if(CurType->GetKind() == SpvTypeKind::RuntimeArray)
					{
						SpvRuntimeArrayType* RuntimeArrayType = static_cast<SpvRuntimeArrayType*>(CurType);
						for(const auto& Decoration : Decorations)
						{
							if(Decoration.Kind == SpvDecorationKind::ArrayStride)
							{
								OffsetBytes += Index * Decoration.ArrayStride.Number;
								ByteSize = Decoration.ArrayStride.Number;
								CurType = RuntimeArrayType->ElementType;
								break;
							}
						}
					}
				}
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
			SpvTypeDesc* CurTypeDesc = PointeeDesc->TypeDesc;
			if(CurTypeDesc->GetKind() == SpvTypeDescKind::Composite)
			{
				SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(CurTypeDesc);
				SpvTypeDesc* IndexTypeDesc = CompositeTypeDesc->GetMemberTypeDescs()[Index];
				if(IndexTypeDesc->GetKind() == SpvTypeDescKind::Member)
				{
					SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(IndexTypeDesc);
					OffsetBytes += MemberTypeDesc->GetOffset() / 8;
					ByteSize = MemberTypeDesc->GetSize() / 8;
					CurTypeDesc = MemberTypeDesc->GetTypeDesc();
				}
			}
			else if(CurTypeDesc->GetKind() == SpvTypeDescKind::Vector)
			{
				SpvVectorTypeDesc* VectorTypeDesc = static_cast<SpvVectorTypeDesc*>(CurTypeDesc);
				SpvBasicTypeDesc* BasicTypeDesc = VectorTypeDesc->GetBasicTypeDesc();
				OffsetBytes += Index * BasicTypeDesc->GetSize() / 8;
				ByteSize = BasicTypeDesc->GetSize() / 8;
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
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		check(Pointer->Pointee);
		Context.VariableDescMap[Pointer->Pointee->Id] = VarDesc;
	}

	void SpvVmVisitor::Visit(SpvDebugLine* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		int32 OldLine = CurStackFrame.CurLine;
		CurStackFrame.CurLine = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineStart()].Storage).Value.GetData();
		ThreadState.RecordedInfo.DebugStates.Emplace(CurStackFrame.CurLine);
	}

	void SpvVmVisitor::Visit(SpvDebugScope* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		int32 OldLine = CurStackFrame.CurLine;
		SpvLexicalScope* OldScope = CurStackFrame.CurScope;
		SpvLexicalScope* NewScope = Context.LexicalScopes[Inst->GetScope()].Get();
		CurStackFrame.CurScope = NewScope;
		CurStackFrame.CurLine = NewScope->GetLine();

		SpvDebugState DebugState {
			.Line = CurStackFrame.CurLine,
			.ScopeChange = SpvLexicalScopeChange{OldScope, NewScope}
		};
		ThreadState.RecordedInfo.DebugStates.Add(MoveTemp(DebugState));
	}

	void SpvVmVisitor::Visit(SpvOpFunctionParameter* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvPointer* Argument = CurStackFrame.Arguments.front();
		CurStackFrame.Arguments.pop();
		CurStackFrame.Pointers.emplace(Inst->GetId().value(), *Argument);
	}

	void SpvVmVisitor::Visit(SpvOpFunctionCall* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvDebugState& CurDebugState = ThreadState.RecordedInfo.DebugStates.Last();
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
			CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ReturnObject));
			NewFrame.ReturnObject = &CurStackFrame.IntermediateObjects[Inst->GetId().value()];
		}
		ThreadState.StackFrames.push_back(MoveTemp(NewFrame));
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
		
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
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
		CurStackFrame.Variables.insert_or_assign(ResultId, MoveTemp(Var));
		
		SpvPointer Pointer{ &CurStackFrame.Variables[ResultId] };
		CurStackFrame.Pointers.insert_or_assign(ResultId, MoveTemp(Pointer));
	}

	void SpvVmVisitor::Visit(SpvOpLabel* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
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
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		SpvVariableDesc* PointeeDesc = Context.VariableDescMap[Pointer->Pointee->Id];
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		TArray<uint8> Value = GetPointerValue(&Context, Pointer);
		if(Value.IsEmpty())
		{
			ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("Reading the uninitialized variable %s !"), *PointeeDesc->Name);
			bTerminate = true;
			return;
		}
		//TODO: the result type size may not be equal to the value if it is an external object
		//We may need to remap the value result with padding to the result type
		check(GetTypeByteSize(ResultType) == Value.Num());
		
		SpvId ResultId = Inst->GetId().value();
		SpvObject Object = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ MoveTemp(Value) }
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpStore* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
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
		SpvDebugState& CurDebugState = ThreadState.RecordedInfo.DebugStates.Last();
		
		SpvVariableChange VariableChange;
		VariableChange.VarId = VarId;
		WritePointerValue(Pointer, PointeeDesc, ValueToStore, &VariableChange);
		
		CurDebugState.VarChanges.Add(MoveTemp(VariableChange));
	}

	void SpvVmVisitor::Visit(SpvOpVectorShuffle* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* VectorObject1 = GetObject(Inst->GetVector1());
		SpvObject* VectorObject2 = GetObject(Inst->GetVector2());
		check(VectorObject1->Type->GetKind() == SpvTypeKind::Vector);
		check(VectorObject2->Type->GetKind() == SpvTypeKind::Vector);
		uint32 VectorCompCount1 = static_cast<SpvVectorType*>(VectorObject1->Type)->ElementCount;
		uint32 VectorCompCount2 = static_cast<SpvVectorType*>(VectorObject2->Type)->ElementCount;
		
		TArray<uint8> ResultValue;
		for(uint32 Index : Inst->GetComponents())
		{
			if(Index >= VectorCompCount1)
			{
				ResultValue.Append(GetObjectValue(VectorObject2, {Index - VectorCompCount1}));
			}
			else
			{
				ResultValue.Append(GetObjectValue(VectorObject1, {Index}));
			}
		}
		
		SpvId ResultId = Inst->GetId().value();
		SpvObject Object = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ MoveTemp(ResultValue) }
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpCompositeConstruct* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
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
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpCompositeExtract* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvId ResultId = Inst->GetId().value();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* CompositeObject = GetObject(Inst->GetComposite());
		
		SpvObject Object = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ GetObjectValue(CompositeObject, Inst->GetIndexes()) }
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpAccessChain* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
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
		CurStackFrame.Pointers.insert_or_assign(Inst->GetId().value(), MoveTemp(Pointer));
	}

	void SpvVmVisitor::Visit(SpvOpConvertFToS* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* FloatValueObject = GetObject(Inst->GetFloatValue());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ConvertFToS));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(FloatValueObject->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(FloatValueObject));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpConvertFToS", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpConvertSToF* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* SignedValueObject = GetObject(Inst->GetSignedValue());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ConvertSToF));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(SignedValueObject->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(SignedValueObject));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpConvertSToF", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpIAdd* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::IAdd));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpIAdd", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpFAdd* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FAdd));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFAdd", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpISub* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ISub));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpISub", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpIMul* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::IMul));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpIMul", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpFMul* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FMul));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFMul", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpFSub* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FSub));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFSub", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpFDiv* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
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
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFDiv", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpBitwiseAnd* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::BitwiseAnd));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpBitwiseAnd", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpBranch* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		ThreadState.NextInstIndex = GetInstIndex(Inst->GetTargetLabel());
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
		
		ThreadState.NextInstIndex = ThreadState.StackFrames.back().ReturnPointIndex;
		SpvLexicalScope* OldScope = ThreadState.StackFrames.back().CurScope;
		int32 Line = ThreadState.StackFrames.back().CurLine;
		
		ThreadState.StackFrames.pop_back();
	
		if(!ThreadState.StackFrames.empty())
		{
			SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
			SpvLexicalScope* NewScope = CurStackFrame.CurScope;
			ThreadState.RecordedInfo.DebugStates.Last().ScopeChange = SpvLexicalScopeChange{OldScope, NewScope};
			ThreadState.RecordedInfo.DebugStates.Last().bReturn = true;
			
			//Append a debugstate for stopping at the function call after return
			ThreadState.RecordedInfo.DebugStates.Add(SpvDebugState{.Line = CurStackFrame.CurLine, .bFuncCallAfterReturn = true});
		}
	}

	void SpvVmVisitor::Visit(SpvOpReturnValue* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		ThreadState.StackFrames.back().ReturnObject->Storage = GetObject(Inst->GetValue())->Storage;
		ThreadState.NextInstIndex = ThreadState.StackFrames.back().ReturnPointIndex;
		SpvLexicalScope* OldScope = ThreadState.StackFrames.back().CurScope;
		int32 Line = ThreadState.StackFrames.back().CurLine;
		SpvObject ReturnObject = *ThreadState.StackFrames.back().ReturnObject;
		
		ThreadState.StackFrames.pop_back();
		
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvLexicalScope* NewScope = CurStackFrame.CurScope;
		ThreadState.RecordedInfo.DebugStates.Last().ReturnObject = MoveTemp(ReturnObject);
		ThreadState.RecordedInfo.DebugStates.Last().bReturn = true;
		// Split a debugstate for stopping at ending line of the function lexcical scope
		// we can't get the ending line, so the line number will be determined by the editor.
		ThreadState.RecordedInfo.DebugStates.Add(SpvDebugState{.ScopeChange = SpvLexicalScopeChange{OldScope, NewScope}, .bReturn = true});

		ThreadState.RecordedInfo.DebugStates.Add(SpvDebugState{.Line = CurStackFrame.CurLine, .bFuncCallAfterReturn = true});
	}

	void SpvVmVisitor::Visit(SpvOpVectorTimesScalar* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetVector());
		SpvObject* Operand2 = GetObject(Inst->GetScalar());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::VectorTimesScalar));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType1=%s"), *GetHlslTypeStr(Operand1->Type)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType2=%s"), *GetHlslTypeStr(Operand2->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpVectorTimesScalar", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpSelect* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Condition = GetObject(Inst->GetCondition());
		SpvObject* Object1 = GetObject(Inst->GetObject1());
		SpvObject* Object2 = GetObject(Inst->GetObject2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Select));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpConditionType=%s"), *GetHlslTypeStr(Condition->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Condition));
		Datas.Append(GetObjectValue(Object1));
		Datas.Append(GetObjectValue(Object2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpSelect", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpIEqual* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::IEqual));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Operand1->Type)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpIEqual", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
	}

	void SpvVmVisitor::Visit(SpvOpINotEqual* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::INotEqual));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Operand1->Type)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpINotEqual", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
	}

	void SpvVmVisitor::Visit(SpvOpSGreaterThan* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::SGreaterThan));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Operand1->Type)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpSGreaterThan", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
	}

	void SpvVmVisitor::Visit(SpvOpSLessThan* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::SLessThan));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Operand1->Type)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpSLessThan", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
	}

	void SpvVmVisitor::Visit(SpvOpFOrdLessThan* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FOrdLessThan));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Operand1->Type)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFOrdLessThan", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
	}

	void SpvVmVisitor::Visit(SpvOpFOrdGreaterThan* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FOrdGreaterThan));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Operand1->Type)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFOrdGreaterThan", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
	}

	void SpvVmVisitor::Visit(SpvSin* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Sin));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Sin", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvCos* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Cos));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Cos", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
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
		SpvVmFrame& CurStackFrame = Context.ThreadState.StackFrames.back();
		
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
		SpvVmFrame& CurStackFrame = Context.ThreadState.StackFrames.back();
		if(Context.Constants.contains(ObjectId))
		{
			return &Context.Constants.at(ObjectId);
		}
		else
		{
			return &CurStackFrame.IntermediateObjects.at(ObjectId);
		}
	}

	TArray<uint8> SpvVmVisitor::ExecuteGpuOp(const FString& Name, int32 ResultSize, const TArray<uint8>& InputData, const TArray<FString>& Args)
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
			.ByteSize = (uint32)InputData.Num(),
			.Usage = GpuBufferUsage::RWStorage,
			.Stride = (uint32)InputData.Num(),
			.InitialData = InputData
		});

		TRefCountPtr<GpuBindGroup> OpBindGroup = GpuBindGroupBuilder{ OpBindGroupLayout }
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
				PassRecorder->SetBindGroups(OpBindGroup, nullptr, nullptr, nullptr);
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
