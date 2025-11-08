#include "CommonHeader.h"
#include "SpirvDebugger.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32>& Indexes, int32* OutOffset)
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
				for (uint32 MemberIndex = 0; MemberIndex < Index; MemberIndex++)
				{
					OffsetBytes += GetTypeByteSize(StructType->MemberTypes[MemberIndex]);
				}
				SpvType* MemberType = StructType->MemberTypes[Index];
				ByteSize = GetTypeByteSize(MemberType);
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

		if (OutOffset)
		{
			*OutOffset = OffsetBytes;
		}
		
		return {ObjectValue.GetData() + OffsetBytes, ByteSize};
	}

	TArray<uint8> GetPointerValue(SpvDebuggerContext* InContext, SpvPointer* InPointer)
	{
		SpvVariableDesc* PointeeDesc = InContext->VariableDescMap[InPointer->Pointee->Id];
		if (InPointer->Pointee->InitializedRanges.IsEmpty())
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

		if (!InPointer->Pointee->IsExternal())
		{
			ByteSize = GetObjectValue(InPointer->Pointee, InPointer->Indexes, &OffsetBytes).Num();
			bool ValueInitialized = false;
			for (const auto& Range : InPointer->Pointee->InitializedRanges)
			{
				if (OffsetBytes >= Range.X && OffsetBytes + ByteSize <= Range.Y)
				{
					ValueInitialized = true;
					break;
				}
			}
			if (!ValueInitialized)
			{
				return {};
			}
		}
		else
		{
			//External objects like uniform buffers may have different alignments and sizes,
			//so can't extract them directly from OpType* but rely on SpvVariableDesc to do it
			if (PointeeDesc && PointeeDesc->TypeDesc)
			{
				SpvTypeDesc* CurTypeDesc = PointeeDesc->TypeDesc;
				for (int32 Layer = 0; Layer < InPointer->Indexes.Num(); Layer++)
				{
					int32 Index = InPointer->Indexes[Layer];
					if (CurTypeDesc->GetKind() == SpvTypeDescKind::Composite)
					{
						SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(CurTypeDesc);
						SpvTypeDesc* IndexTypeDesc = CompositeTypeDesc->GetMemberTypeDescs()[Index];
						if (IndexTypeDesc->GetKind() == SpvTypeDescKind::Member)
						{
							SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(IndexTypeDesc);
							OffsetBytes += MemberTypeDesc->GetOffset() / 8;
							ByteSize = MemberTypeDesc->GetSize() / 8;
							CurTypeDesc = MemberTypeDesc->GetTypeDesc();
						}
					}
					else if (CurTypeDesc->GetKind() == SpvTypeDescKind::Vector)
					{
						SpvVectorTypeDesc* VectorTypeDesc = static_cast<SpvVectorTypeDesc*>(CurTypeDesc);
						SpvBasicTypeDesc* BasicTypeDesc = VectorTypeDesc->GetBasicTypeDesc();
						OffsetBytes += Index * BasicTypeDesc->GetSize() / 8;
						ByteSize = BasicTypeDesc->GetSize() / 8;
					}
					else if (CurTypeDesc->GetKind() == SpvTypeDescKind::Matrix)
					{
						SpvMatrixTypeDesc* MatrixTypeDesc = static_cast<SpvMatrixTypeDesc*>(CurTypeDesc);
						SpvVectorTypeDesc* ElementTypeDesc = MatrixTypeDesc->GetVectorTypeDesc();
						int32 ElementTypeSize = GetTypeByteSize(ElementTypeDesc);
						OffsetBytes += Index * ElementTypeSize;
						ByteSize = ElementTypeSize;
						CurTypeDesc = ElementTypeDesc;
					}
					else if (CurTypeDesc->GetKind() == SpvTypeDescKind::Array)
					{
						SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(CurTypeDesc);
						int32 BaseTypeSize = GetTypeByteSize(ArrayTypeDesc->GetBaseTypeDesc());
						int32 ElementNum = 1;
						for (int32 i = Layer + 1; i < ArrayTypeDesc->GetCompCounts().Num(); i++)
						{
							ElementNum *= ArrayTypeDesc->GetCompCounts()[i];
						}
						int32 ElementTypeSize = BaseTypeSize * ElementNum;
						OffsetBytes += Index * ElementTypeSize;
						ByteSize = ElementTypeSize;
					}
				}
			}
			else
			{
				//In some cases like StructuredBuffer,
				//it may not be able to get the value correctly according to SpvVariableDesc, try getting the value from SpvType
				SpvType* CurType = InPointer->Pointee->Type;
				for (int32 Index : InPointer->Indexes)
				{
					TArray<SpvDecoration> Decorations;
					InContext->Decorations.MultiFind(CurType->GetId(), Decorations);

					if (CurType->GetKind() == SpvTypeKind::Struct)
					{
						SpvStructType* StructType = static_cast<SpvStructType*>(CurType);
						const SpvDecoration* CurMemberIndexDecoration = Decorations.FindByPredicate([Index](const SpvDecoration& Item) {
							return Item.Kind == SpvDecorationKind::Offset && Item.Offset.MemberIndex == Index;
							});
						const SpvDecoration* NextMemberIndexDecoration = Decorations.FindByPredicate([Index](const SpvDecoration& Item) {
							return Item.Kind == SpvDecorationKind::Offset && Item.Offset.MemberIndex == Index + 1;
							});
						if (Index == StructType->MemberTypes.Num() - 1)
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
					else if (CurType->GetKind() == SpvTypeKind::RuntimeArray)
					{
						SpvRuntimeArrayType* RuntimeArrayType = static_cast<SpvRuntimeArrayType*>(CurType);
						for (const auto& Decoration : Decorations)
						{
							if (Decoration.Kind == SpvDecorationKind::ArrayStride)
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

	void SpvDebuggerVisitor::Visit(const SpvDebugDeclare* Inst)
	{
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		Context.VariableDescMap[Inst->GetVariable()] = VarDesc;
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugValue* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvDebugLine* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvDebugScope* Inst)
	{
		Context.Scope = Context.LexicalScopes[Inst->GetScope()].Get();
		Context.Line = Context.Scope->GetLine();
		int EndOffset = Inst->GetWordOffset().value() + Inst->GetWordLen().value();
		static const int t = [&] {
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>( VoidType, AppendScopeFuncId);
			FuncCallOp->SetId(Patcher.NewId());
			Patcher.AddInstruction(EndOffset, MoveTemp(FuncCallOp));
			return 1; 
		}();
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunctionCall* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpVariable* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{ResultId, PointerType->PointeeType}, StorageClass};

		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
		Var.Storage = SpvObject::Internal{MoveTemp(Value)};
		
		Context.LocalVariables[ResultId] =  Var;
	}

	void SpvDebuggerVisitor::Visit(const SpvOpPhi* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpLabel* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpLoad* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpStore* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToU* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToS* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUDiv* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSDiv* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUMod* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSRem* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFRem* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranch* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranchConditional* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturn* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturnValue* Inst)
	{

	}

#define IMPL_COMPARISON(Name)		                                                                              \
	void SpvDebuggerVisitor::Visit(const SpvOp##Name* Inst)                                                       \
	{                                                                                                             \
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

	void SpvDebuggerVisitor::Visit(const SpvPow* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvFClamp* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvUClamp* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvSClamp* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvSmoothStep* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvNormalize* Inst)
	{
		
	}

	int32 SpvDebuggerVisitor::GetInstIndex(SpvId Inst) const
	{
		return Insts->IndexOfByPredicate([this, Inst](const TUniquePtr<SpvInstruction>& InInst){
			return InInst->GetId() ? InInst->GetId().value() == Inst : false;
		});
	}

	void SpvDebuggerVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections)
	{
		this->Insts = &Insts;
		Patcher.SetSpvContext(Insts, SpvCode, &Context);
		//Get entry point loc
		Context.InstIndex = GetInstIndex(Context.EntryPoint);

		//Init global external/location variables
		for (auto& [Id, Var] : Context.GlobalVariables)
		{
			int32 SetNumber = INDEX_NONE;
			int32 BindingNumber = INDEX_NONE;
			TArray<SpvDecoration> Decorations;
			Context.Decorations.MultiFind(Id, Decorations);
			for (const auto& Decoration : Decorations)
			{
				if (Decoration.Kind == SpvDecorationKind::DescriptorSet)
				{
					SetNumber = Decoration.DescriptorSet.Number;
				}
				else if (Decoration.Kind == SpvDecorationKind::Binding)
				{
					BindingNumber = Decoration.Binding.Number;
				}
			}
			if (SetNumber != INDEX_NONE && BindingNumber != INDEX_NONE;
				SpvBinding * Binding = Context.Bindings.FindByPredicate([&](const SpvBinding& InItem) { return InItem.Binding == BindingNumber && InItem.DescriptorSet == SetNumber; }))
			{
				auto& Storage = std::get<SpvObject::External>(Var.Storage);
				Storage.Resource = Binding->Resource;
				if (Binding->Resource->GetType() == GpuResourceType::Buffer)
				{
					GpuBuffer* Buffer = static_cast<GpuBuffer*>(Binding->Resource.GetReference());
					uint8* Data = (uint8*)GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Read_Only);
					Storage.Value = { Data, (int)Buffer->GetByteSize() };
					GGpuRhi->UnMapGpuBuffer(Buffer);
					Var.InitializedRanges.AddUnique({ 0, (int)Buffer->GetByteSize() });
				}
			}
		}

		//Patch debugger buffer
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32,0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
		SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
		SpvId RunTimeArrayType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeRuntimeArray>(UIntType));
		SpvId DebuggerBufferType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>( TArray<SpvId>{RunTimeArrayType}));
		SpvId DebuggerBufferPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, DebuggerBufferType));

		int ArrayStride = 4;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(RunTimeArrayType, SpvDecorationKind::ArrayStride, TArray<uint8>{ (uint8*)&ArrayStride, sizeof(int) }));
		int MemberOffset = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(DebuggerBufferType, 0, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset, sizeof(int) }));
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBufferType, SpvDecorationKind::BufferBlock));

		DebuggerBuffer = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>( DebuggerBufferPointerType, SpvStorageClass::Uniform);
			VarOp->SetId(DebuggerBuffer);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerBuffer, "__DebuggerBuffer"));
		}
		int SetNumber = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::DescriptorSet, TArray<uint8>{ (uint8*)&SetNumber, sizeof(int) }));
		int BindingNumber = 0721;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));

		DebuggerStateType = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerStateType);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerStateType, "__DebuggerStateType"));
		}
		DebuggerOffset = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerOffset);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerOffset, "__DebuggerOffset"));
		}
		DebuggerLine = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerLine);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerLine, "__DebuggerLine"));
		}
		DebuggerScopeId = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerScopeId);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerScopeId, "__DebuggerScopeId"));
		}
		DebuggerVarId = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerVarId);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerVarId, "__DebuggerVarId"));
		}
		DebuggerVarDirtyOffset = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerVarDirtyOffset);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerVarDirtyOffset, "__DebuggerVarDirtyOffset"));
		}
		DebuggerVarDirtyByteSize = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerVarDirtyByteSize);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerVarDirtyByteSize, "__DebuggerVarDirtyByteSize"));
		}

		//Patch AppendScope function
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		AppendScopeFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendScopeFuncId, "__AppendScope"));

		TArray<TUniquePtr<SpvInstruction>> AppendScopeFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType));
			auto FuncOp = MakeUnique<SpvOpFunction>( VoidType, SpvFunctionControl::None, FuncType );
			FuncOp->SetId(AppendScopeFuncId);
			AppendScopeFuncInsts.Add(MoveTemp(FuncOp));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendScopeFuncInsts.Add(MoveTemp(LabelOp));

			PatchActiveCondition(AppendScopeFuncInsts);

			SpvId LoadedDebuggerOffset = Patcher.NewId();
			{
				auto LoadedDebuggerOffsetOp = MakeUnique<SpvOpLoad>( UIntType, DebuggerOffset );
				LoadedDebuggerOffsetOp->SetId(LoadedDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(LoadedDebuggerOffsetOp));

				SpvId AlignedDebuggerOffset = Patcher.NewId();
				auto AlignedDebuggerOffsetOp = MakeUnique<SpvOpShiftRightLogical>( UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(2u) );
				AlignedDebuggerOffsetOp->SetId(AlignedDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(AlignedDebuggerOffsetOp));

				SpvId LoadedDebuggerStateType = Patcher.NewId();
				auto LoadedDebuggerStateTypeOp = MakeUnique<SpvOpLoad>( UIntType, DebuggerStateType );
				LoadedDebuggerStateTypeOp->SetId(LoadedDebuggerStateType);
				AppendScopeFuncInsts.Add(MoveTemp(LoadedDebuggerStateTypeOp));

				SpvId DebuggerBufferStorage = Patcher.NewId();
				auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>( UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Patcher.FindOrAddConstant(0u), AlignedDebuggerOffset} );
				DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
				AppendScopeFuncInsts.Add(MoveTemp(DebuggerBufferStorageOp));

				AppendScopeFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorage, LoadedDebuggerStateType));
			}

			{
				SpvId NewDebuggerOffset = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>( UIntType,  LoadedDebuggerOffset, Patcher.FindOrAddConstant(4u));
				AddOp->SetId(NewDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(AddOp));

				AppendScopeFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerOffset, NewDebuggerOffset));
			}

			LoadedDebuggerOffset = Patcher.NewId();
			{
				auto LoadedDebuggerOffsetOp = MakeUnique<SpvOpLoad>(UIntType, DebuggerOffset);
				LoadedDebuggerOffsetOp->SetId(LoadedDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(LoadedDebuggerOffsetOp));

				SpvId AlignedDebuggerOffset = Patcher.NewId();
				auto AlignedDebuggerOffsetOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(2u));
				AlignedDebuggerOffsetOp->SetId(AlignedDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(AlignedDebuggerOffsetOp));

				SpvId LoadedDebuggerLine = Patcher.NewId();
				auto LoadedDebuggerLineOp = MakeUnique<SpvOpLoad>( UIntType, DebuggerLine );
				LoadedDebuggerLineOp->SetId(LoadedDebuggerLine);
				AppendScopeFuncInsts.Add(MoveTemp(LoadedDebuggerLineOp));

				SpvId DebuggerBufferStorage = Patcher.NewId();
				auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Patcher.FindOrAddConstant(0u), AlignedDebuggerOffset});
				DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
				AppendScopeFuncInsts.Add(MoveTemp(DebuggerBufferStorageOp));

				AppendScopeFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorage, LoadedDebuggerLine));
			}

			{
				SpvId NewDebuggerOffset = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(4u));
				AddOp->SetId(NewDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(AddOp));

				AppendScopeFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerOffset, NewDebuggerOffset));
			}

			LoadedDebuggerOffset = Patcher.NewId();
			{
				auto LoadedDebuggerOffsetOp = MakeUnique<SpvOpLoad>(UIntType, DebuggerOffset);
				LoadedDebuggerOffsetOp->SetId(LoadedDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(LoadedDebuggerOffsetOp));

				SpvId AlignedDebuggerOffset = Patcher.NewId();
				auto AlignedDebuggerOffsetOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(2u));
				AlignedDebuggerOffsetOp->SetId(AlignedDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(AlignedDebuggerOffsetOp));

				SpvId LoadedDebuggerScopeId = Patcher.NewId();
				auto LoadedDebuggerScopeIdOp = MakeUnique<SpvOpLoad>(UIntType, DebuggerScopeId );
				LoadedDebuggerScopeIdOp->SetId(LoadedDebuggerScopeId);
				AppendScopeFuncInsts.Add(MoveTemp(LoadedDebuggerScopeIdOp));

				SpvId DebuggerBufferStorage = Patcher.NewId();
				auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>( UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Patcher.FindOrAddConstant(0u), AlignedDebuggerOffset} );
				DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
				AppendScopeFuncInsts.Add(MoveTemp(DebuggerBufferStorageOp));

				AppendScopeFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorage, LoadedDebuggerScopeId));
			}

			{
				SpvId NewDebuggerOffset = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(4u));
				AddOp->SetId(NewDebuggerOffset);
				AppendScopeFuncInsts.Add(MoveTemp(AddOp));

				AppendScopeFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerOffset, NewDebuggerOffset));
			}

			AppendScopeFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendScopeFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendScopeFuncInsts));

		while (Context.InstIndex < Insts.Num())
		{
			Insts[Context.InstIndex]->Accept(this);
			Context.InstIndex++;
		}

	}
}
