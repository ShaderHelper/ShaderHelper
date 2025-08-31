#include "CommonHeader.h"
#include "SpirvVM.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32>& Indexes)
	{
		check(InObject);
		check(!InObject->IsExternal());
		const TArray<uint8>& ObjectValue = std::get<SpvObject::Internal>(InObject->Storage).Value;
		int32 OffsetBytes = 0;
		int32 ByteSize = ObjectValue.Num();
		SpvType* CurType = InObject->Type;
		for(uint32 Index : Indexes)
		{
			if(CurType->GetKind() == SpvTypeKind::Vector)
			{
				SpvVectorType* VectorType = static_cast<SpvVectorType*>(CurType);
				OffsetBytes += Index * GetTypeByteSize(VectorType->ElementType);
				ByteSize = GetTypeByteSize(VectorType->ElementType);
			}
			else if(CurType->GetKind() == SpvTypeKind::Struct)
			{
				SpvStructType* StructType = static_cast<SpvStructType*>(CurType);
				SpvType* MemberType = StructType->MemberTypes[Index];
				OffsetBytes += GetTypeByteSize(MemberType);
				ByteSize = GetTypeByteSize(StructType->MemberTypes[Index]);
				CurType = MemberType;
			}
			else if(CurType->GetKind() == SpvTypeKind::Array)
			{
				SpvArrayType* ArrayType = static_cast<SpvArrayType*>(CurType);
				OffsetBytes += Index * GetTypeByteSize(ArrayType->ElementType);
				ByteSize = GetTypeByteSize(ArrayType->ElementType);
				CurType = ArrayType->ElementType;
			}
			else if(CurType->GetKind() == SpvTypeKind::Matrix)
			{
				SpvMatrixType* MatrixType = static_cast<SpvMatrixType*>(CurType);
				OffsetBytes += Index * GetTypeByteSize(MatrixType->ElementType);
				ByteSize = GetTypeByteSize(MatrixType->ElementType);
				CurType = MatrixType->ElementType;
			}
			else
			{
				AUX::Unreachable();
			}
		}
		
		return {ObjectValue.GetData() + OffsetBytes, ByteSize};
	}

	TArray<uint8> GetPointerValue(SpvVmContext* InContext, SpvPointer* InPointer)
	{
		SpvVariableDesc* PointeeDesc = InContext->VariableDescMap[InPointer->Pointee->Id];
		
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
				for(int32 Layer = 0; Layer < InPointer->Indexes.Num(); Layer++ )
				{
					int32 Index = InPointer->Indexes[Layer];
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
					else if(CurTypeDesc->GetKind() == SpvTypeDescKind::Matrix)
					{
						SpvMatrixTypeDesc* MatrixTypeDesc = static_cast<SpvMatrixTypeDesc*>(CurTypeDesc);
						SpvVectorTypeDesc* ElementTypeDesc = MatrixTypeDesc->GetVectorTypeDesc();
						int32 ElementTypeSize = GetTypeByteSize(ElementTypeDesc);
						OffsetBytes += Index * ElementTypeSize;
						ByteSize = ElementTypeSize;
						CurTypeDesc = ElementTypeDesc;
					}
					else if(CurTypeDesc->GetKind() == SpvTypeDescKind::Array)
					{
						SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(CurTypeDesc);
						int32 BaseTypeSize = GetTypeByteSize(ArrayTypeDesc->GetBaseTypeDesc());
						int32 ElementNum = 1;
						for(int32 i = Layer + 1; i < ArrayTypeDesc->GetCompCounts().Num(); i++)
						{
							ElementNum *= ArrayTypeDesc->GetCompCounts()[i];
						}
						int32 ElementTypeSize = BaseTypeSize * ElementNum;
						OffsetBytes += Index * ElementTypeSize;
						ByteSize = ElementTypeSize;
					}
					
					bool ValueInitialized = false;
					for(const auto& Range : InPointer->Pointee->InitializedRanges)
					{
						if(OffsetBytes >= Range.X && OffsetBytes + ByteSize <= Range.Y)
						{
							ValueInitialized = true;
							break;
						}
					}
					if(!ValueInitialized)
					{
						return {};
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
					else
					{
						AUX::Unreachable();
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
		for(int32 Layer = 0; Layer < InPointer->Indexes.Num(); Layer++ )
		{
			int32 Index = InPointer->Indexes[Layer];
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
			else if(CurTypeDesc->GetKind() == SpvTypeDescKind::Matrix)
			{
				SpvMatrixTypeDesc* MatrixTypeDesc = static_cast<SpvMatrixTypeDesc*>(CurTypeDesc);
				SpvVectorTypeDesc* ElementTypeDesc = MatrixTypeDesc->GetVectorTypeDesc();
				int32 ElementTypeSize = GetTypeByteSize(ElementTypeDesc);
				OffsetBytes += Index * ElementTypeSize;
				ByteSize = ElementTypeSize;
				CurTypeDesc = ElementTypeDesc;
			}
			else if(CurTypeDesc->GetKind() == SpvTypeDescKind::Array)
			{
				SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(CurTypeDesc);
				int32 BaseTypeSize = GetTypeByteSize(ArrayTypeDesc->GetBaseTypeDesc());
				int32 ElementNum = 1;
				for(int32 i = Layer + 1; i < ArrayTypeDesc->GetCompCounts().Num(); i++)
				{
					ElementNum *= ArrayTypeDesc->GetCompCounts()[i];
				}
				int32 ElementTypeSize = BaseTypeSize * ElementNum;
				OffsetBytes += Index * ElementTypeSize;
				ByteSize = ElementTypeSize;
			}
		}
		check(ByteSize == ValueToStore.Num());
		FMemory::Memcpy(ValueRef->GetData() + OffsetBytes, ValueToStore.GetData(), ByteSize);
		InPointer->Pointee->InitializedRanges.AddUnique({OffsetBytes, OffsetBytes + ByteSize});
		
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
		Context.VariableDescMap[Inst->GetVariable()] = VarDesc;
	}

	void SpvVmVisitor::Visit(SpvDebugValue* Inst)
	{

	}

	void SpvVmVisitor::Visit(SpvDebugFunctionDefinition* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();

		if(!CurStackFrame.Parameters.empty())
		{
			SpvFunctionDesc* CurFuncDesc = static_cast<SpvFunctionDesc*>(Context.LexicalScopes[Inst->GetFunction()].Get());
			ShaderFunc* EditorFunc = Context.EditorFuncInfo.FindByPredicate([&](const ShaderFunc& InItem) {
				return InItem.Name == CurFuncDesc->GetName() && InItem.Start.x == CurFuncDesc->GetLine();
			});
			for(int i = 0; i < CurStackFrame.Parameters.size(); i++)
			{
				SpvVmParameter& Parameter = CurStackFrame.Parameters[i];
				SpvPointer* Argument = CurStackFrame.Arguments[i];
				FString ParameterName = Context.Names[Parameter.Pointer->Pointee->Id];
				const ShaderParameter& EditorParameter = EditorFunc->Params[i];
				Parameter.Flag = EditorParameter.SemaFlag;
				if(EditorParameter.SemaFlag != ParamSemaFlag::Out)
				{
					SpvId VarId = Parameter.Pointer->Pointee->Id;
					SpvVariableDesc* PointeeDesc = Context.VariableDescMap[VarId];
					const TArray<uint8>& ValueToStore = GetPointerValue(&Context, Argument);
					SpvDebugState& CurDebugState = ThreadState.RecordedInfo.DebugStates.Last();

					SpvVariableChange VariableChange;
					VariableChange.VarId = VarId;
					WritePointerValue(Parameter.Pointer, PointeeDesc, ValueToStore, &VariableChange);

					CurDebugState.VarChanges.Add(MoveTemp(VariableChange));
					CurDebugState.bParamChange = true;
				}
			}
		}
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
		SpvId ResultId = Inst->GetId().value();
		
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = { {ResultId, PointerType->PointeeType}, SpvStorageClass::Function};

		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
		Var.Storage = SpvObject::Internal{ MoveTemp(Value) };

		ThreadState.RecordedInfo.AllVariables[ResultId] = Var;
		CurStackFrame.Variables.insert_or_assign(ResultId, MoveTemp(Var));
		SpvPointer Pointer{
			.Id = ResultId,
			.Pointee = &CurStackFrame.Variables[ResultId]
		};
		CurStackFrame.Pointers.insert_or_assign(ResultId, MoveTemp(Pointer));
		SpvVmParameter Parameter{ &CurStackFrame.Pointers.at(ResultId) };
		CurStackFrame.Parameters.push_back(MoveTemp(Parameter));
	}

	void SpvVmVisitor::Visit(SpvOpFunctionCall* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvDebugState& CurDebugState = ThreadState.RecordedInfo.DebugStates.Last();
		CurDebugState.bFuncCall = true;
		
		ThreadState.NextInstIndex = GetInstIndex(Inst->GetFunction());
		std::vector<SpvPointer*> Arguments;
		for(SpvId ArgumentId : Inst->GetArguments())
		{
			if(Context.GlobalPointers.contains(ArgumentId))
			{
				Arguments.push_back(&Context.GlobalPointers.at(ArgumentId));
			}
			else
			{
				Arguments.push_back(&CurStackFrame.Pointers.at(ArgumentId));
			}
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
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{ResultId, PointerType->PointeeType}, StorageClass};

		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
		Var.Storage = SpvObject::Internal{MoveTemp(Value)};
		
		ThreadState.RecordedInfo.AllVariables[ResultId] =  Var;
		CurStackFrame.Variables.insert_or_assign(ResultId, MoveTemp(Var));
		
		SpvPointer Pointer{
			.Id = ResultId,
			.Pointee = &CurStackFrame.Variables[ResultId]
		};
		CurStackFrame.Pointers.insert_or_assign(ResultId, MoveTemp(Pointer));
	}

	void SpvVmVisitor::Visit(SpvOpPhi* Inst)
	{

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
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvPointer* Pointer = GetPointer(Inst->GetPointer());
		SpvVariableDesc* PointeeDesc = Context.VariableDescMap[Pointer->Pointee->Id];
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<uint8> Value;
		TArray<GpuResource*> Resources;
		if(Pointer->Pointee->IsOpaque() && Pointer->Pointee->IsExternal())
		{
			Resources.Add(std::get<SpvObject::External>(Pointer->Pointee->Storage).Resource);
		}
		else
		{
			if(Pointer->Pointee->InitializedRanges.IsEmpty() & EnableUbsan)
			{
				//The variable name cannot be obtained from the PointeeDesc because DebugDeclare appears before OpLoad in the following case:
				//float a = a + 1;
				ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("Reading the uninitialized memory of the variable: %s !"), *Context.Names[Pointer->Pointee->Id]);
				bTerminate = true;
				return;
			}
			//TODO: the result type size may not be equal to the value if it is an external object
			//We may need to remap the value result with padding to the result type
			//check(GetTypeByteSize(ResultType) == Value.Num());
			TArray<uint8> VarValue = GetPointerValue(&Context, Pointer);
			Value = { VarValue.GetData(), GetTypeByteSize(ResultType) };
		}
		
		SpvId ResultId = Inst->GetId().value();
		SpvObject Object = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ MoveTemp(Value), Resources }
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
		
		SpvObject* ObjectToStore = GetObject(&Context, Inst->GetObject());
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
		SpvObject* VectorObject1 = GetObject(&Context, Inst->GetVector1());
		SpvObject* VectorObject2 = GetObject(&Context, Inst->GetVector2());
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
			SpvObject* ConstituentObj = GetObject(&Context, ConstituentId);
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
		SpvObject* CompositeObject = GetObject(&Context, Inst->GetComposite());
		
		SpvObject Object = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ GetObjectValue(CompositeObject, Inst->GetIndexes()) }
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(Object));
	}

	void SpvVmVisitor::Visit(SpvOpTranspose* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Matrix = GetObject(&Context, Inst->GetMatrix());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Transpose));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Matrix->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Matrix));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpTranspose", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
		
	}

	void SpvVmVisitor::Visit(SpvOpAccessChain* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvId ResultId = Inst->GetId().value();
		
		SpvPointer* BasePointer = GetPointer(Inst->GetBasePointer());
		TArray<int32> Indexes = BasePointer->Indexes;
		for(SpvId IndexId : Inst->GetIndexes())
		{
			int32 IndexValue = *(int32*)std::get<SpvObject::Internal>(Context.Constants[IndexId].Storage).Value.GetData();
			Indexes.Add(IndexValue);
		}
		
		SpvPointer Pointer{
			.Id = ResultId,
			.Pointee = BasePointer->Pointee,
			.Indexes = MoveTemp(Indexes)
		};
		CurStackFrame.Pointers.insert_or_assign(ResultId, MoveTemp(Pointer));
	}

	void SpvVmVisitor::Visit(SpvOpConvertFToU* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* FloatValueObject = GetObject(&Context, Inst->GetFloatValue());
		const TArray<uint8>& FloatValue = GetObjectValue(FloatValueObject);

		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (FloatValueObject->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(FloatValueObject->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				float f = *reinterpret_cast<const float*>(FloatValue.GetData() + i * 4);
				if (f < 0.0f || uint64(f) > uint64(std::numeric_limits<uint32>::max()))
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("Conversion(floating point to unsigned integer) is undefined for component %d: Result type is not wide enough to hold the converted value(%f)."), i, f);
					bTerminate = true;
					return;
				}
			}
		}

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ConvertFToU));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(FloatValueObject->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(FloatValue);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpConvertFToU", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpConvertFToS* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* FloatValueObject = GetObject(&Context, Inst->GetFloatValue());
		const TArray<uint8>& FloatValue = GetObjectValue(FloatValueObject);
		
		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (FloatValueObject->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(FloatValueObject->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				float f = *reinterpret_cast<const float*>(FloatValue.GetData() + i * 4);
				if(int64(f) < int64(std::numeric_limits<int32>::min()) || int64(f) > int64(std::numeric_limits<int32>::max()))
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("Conversion(floating point to signed integer) is undefined for component %d: It's not wide enough to hold the converted value(%f)."), i, f);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ConvertFToS));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(FloatValueObject->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(FloatValue);
		
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
		SpvObject* SignedValueObject = GetObject(&Context, Inst->GetSignedValue());
		
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

	void SpvVmVisitor::Visit(SpvOpConvertUToF* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* UnsignedValueObject = GetObject(&Context, Inst->GetUnsignedValue());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ConvertUToF));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(UnsignedValueObject->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(UnsignedValueObject));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpConvertUToF", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpBitcast* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand = GetObject(&Context, Inst->GetOperand());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Bitcast));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Operand->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpBitcast", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpSNegate* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::SNegate));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand = GetObject(&Context, Inst->GetOperand());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpSNegate", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpFNegate* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FNegate));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand = GetObject(&Context, Inst->GetOperand());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFNegate", GetTypeByteSize(ResultType), Datas, ExtraArgs);
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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

	void SpvVmVisitor::Visit(SpvOpUDiv* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();

		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
    	SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		const TArray<uint8>& Operand1Value = GetObjectValue(Operand1);
   		const TArray<uint8>& Operand2Value = GetObjectValue(Operand2);

		if (EnableUbsan)
		{
			int ElemCount = 1;
			if (Operand2->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(Operand2->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				uint32 v = *reinterpret_cast<const uint32*>(Operand2Value.GetData() + i * 4);
				if (v == 0)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError =
						FString::Printf(TEXT("For component %d: Integer division by zero is undefined."), i);
					bTerminate = true;
					return;
				}
			}
		}

		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::UDiv));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(Operand1Value);
		Datas.Append(Operand2Value);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpUDiv", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpSDiv* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		const TArray<uint8>& Operand1Value = GetObjectValue(Operand1);
		const TArray<uint8>& Operand2Value = GetObjectValue(Operand2);
		
		if (EnableUbsan)
		{
			int ElemCount = 1;
			if (Operand2->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(Operand2->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				int32 v = *reinterpret_cast<const int32*>(Operand2Value.GetData() + i * 4);
				if (v == 0)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("For component %d: Integer division by zero is undefined."), i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::SDiv));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(Operand1Value);
		Datas.Append(Operand2Value);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpSDiv", GetTypeByteSize(ResultType), Datas, ExtraArgs);
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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

	void SpvVmVisitor::Visit(SpvOpUMod* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		const TArray<uint8>& Operand1Value = GetObjectValue(Operand1);
		const TArray<uint8>& Operand2Value = GetObjectValue(Operand2);
		
		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (Operand2->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(Operand2->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				uint32 v2 = *reinterpret_cast<const uint32*>(Operand2Value.GetData() + i * 4);
				if (v2 == 0)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("For component %d: The operation of an unsigned integer remainder by zero is undefined."), i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::UMod));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(Operand1Value);
		Datas.Append(Operand2Value);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpUMod", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpSRem* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		const TArray<uint8>& Operand1Value = GetObjectValue(Operand1);
		const TArray<uint8>& Operand2Value = GetObjectValue(Operand2);
		
		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (Operand2->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(Operand2->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				int32 v1 = *reinterpret_cast<const int32*>(Operand1Value.GetData() + i * 4);
				int32 v2 = *reinterpret_cast<const int32*>(Operand2Value.GetData() + i * 4);
				if (v2 == 0)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("For component %d: The operation of a signed integer remainder by zero is undefined."), i);
					bTerminate = true;
					return;
				}
				else if(v2 == -1 && v1 == std::numeric_limits<int32>::min())
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("For component %d: The remainder operation of causing signed overflow is undefined."), i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::SRem));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(Operand1Value);
		Datas.Append(Operand2Value);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpSRem", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpFRem* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		const TArray<uint8>& Operand1Value = GetObjectValue(Operand1);
		const TArray<uint8>& Operand2Value = GetObjectValue(Operand2);
		
		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (Operand2->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(Operand2->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				float v2 = *reinterpret_cast<const float*>(Operand2Value.GetData() + i * 4);
				if (v2 == 0)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError = FString::Printf(TEXT("For component %d: The operation of a floating-point remainder by zero is undefined."), i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::FRem));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(Operand1Value);
		Datas.Append(Operand2Value);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpFRem", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpBitwiseOr* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::BitwiseOr));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpBitwiseOr", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpBitwiseXor* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::BitwiseXor));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpBitwiseXor", GetTypeByteSize(ResultType), Datas, ExtraArgs);
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
		
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
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

	void SpvVmVisitor::Visit(SpvOpNot* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Not));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand = GetObject(&Context, Inst->GetOperand());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpNot", GetTypeByteSize(ResultType), Datas, ExtraArgs);
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
		SpvObject* Condition = GetObject(&Context, Inst->GetCondition());
		uint32 ConditionValue = *(uint32*)GetObjectValue(Condition).GetData();
		if(ConditionValue == 1)
		{
			ThreadState.NextInstIndex = GetInstIndex(Inst->GetTrueLabel());
		}
		else
		{
			ThreadState.NextInstIndex = GetInstIndex(Inst->GetFalseLabel());
		}
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
	}

	void SpvVmVisitor::Visit(SpvOpSwitch* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvObject* Selector = GetObject(&Context, Inst->GetSelector());
		const TArray<uint8>& SelectorValue = GetObjectValue(Selector);
		ThreadState.RecordedInfo.DebugStates.Last().bCondition = true;
		for(const auto& [Value, Label] : Inst->GetTargets())
		{
			if(SelectorValue == Value)
			{
				ThreadState.NextInstIndex = GetInstIndex(Label);
				return;
			}
		}
		
		ThreadState.NextInstIndex = GetInstIndex(Inst->GetDefault());
	}

	void SpvVmVisitor::Visit(SpvOpReturn* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		ThreadState.NextInstIndex = ThreadState.StackFrames.back().ReturnPointIndex;
		SpvLexicalScope* OldScope = ThreadState.StackFrames.back().CurScope;

		FString UbError;
		for (int i = 0; i < CurStackFrame.Arguments.size(); i++)
		{
			SpvPointer* Argument = CurStackFrame.Arguments[i];
			const SpvVmParameter& Parameter = CurStackFrame.Parameters[i];
			if(Parameter.Flag == ParamSemaFlag::Out && !Parameter.Pointer->Pointee->IsCompletelyInitialized())
			{
				FString ParameterName = Context.Names[Parameter.Pointer->Pointee->Id];
				UbError = FString::Printf(TEXT("It's undefined behavior to not explicitly initialize the \"out\" parameter:%s inside the function."), *ParameterName);
				break;
			}

			SpvId VarId = Argument->Pointee->Id;
			SpvVariableDesc* PointeeDesc = Context.VariableDescMap[VarId];
			const TArray<uint8>& ValueToStore = GetPointerValue(&Context, Parameter.Pointer);
			SpvDebugState& CurDebugState = ThreadState.RecordedInfo.DebugStates.Last();

			SpvVariableChange VariableChange;
			VariableChange.VarId = VarId;
			WritePointerValue(Argument, PointeeDesc, ValueToStore, &VariableChange);

			CurDebugState.VarChanges.Add(MoveTemp(VariableChange));
		} 
		
		ThreadState.StackFrames.pop_back();
		if(!ThreadState.StackFrames.empty())
		{
			SpvVmFrame& CallerStackFrame = ThreadState.StackFrames.back();
			SpvLexicalScope* NewScope = CallerStackFrame.CurScope;
			ThreadState.RecordedInfo.DebugStates.Last().ScopeChange = SpvLexicalScopeChange{OldScope, NewScope};
			ThreadState.RecordedInfo.DebugStates.Last().bReturn = true;
			if (!UbError.IsEmpty())
			{
				ThreadState.RecordedInfo.DebugStates.Last().UbError = MoveTemp(UbError);
				bTerminate = true;
				return;
			}
			
			//Append a debugstate for stopping at the function call after return
			ThreadState.RecordedInfo.DebugStates.Add(SpvDebugState{.Line = CallerStackFrame.CurLine, .bFuncCallAfterReturn = true});
		}
	}

	void SpvVmVisitor::Visit(SpvOpReturnValue* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		CurStackFrame.ReturnObject->Storage = GetObject(&Context, Inst->GetValue())->Storage;
		ThreadState.NextInstIndex = CurStackFrame.ReturnPointIndex;
		SpvLexicalScope* OldScope = CurStackFrame.CurScope;
		SpvObject ReturnObject = *CurStackFrame.ReturnObject;

		FString UbError;
		for (int i = 0; i < CurStackFrame.Arguments.size(); i++)
		{
			SpvPointer* Argument = CurStackFrame.Arguments[i];
			const SpvVmParameter& Parameter = CurStackFrame.Parameters[i];
			if (Parameter.Flag == ParamSemaFlag::Out && !Parameter.Pointer->Pointee->IsCompletelyInitialized())
			{
				FString ParameterName = Context.Names[Parameter.Pointer->Pointee->Id];
				UbError = FString::Printf(TEXT("It's undefined behavior to not explicitly initialize the \"out\" parameter:%s inside the function."), *ParameterName);
				break;
			}

			SpvId VarId = Argument->Pointee->Id;
			SpvVariableDesc* PointeeDesc = Context.VariableDescMap[VarId];
			const TArray<uint8>& ValueToStore = GetPointerValue(&Context, Parameter.Pointer);
			SpvDebugState& CurDebugState = ThreadState.RecordedInfo.DebugStates.Last();

			SpvVariableChange VariableChange;
			VariableChange.VarId = VarId;
			WritePointerValue(Argument, PointeeDesc, ValueToStore, &VariableChange);

			CurDebugState.VarChanges.Add(MoveTemp(VariableChange));
		}
		
		ThreadState.StackFrames.pop_back();
		if (!ThreadState.StackFrames.empty())
		{
			SpvVmFrame& CallerStackFrame = ThreadState.StackFrames.back();
			SpvLexicalScope* NewScope = CallerStackFrame.CurScope;
			ThreadState.RecordedInfo.DebugStates.Last().ReturnObject = MoveTemp(ReturnObject);
			ThreadState.RecordedInfo.DebugStates.Last().bReturn = true;
			// Split a debugstate for stopping at ending line of the function lexcical scope
			// we can't get the ending line, so the line number will be determined by the editor.
			ShaderFunc* Func = Context.EditorFuncInfo.FindByPredicate([&](auto&& Item) { 
				SpvFunctionDesc* FuncDesc = GetFunctionDesc(OldScope);
				return Item.Name == FuncDesc->GetName() && Item.Start.X == FuncDesc->GetLine();
			});
			ThreadState.RecordedInfo.DebugStates.Add(SpvDebugState{
				.Line = Func->End.X,
				.ScopeChange = SpvLexicalScopeChange{OldScope, NewScope}, 
				.bReturn = true 
			});
			if (!UbError.IsEmpty())
			{
				ThreadState.RecordedInfo.DebugStates.Last().UbError = MoveTemp(UbError);
				bTerminate = true;
				return;
			}

			ThreadState.RecordedInfo.DebugStates.Add(SpvDebugState{ .Line = CallerStackFrame.CurLine, .bFuncCallAfterReturn = true });
		}
	}

	void SpvVmVisitor::Visit(SpvOpVectorTimesScalar* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetVector());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetScalar());
		
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

	void SpvVmVisitor::Visit(SpvOpMatrixTimesScalar* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetMatrix());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetScalar());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::MatrixTimesScalar));
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
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpMatrixTimesScalar", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpVectorTimesMatrix* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetVector());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetMatrix());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::VectorTimesMatrix));
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
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpVectorTimesMatrix", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpMatrixTimesVector* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetMatrix());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetVector());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::MatrixTimesVector));
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
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpMatrixTimesVector", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpMatrixTimesMatrix* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetLeftMatrix());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetRightMatrix());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::MatrixTimesMatrix));
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
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpMatrixTimesMatrix", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpDot* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Vector1 = GetObject(&Context, Inst->GetVector1());
		SpvObject* Vector2 = GetObject(&Context, Inst->GetVector2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Dot));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType1=%s"), *GetHlslTypeStr(Vector1->Type)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType2=%s"), *GetHlslTypeStr(Vector2->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Vector1));
		Datas.Append(GetObjectValue(Vector2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpDot", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpAny* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Vector = GetObject(&Context, Inst->GetVector());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Any));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Vector->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Vector));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpAny", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpAll* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Vector = GetObject(&Context, Inst->GetVector());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::All));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Vector->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Vector));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpAll", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpIsNan* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* X = GetObject(&Context, Inst->GetX());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::IsNan));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(X->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpIsNan", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpIsInf* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* X = GetObject(&Context, Inst->GetX());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::IsInf));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(X->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpIsInf", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpLogicalOr* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::LogicalOr));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpLogicalOr", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpLogicalAnd* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::LogicalAnd));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand1));
		Datas.Append(GetObjectValue(Operand2));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpLogicalAnd", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvId ResultId = Inst->GetId().value();
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpLogicalNot* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::LogicalNot));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Operand = GetObject(&Context, Inst->GetOperand());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Operand));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpLogicalNot", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpSelect* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Condition = GetObject(&Context, Inst->GetCondition());
		SpvObject* Object1 = GetObject(&Context, Inst->GetObject1());
		SpvObject* Object2 = GetObject(&Context, Inst->GetObject2());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Select));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType1=%s"), *GetHlslTypeStr(Condition->Type)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType2=%s"), *GetHlslTypeStr(Object1->Type)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType3=%s"), *GetHlslTypeStr(Object2->Type)));
		
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

#define IMPL_COMPARISON(Name)		                                                                              \
	void SpvVmVisitor::Visit(SpvOp##Name* Inst)                                                                   \
	{                                                                                                             \
		SpvVmContext& Context = GetActiveContext();                                                               \
		SpvThreadState& ThreadState = Context.ThreadState;                                                        \
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();                                               \
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();                                         \
		SpvObject* Operand1 = GetObject(&Context, Inst->GetOperand1());                                           \
		SpvObject* Operand2 = GetObject(&Context, Inst->GetOperand2());                                           \
		                                                                                                          \
		TArray<FString> ExtraArgs;                                                                                \
		ExtraArgs.Add("-D");                                                                                      \
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::Name));                                      \
		ExtraArgs.Add("-D");                                                                                      \
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));                     \
		ExtraArgs.Add("-D");                                                                                      \
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType1=%s"), *GetHlslTypeStr(Operand1->Type)));               \
		ExtraArgs.Add("-D");                                                                                      \
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType2=%s"), *GetHlslTypeStr(Operand2->Type)));               \
                                                                                                                  \
		TArray<uint8> Datas;                                                                                      \
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));                                                          \
		Datas.Append(GetObjectValue(Operand1));                                                                   \
		Datas.Append(GetObjectValue(Operand2));                                                                   \
		                                                                                                          \
		TArray<uint8> ResultValue = ExecuteGpuOp("Op##Name", GetTypeByteSize(ResultType), Datas, ExtraArgs);      \
		SpvId ResultId = Inst->GetId().value();                                                                   \
		SpvObject ResultObject{                                                                                   \
			.Id = ResultId,                                                                                       \
			.Type = ResultType,                                                                                   \
			.Storage = SpvObject::Internal(ResultValue)                                                           \
		};                                                                                                        \
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));                     \
	}

IMPL_COMPARISON(IEqual)
IMPL_COMPARISON(INotEqual)
IMPL_COMPARISON(UGreaterThan)
IMPL_COMPARISON(SGreaterThan)
IMPL_COMPARISON(UGreaterThanEqual)
IMPL_COMPARISON(SGreaterThanEqual)
IMPL_COMPARISON(ULessThan)
IMPL_COMPARISON(SLessThan)
IMPL_COMPARISON(ULessThanEqual)
IMPL_COMPARISON(SLessThanEqual)
IMPL_COMPARISON(FOrdEqual)
IMPL_COMPARISON(FOrdNotEqual)
IMPL_COMPARISON(FOrdLessThan)
IMPL_COMPARISON(FOrdGreaterThan)
IMPL_COMPARISON(FOrdLessThanEqual)
IMPL_COMPARISON(FOrdGreaterThanEqual)

	void SpvVmVisitor::Visit(SpvOpShiftRightLogical* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Base = GetObject(&Context, Inst->GetBase());
		SpvObject* Shift = GetObject(&Context, Inst->GetShift());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ShiftRightLogical));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Base));
		Datas.Append(GetObjectValue(Shift));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpShiftRightLogical", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpShiftRightArithmetic* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Base = GetObject(&Context, Inst->GetBase());
		SpvObject* Shift = GetObject(&Context, Inst->GetShift());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ShiftRightArithmetic));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Base));
		Datas.Append(GetObjectValue(Shift));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpShiftRightArithmetic", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpShiftLeftLogical* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* Base = GetObject(&Context, Inst->GetBase());
		SpvObject* Shift = GetObject(&Context, Inst->GetShift());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ShiftLeftLogical));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Base));
		Datas.Append(GetObjectValue(Shift));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpShiftLeftLogical", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvRoundEven* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::RoundEven));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("RoundEven", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvTrunc* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Trunc));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Trunc", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvFAbs* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::FAbs));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("FAbs", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvSAbs* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::SAbs));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("SAbs", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvFSign* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::FSign));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("FSign", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvSSign* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::SSign));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("SSign", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvFloor* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Floor));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Floor", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvCeil* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Ceil));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Ceil", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvFract* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Fract));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Fract", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
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
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
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
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
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

	void SpvVmVisitor::Visit(SpvTan* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Tan));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Tan", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvAsin* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Asin));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Asin", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvAcos* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Acos));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Acos", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvAtan* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Atan));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Atan", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvPow* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Pow));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y= GetObject(&Context, Inst->GetY());
		const TArray<uint8>& XValue = GetObjectValue(X);
		const TArray<uint8>& YValue = GetObjectValue(Y);

		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (X->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(X->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				float x = *reinterpret_cast<const float*>(XValue.GetData() + i * 4);
				float y = *reinterpret_cast<const float*>(YValue.GetData() + i * 4);
				if (x < 0.0f)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError =
						FString::Printf(TEXT("pow: x(%f) < 0 for component %d, the result is undefined."), x, i);
					bTerminate = true;
					return;
				}
				if (x == 0.0f && y <= 0.0f)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError =
						FString::Printf(TEXT("pow: x(%f) == 0 and y(%f) <= 0 for component %d, the result is undefined."), x, y, i);
					bTerminate = true;
					return;
				}
			}
		}

		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(XValue);
		Datas.Append(YValue);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Pow", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvExp* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Exp));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Exp", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvLog* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Log));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Log", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvExp2* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Exp2));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Exp2", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvLog2* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Log2));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Log2", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvSqrt* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Sqrt));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Sqrt", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvInverseSqrt* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::InverseSqrt));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("InverseSqrt", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvDeterminant* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* X = GetObject(&Context, Inst->GetX());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Determinant));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(X->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Determinant", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvUMin* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::UMin));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("UMin", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvSMin* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::SMin));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("SMin", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvUMax* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::UMax));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("UMax", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvSMax* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::SMax));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("SMax", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvFClamp* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* MinVal = GetObject(&Context, Inst->GetMinVal());
		SpvObject* MaxVal = GetObject(&Context, Inst->GetMaxVal());
		const TArray<uint8>& XValue = GetObjectValue(X);
		const TArray<uint8>& MinValue = GetObjectValue(MinVal);
		const TArray<uint8>& MaxValue = GetObjectValue(MaxVal);
		
		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (MinVal->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(MinVal->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				float Min = *reinterpret_cast<const float*>(MinValue.GetData() + i * 4);
				float Max = *reinterpret_cast<const float*>(MaxValue.GetData() + i * 4);
				if (Min > Max)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError =
						FString::Printf(TEXT("clamp: minVal(%f) > maxVal(%f) for component %d, the result is undefined."), Min, Max, i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::FClamp));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(XValue);
		Datas.Append(MinValue);
		Datas.Append(MaxValue);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("FClamp", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvUClamp* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* MinVal = GetObject(&Context, Inst->GetMinVal());
		SpvObject* MaxVal = GetObject(&Context, Inst->GetMaxVal());
		const TArray<uint8>& XValue = GetObjectValue(X);
		const TArray<uint8>& MinValue = GetObjectValue(MinVal);
		const TArray<uint8>& MaxValue = GetObjectValue(MaxVal);
		
		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (MinVal->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(MinVal->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				uint32 Min = *reinterpret_cast<const uint32*>(MinValue.GetData() + i * 4);
				uint32 Max = *reinterpret_cast<const uint32*>(MaxValue.GetData() + i * 4);
				if (Min > Max)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError =
						FString::Printf(TEXT("clamp: minVal(%d) > maxVal(%d) for component %d, the result is undefined."), Min, Max, i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::UClamp));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(XValue);
		Datas.Append(MinValue);
		Datas.Append(MaxValue);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("UClamp", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvSClamp* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* MinVal = GetObject(&Context, Inst->GetMinVal());
		SpvObject* MaxVal = GetObject(&Context, Inst->GetMaxVal());
		const TArray<uint8>& XValue = GetObjectValue(X);
		const TArray<uint8>& MinValue = GetObjectValue(MinVal);
		const TArray<uint8>& MaxValue = GetObjectValue(MaxVal);
		
		if(EnableUbsan)
		{
			int ElemCount = 1;
			if (MinVal->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(MinVal->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				int32 Min = *reinterpret_cast<const int32*>(MinValue.GetData() + i * 4);
				int32 Max = *reinterpret_cast<const int32*>(MaxValue.GetData() + i * 4);
				if (Min > Max)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError =
						FString::Printf(TEXT("clamp: minVal(%d) > maxVal(%d) for component %d, the result is undefined."), Min, Max, i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::SClamp));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(XValue);
		Datas.Append(MinValue);
		Datas.Append(MaxValue);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("SClamp", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvFMix* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::FMix));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		SpvObject* A = GetObject(&Context, Inst->GetA());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		Datas.Append(GetObjectValue(A));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("FMix", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvStep* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Step));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Edge = GetObject(&Context, Inst->GetEdge());
		SpvObject* X = GetObject(&Context, Inst->GetX());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(Edge));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Step", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvSmoothStep* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::SmoothStep));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* Edge0 = GetObject(&Context, Inst->GetEdge0());
		SpvObject* Edge1 = GetObject(&Context, Inst->GetEdge1());
		SpvObject* X = GetObject(&Context, Inst->GetX());
		const TArray<uint8>& Edge0Value = GetObjectValue(Edge0);
		const TArray<uint8>& Edge1Value = GetObjectValue(Edge1);
		const TArray<uint8>& XValue = GetObjectValue(X);

		if (EnableUbsan)
		{
			int ElemCount = 1;
			if (Edge0->Type->GetKind() == SpvTypeKind::Vector)
			{
				ElemCount = static_cast<SpvVectorType*>(Edge0->Type)->ElementCount;
			}
			for (int i = 0; i < ElemCount; ++i)
			{
				float e0 = *reinterpret_cast<const float*>(Edge0Value.GetData() + i * 4);
				float e1 = *reinterpret_cast<const float*>(Edge1Value.GetData() + i * 4);
				if (e0 >= e1)
				{
					ThreadState.RecordedInfo.DebugStates.Last().UbError =
						FString::Printf(TEXT("smoothstep: Edge0(%f) >= Edge1(%f) for component %d, the result is undefined."), e0, e1, i);
					bTerminate = true;
					return;
				}
			}
		}
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(Edge0Value);
		Datas.Append(Edge1Value);
		Datas.Append(XValue);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("SmoothStep", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvPackHalf2x16* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* V = GetObject(&Context, Inst->GetV());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::PackHalf2x16));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(V->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(V));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("PackHalf2x16", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvUnpackHalf2x16* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* V = GetObject(&Context, Inst->GetV());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::UnpackHalf2x16));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(V->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(V));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("UnpackHalf2x16", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvLength* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* X = GetObject(&Context, Inst->GetX());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Length));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(X->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Length", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvDistance* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* P0 = GetObject(&Context, Inst->GetP0());
		SpvObject* P1 = GetObject(&Context, Inst->GetP1());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Distance));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType1=%s"), *GetHlslTypeStr(P0->Type)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType2=%s"), *GetHlslTypeStr(P1->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(P0));
		Datas.Append(GetObjectValue(P1));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Distance", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvCross* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Cross));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Cross", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvNormalize* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* X = GetObject(&Context, Inst->GetX());
		const TArray<uint8>& XValue = GetObjectValue(X);
		
		if(EnableUbsan)
		{
			TArray<uint8> ZeroArr;
			ZeroArr.SetNumZeroed(XValue.Num());
			if (FMemory::Memcmp(XValue.GetData(), ZeroArr.GetData(), XValue.Num()) == 0)
			{
			   ThreadState.RecordedInfo.DebugStates.Last().UbError =
				   TEXT("normalize: normalizing a zero scalar or vector is undefined.");
			   bTerminate = true;
			   return;
			}
		}
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Normalize));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(XValue);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Normalize", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvReflect* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Reflect));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* I = GetObject(&Context, Inst->GetI());
		SpvObject* N = GetObject(&Context, Inst->GetN());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(I));
		Datas.Append(GetObjectValue(N));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Reflect", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvRefract* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvObject* I = GetObject(&Context, Inst->GetI());
		SpvObject* N = GetObject(&Context, Inst->GetN());
		SpvObject* Eta = GetObject(&Context, Inst->GetEta());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::Refract));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType1=%s"), *GetHlslTypeStr(I->Type)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType2=%s"), *GetHlslTypeStr(N->Type)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType3=%s"), *GetHlslTypeStr(Eta->Type)));
		
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(I));
		Datas.Append(GetObjectValue(N));
		Datas.Append(GetObjectValue(Eta));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("Refract", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvNMin* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::NMin));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("NMin", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvNMax* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvGLSLstd450::NMax));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		SpvObject* X = GetObject(&Context, Inst->GetX());
		SpvObject* Y = GetObject(&Context, Inst->GetY());
		TArray<uint8> Datas;
		Datas.SetNumZeroed(GetTypeByteSize(ResultType));
		Datas.Append(GetObjectValue(X));
		Datas.Append(GetObjectValue(Y));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("NMax", GetTypeByteSize(ResultType), Datas, ExtraArgs);
		SpvObject ResultObject{
			.Id = Inst->GetId().value(),
			.Type = ResultType,
			.Storage = SpvObject::Internal(ResultValue)
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(Inst->GetId().value(), MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpSampledImage* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		
		SpvObject* Image = GetObject(&Context, Inst->GetImage());
		SpvObject* Sampler = GetObject(&Context, Inst->GetSampler());
		TArray<GpuResource*> Resources;
		Resources.Append(std::get<SpvObject::Internal>(Image->Storage).Resources);
		Resources.Append(std::get<SpvObject::Internal>(Sampler->Storage).Resources);
		
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{.Resources = Resources}
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpImageFetch* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		SpvObject* Image = GetObject(&Context, Inst->GetImage());
		SpvObject* Coordinate = GetObject(&Context, Inst->GetCoordinate());
		std::optional<SpvImageOperands> ImageOperands = Inst->GetImageOperands();
		const TArray<SpvId>& Operands = Inst->GetOperands();
		
		int32 ResultTypeSize = GetTypeByteSize(ResultType);
		TArray<uint8> Datas;
		Datas.SetNumZeroed(ResultTypeSize);
		Datas.Append(GetObjectValue(Coordinate));
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ImageFetch));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType1=%s"), *GetHlslTypeStr(Coordinate->Type)));
		if(ImageOperands)
		{
			ExtraArgs.Add("-D");
			ExtraArgs.Add(FString::Printf(TEXT("IMAGE_OPERAND=%d"), (int)ImageOperands.value()));
			if(EnumHasAnyFlags(ImageOperands.value(),SpvImageOperands::Lod))
			{
				SpvObject* Lod = GetObject(&Context, Operands[0]);
				Datas.Append(GetObjectValue(Lod));
				ExtraArgs.Add("-D");
				ExtraArgs.Add(FString::Printf(TEXT("OpOperandType2=%s"), *GetHlslTypeStr(Lod->Type)));
			}
		}
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpImageFetch", ResultTypeSize, Datas, ExtraArgs, std::get<SpvObject::Internal>(Image->Storage).Resources);
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ MoveTemp(ResultValue) }
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpImageQuerySizeLod* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		SpvObject* Image = GetObject(&Context, Inst->GetImage());
		SpvObject* Lod = GetObject(&Context, Inst->GetLod());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ImageQuerySizeLod));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpOperandType=%s"), *GetHlslTypeStr(Lod->Type)));
		
		int32 ResultTypeSize = GetTypeByteSize(ResultType);
		TArray<uint8> Datas;
		Datas.SetNumZeroed(ResultTypeSize);
		Datas.Append(GetObjectValue(Lod));
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpImageQuerySizeLod", ResultTypeSize, Datas, ExtraArgs, std::get<SpvObject::Internal>(Image->Storage).Resources);
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ MoveTemp(ResultValue) }
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
	}

	void SpvVmVisitor::Visit(SpvOpImageQueryLevels* Inst)
	{
		SpvVmContext& Context = GetActiveContext();
		SpvThreadState& ThreadState = Context.ThreadState;
		SpvVmFrame& CurStackFrame = ThreadState.StackFrames.back();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		SpvId ResultId = Inst->GetId().value();
		SpvObject* Image = GetObject(&Context, Inst->GetImage());
		
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpCode=%d"), (int)SpvOp::ImageQueryLevels));
		ExtraArgs.Add("-D");
		ExtraArgs.Add(FString::Printf(TEXT("OpResultType=%s"), *GetHlslTypeStr(ResultType)));
		
		int32 ResultTypeSize = GetTypeByteSize(ResultType);
		TArray<uint8> Datas;
		Datas.SetNumZeroed(ResultTypeSize);
		
		TArray<uint8> ResultValue = ExecuteGpuOp("OpImageQueryLevels", ResultTypeSize, Datas, ExtraArgs, std::get<SpvObject::Internal>(Image->Storage).Resources);
		SpvObject ResultObject{
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{ MoveTemp(ResultValue) }
		};
		CurStackFrame.IntermediateObjects.insert_or_assign(ResultId, MoveTemp(ResultObject));
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

	SpvObject* GetObject(SpvVmContext* InContext, SpvId ObjectId)
	{
		SpvVmFrame& StackFrame = InContext->ThreadState.StackFrames.back();
		if(InContext->Constants.contains(ObjectId))
		{
			return &InContext->Constants.at(ObjectId);
		}
		else
		{
			return &StackFrame.IntermediateObjects.at(ObjectId);
		}
	}

	TArray<uint8> SpvVmVisitor::ExecuteGpuOp(const FString& Name, int32 ResultSize, const TArray<uint8>& InputData, const TArray<FString>& Args, const TArray<GpuResource*>& InResources)
	{
		static TRefCountPtr<GpuShader> OpComputeShader = GGpuRhi->CreateShaderFromFile({
					.FileName = PathHelper::ShaderDir() / "ShaderHelper/Debugger/Op.hlsl",
					.Type = ShaderType::ComputeShader,
					.EntryPoint = "MainCS"
		});
		static TRefCountPtr<GpuShader> OpVertexShader = GGpuRhi->CreateShaderFromFile({
					.FileName = PathHelper::ShaderDir() / "ShaderHelper/Debugger/Op.hlsl",
					.Type = ShaderType::VertexShader,
					.EntryPoint = "MainVS"
		});
		static TRefCountPtr<GpuShader> OpPixelShader = GGpuRhi->CreateShaderFromFile({
					.FileName = PathHelper::ShaderDir() / "ShaderHelper/Debugger/Op.hlsl",
					.Type = ShaderType::PixelShader,
					.EntryPoint = "MainPS"
		});
		
		TRefCountPtr<GpuBuffer> Input = GGpuRhi->CreateBuffer({
			.ByteSize = (uint32)InputData.Num(),
			.Usage = GpuBufferUsage::RWStorage,
			.Stride = (uint32)InputData.Num(),
			.InitialData = InputData
		});
		
		TRefCountPtr<GpuBindGroupLayout> OpBindGroupLayout;
		TRefCountPtr<GpuBindGroup> OpBindGroup;
		if(Name == "OpImageSampleImplicitLod")
		{
			OpBindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
				.AddExistingBinding(0, BindingType::RWStorageBuffer)
				.AddExistingBinding(1, BindingType::Texture)
				.AddExistingBinding(2, BindingType::Sampler)
				.Build();
			OpBindGroup = GpuBindGroupBuilder{ OpBindGroupLayout }
				.SetExistingBinding(0, Input)
				.SetExistingBinding(1, InResources[0])
				.SetExistingBinding(2, InResources[1])
				.Build();
		}
		else if(Name == "OpImageFetch" || Name == "OpImageQueryLevels" || Name == "OpImageQuerySizeLod")
		{
			OpBindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
				.AddExistingBinding(0, BindingType::RWStorageBuffer)
				.AddExistingBinding(1, BindingType::Texture)
				.Build();
			OpBindGroup = GpuBindGroupBuilder{ OpBindGroupLayout }
				.SetExistingBinding(0, Input)
				.SetExistingBinding(1, InResources[0])
				.Build();
		}
		else
		{
			OpBindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
				.AddExistingBinding(0, BindingType::RWStorageBuffer)
				.Build();
			OpBindGroup = GpuBindGroupBuilder{ OpBindGroupLayout }
				.SetExistingBinding(0, Input)
				.Build();
		}
		
		auto CmdRecorder = GGpuRhi->BeginRecording();
		
		if(Name == "DPdx" || Name == "DPdy" || Name == "OpFwidth" || Name == "OpImageSampleImplicitLod")
		{
			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(OpVertexShader, ErrorInfo, WarnInfo, Args);
			check(ErrorInfo.IsEmpty());
			
			GGpuRhi->CompileShader(OpPixelShader, ErrorInfo, WarnInfo, Args);
			check(ErrorInfo.IsEmpty());
			
			TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState({
				.Vs = OpVertexShader,
				.Ps = OpPixelShader,
				.BindGroupLayout0 = OpBindGroupLayout
			});
			
			auto PassRecorder = CmdRecorder->BeginRenderPass({}, Name);
			{
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->SetBindGroups(OpBindGroup, nullptr, nullptr, nullptr);
				PassRecorder->SetViewPort({ .Width = 2, .Height = 2});
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		else
		{
			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(OpComputeShader, ErrorInfo, WarnInfo, Args);
			check(ErrorInfo.IsEmpty());
			
			TRefCountPtr<GpuComputePipelineState> Pipeline = GGpuRhi->CreateComputePipelineState({
				.Cs = OpComputeShader,
				.BindGroupLayout0 = OpBindGroupLayout
			});
			
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
