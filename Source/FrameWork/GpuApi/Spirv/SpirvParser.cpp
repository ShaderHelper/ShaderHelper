#include "CommonHeader.h"
#include "SpirvParser.h"

namespace FW
{

	void SpvMetaVisitor::Visit(const SpvOpVariable* Inst)
	{
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvId ResultId = Inst->GetId().value();
		
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{ResultId, PointerType->PointeeType}, StorageClass, PointerType };
		if(StorageClass == SpvStorageClass::Uniform || StorageClass == SpvStorageClass::UniformConstant)
		{
			//Cannot confirm the allocation size of external objects.
			Var.Storage = SpvObject::External{};
		}
		else
		{
			TArray<uint8> Value;
			Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
			Var.Storage = SpvObject::Internal{MoveTemp(Value)};
		}
		
		if(Inst->GetInitializer().has_value())
		{
			SpvObject* Constant = &Context.Constants[Inst->GetInitializer().value()];
			TArray<uint8>& Value = std::get<SpvObject::Internal>(Var.Storage).Value;
			Value = std::get<SpvObject::Internal>(Constant->Storage).Value;
			Var.InitializedRanges.AddUnique({0, Value.Num()});
		}
		
		Context.GlobalVariables.emplace(ResultId, MoveTemp(Var));
		SpvPointer Pointer{
			.Id = ResultId,
			.Var = &Context.GlobalVariables[ResultId],
			.Type = PointerType,
		};
		Context.GlobalPointers.emplace(ResultId, MoveTemp(Pointer));
		
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeFloat* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvFloatType>(ResultId, Inst->GetWidth()));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeInt* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvIntegerType>(ResultId, Inst->GetWidth(), !!Inst->GetSignedness()));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeVector* Inst)
	{
		SpvType* ElementType = Context.Types[Inst->GetComponentType()].Get();
		check(ElementType->IsScalar());
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvVectorType>(ResultId, static_cast<SpvScalarType*>(ElementType), Inst->GetComponentCount()));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeMatrix* Inst)
	{
		SpvType* ElementType = Context.Types[Inst->GetColumnType()].Get();
		check(ElementType->GetKind() == SpvTypeKind::Vector);
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvMatrixType>(ResultId, static_cast<SpvVectorType*>(ElementType), Inst->GetColumnCount()));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeVoid* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvVoidType>(ResultId));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeBool* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvBoolType>(ResultId));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypePointer* Inst)
	{
		SpvType* PointeeType = Context.Types[Inst->GetPointeeType()].Get();
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvPointerType>(ResultId, Inst->GetStorageClass(), PointeeType));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeFunction* Inst)
	{
		SpvType* ReturnType = Context.Types[Inst->GetReturnType()].Get();
		SpvId ResultId = Inst->GetId().value();
		TArray<SpvType*> ParameterTypes;
		for (SpvId ParameterTypeId : Inst->GetParameterTypes())
		{
			ParameterTypes.Add(Context.Types[ParameterTypeId].Get());
		}
		Context.Types.emplace(ResultId, MakeUnique<SpvFunctionType>(ResultId, ReturnType, ParameterTypes));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeStruct* Inst)
	{
		TArray<SpvType*> MemberTypes;
		for(SpvId MemberTypeId : Inst->GetMemberTypeIds())
		{
			MemberTypes.Add(Context.Types[MemberTypeId].Get());
		}
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvStructType>(ResultId, MemberTypes));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeImage* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvType* SampledType = Context.Types[Inst->GetSampledType()].Get();
		Context.Types.emplace(ResultId, MakeUnique<SpvImageType>(ResultId, SampledType, Inst->GetDim()));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeSampler* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvSamplerType>(ResultId));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeArray* Inst)
	{
		SpvType* ElementType = Context.Types[Inst->GetElementType()].Get();
		uint32 Length = *(uint32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLength()].Storage).Value.GetData();
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvArrayType>(ResultId, ElementType, Length));
	}

	void SpvMetaVisitor::Visit(const SpvOpTypeRuntimeArray* Inst)
	{
		SpvType* ElementType = Context.Types[Inst->GetElementType()].Get();
		SpvId ResultId = Inst->GetId().value();
		Context.Types.emplace(ResultId, MakeUnique<SpvRuntimeArrayType>(ResultId, ElementType));
	}

	void SpvMetaVisitor::Visit(const SpvOpExecutionMode* Inst)
	{
		Context.EntryPoint = Inst->GetEntryPointId();
		if(Inst->GetMode() == SpvExecutionMode::LocalSize)
		{
			const TArray<uint8>& Operands = Inst->GetExtraOperands();
			Context.ThreadGroupSize.x = *(uint32*)(Operands.GetData());
			Context.ThreadGroupSize.y = *(uint32*)(Operands.GetData() + 4);
			Context.ThreadGroupSize.z = *(uint32*)(Operands.GetData() + 8);
		}
	}

	void SpvMetaVisitor::Visit(const SpvOpName* Inst)
	{
		Context.Names.emplace(Inst->GetTarget(), Inst->GetName());
	}

	void SpvMetaVisitor::Visit(const SpvOpString* Inst)
	{
		Context.DebugStrs.emplace(Inst->GetId().value(), Inst->GetStr());
	}

	void SpvMetaVisitor::Visit(const SpvOpDecorate* Inst)
	{
		const TArray<uint8>& ExtraOperands = Inst->GetExtraOperands();
		if(Inst->GetKind() == SpvDecorationKind::BuiltIn)
		{
			SpvBuiltIn BuiltIn = *(SpvBuiltIn*)ExtraOperands.GetData();
			SpvDecoration Decoration{
				.Kind = SpvDecorationKind::BuiltIn,
				.BuiltIn = {BuiltIn}
			};
			Context.Decorations.Add(Inst->GetTargetId(), MoveTemp(Decoration));
			Context.BuiltIns.Add(BuiltIn, Inst->GetTargetId());
		}
		else if(Inst->GetKind() == SpvDecorationKind::Binding)
		{
			uint32 Number = *(uint32*)ExtraOperands.GetData();
			SpvDecoration Decoration{
				.Kind = SpvDecorationKind::Binding,
				.Binding = {Number}
			};
			Context.Decorations.Add(Inst->GetTargetId(), MoveTemp(Decoration));
		}
		else if(Inst->GetKind() == SpvDecorationKind::DescriptorSet)
		{
			uint32 Number = *(uint32*)ExtraOperands.GetData();
			SpvDecoration Decoration{
				.Kind = SpvDecorationKind::DescriptorSet,
				.DescriptorSet = {Number}
			};
			Context.Decorations.Add(Inst->GetTargetId(), MoveTemp(Decoration));
		}
		else if(Inst->GetKind() == SpvDecorationKind::Location)
		{
			uint32 Number = *(uint32*)ExtraOperands.GetData();
			SpvDecoration Decoration{
				.Kind = SpvDecorationKind::Location,
				.Location = {Number}
			};
			Context.Decorations.Add(Inst->GetTargetId(), MoveTemp(Decoration));
		}
		else if(Inst->GetKind() == SpvDecorationKind::ArrayStride)
		{
			uint32 Number = *(uint32*)ExtraOperands.GetData();
			SpvDecoration Decoration{
				.Kind = SpvDecorationKind::ArrayStride,
				.ArrayStride = {Number}
			};
			Context.Decorations.Add(Inst->GetTargetId(), MoveTemp(Decoration));
		}
		else
		{
			SpvDecoration Decoration{
				.Kind = Inst->GetKind(),
			};
			Context.Decorations.Add(Inst->GetTargetId(), MoveTemp(Decoration));
		}
	}

	void SpvMetaVisitor::Visit(const SpvOpMemberDecorate* Inst)
	{
		const TArray<uint8>& ExtraOperands = Inst->GetExtraOperands();
		if(Inst->GetKind() == SpvDecorationKind::Offset)
		{
			uint32 ByteOffset = *(uint32*)ExtraOperands.GetData();
			SpvDecoration Decoration{
				.Kind = SpvDecorationKind::Offset,
				.Offset = {Inst->GetMember(), ByteOffset}
			};
			Context.Decorations.Add(Inst->GetStructureType(), MoveTemp(Decoration));
		}
		else
		{
			SpvDecoration Decoration{
				.Kind = Inst->GetKind(),
			};
			Context.Decorations.Add(Inst->GetStructureType(), MoveTemp(Decoration));
		}
	}

	void SpvMetaVisitor::Visit(const SpvOpConstant* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvObject Obj = {
			.Id = ResultId,
			.Type = Context.Types[Inst->GetResultType()].Get(),
			.Storage = SpvObject::Internal{Inst->GetValue()}
		};
		Context.Constants.emplace(ResultId, MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(const SpvOpConstantTrue* Inst)
	{
		SpvId ResultId = Inst->GetId().value();

		SpvType* RetType = Context.Types[Inst->GetResultType()].Get();
		check(RetType->GetKind() == SpvTypeKind::Bool);
		SpvObject Obj = {ResultId, RetType, SpvObject::Internal{Inst->GetValue()}};
		Context.Constants.emplace(ResultId, MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(const SpvOpConstantFalse* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvType* RetType = Context.Types[Inst->GetResultType()].Get();
		check(RetType->GetKind() == SpvTypeKind::Bool);
		SpvObject Obj = {ResultId, RetType, SpvObject::Internal{Inst->GetValue()}};
		Context.Constants.emplace(ResultId, MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(const SpvOpConstantComposite* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		
		SpvType* RetType = Context.Types[Inst->GetResultType()].Get();
		TArray<uint8> CompositeValue;
		for(SpvId ConstituentId : Inst->GetConstituents())
		{
			SpvObject& ConstituentObj = Context.Constants[ConstituentId];
			CompositeValue.Append(std::get<SpvObject::Internal>(ConstituentObj.Storage).Value);
		}
		SpvObject Obj = {ResultId, RetType, SpvObject::Internal{MoveTemp(CompositeValue)}};
		Context.Constants.emplace(ResultId, MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(const SpvOpConstantNull* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(ResultType));
		SpvObject Obj = {
			.Id = ResultId,
			.Type = ResultType,
			.Storage = SpvObject::Internal{MoveTemp(Value)}
		};
		Context.Constants.emplace(ResultId, MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(const SpvDebugTypeBasic* Inst)
	{
		int32 Size = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetSize()].Storage).Value.GetData();
		SpvDebugBasicTypeEncoding Encoding = *(SpvDebugBasicTypeEncoding*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetEncoding()].Storage).Value.GetData();
		auto BasicTypeDesc = MakeUnique<SpvBasicTypeDesc>(Context.DebugStrs[Inst->GetName()],
														  Size,
														  Encoding);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(BasicTypeDesc));
	}

	void SpvMetaVisitor::Visit(const SpvDebugTypeVector* Inst)
	{
		int32 CompCount = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetComponentCount()].Storage).Value.GetData();
		auto VectorTypeDesc = MakeUnique<SpvVectorTypeDesc>(static_cast<SpvBasicTypeDesc*>(Context.TypeDescs[Inst->GetBasicType()].Get()), CompCount);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(VectorTypeDesc));
	}

	void SpvMetaVisitor::Visit(const SpvDebugTypeMatrix* Inst)
	{
		int32 VectorCount = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetVectorCount()].Storage).Value.GetData();
		auto MatrixTypeDesc = MakeUnique<SpvMatrixTypeDesc>(static_cast<SpvVectorTypeDesc*>(Context.TypeDescs[Inst->GetVectorType()].Get()), VectorCount);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(MatrixTypeDesc));
	}

	void SpvMetaVisitor::Visit(const SpvDebugTypeComposite* Inst)
	{
		TArray<SpvTypeDesc*> MemberTypeDescs;
		for(SpvId MemberTypeDescId : Inst->GetMembers())
		{
			if (Context.TypeDescs.contains(MemberTypeDescId))
			{
				MemberTypeDescs.Add(Context.TypeDescs.at(MemberTypeDescId).Get());
			}
		}
		
		//The size may be unknown(DebugInfoNone)
		if(Context.Constants.contains(Inst->GetSize()))
		{
			int32 Size = *(int32*)std::get<SpvObject::Internal>(Context.Constants.at(Inst->GetSize()).Storage).Value.GetData();
			auto CompositeTypeDesc = MakeUnique<SpvCompositeTypeDesc>(Context.DebugStrs[Inst->GetName()],
																	  Size,
																	  MemberTypeDescs);
			Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(CompositeTypeDesc));
		}
		
		
	}

	void SpvMetaVisitor::Visit(const SpvDebugTypeMember* Inst)
	{
		int32 Offset = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetOffset()].Storage).Value.GetData();
		int32 Size = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetSize()].Storage).Value.GetData();
		auto MemberTypeDesc = MakeUnique<SpvMemberTypeDesc>(Context.DebugStrs[Inst->GetName()],
															Context.TypeDescs[Inst->GetType()].Get(),
															Offset, Size);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(MemberTypeDesc));
	}

	void SpvMetaVisitor::Visit(const SpvDebugTypeArray* Inst)
	{
		SpvTypeDesc* BaseTypeDesc = Context.TypeDescs[Inst->GetBaseType()].Get();
		TArray<int32> CompCounts;
		for(SpvId CompCountId : Inst->GetComponentCounts())
		{
			CompCounts.Add(*(int32*)std::get<SpvObject::Internal>(Context.Constants[CompCountId].Storage).Value.GetData());
		}
		auto ArrayTypeDesc = MakeUnique<SpvArrayTypeDesc>(BaseTypeDesc, CompCounts);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(ArrayTypeDesc));
	}

	void SpvMetaVisitor::Visit(const SpvDebugTypeFunction* Inst)
	{
		TArray<SpvTypeDesc*> ParmTypeDescs;
		for(SpvId ParmTypeDescId : Inst->GetParmTypes())
		{
			if (Context.TypeDescs.contains(ParmTypeDescId))
			{
				ParmTypeDescs.Add(Context.TypeDescs.at(ParmTypeDescId).Get());
			}

		}
		
		TUniquePtr<SpvTypeDesc> FuncTypeDesc;
		if(auto Search = Context.Types.find(Inst->GetReturnType()) ; Search != Context.Types.end())
		{
			SpvVoidType* ReturnType = static_cast<SpvVoidType*>(Search->second.Get());
			FuncTypeDesc = MakeUnique<SpvFuncTypeDesc>(ReturnType, ParmTypeDescs);
		}
		else
		{
			SpvTypeDesc* ReturnTypeDesc = Context.TypeDescs[Inst->GetReturnType()].Get();
			FuncTypeDesc = MakeUnique<SpvFuncTypeDesc>(ReturnTypeDesc, ParmTypeDescs);
		}
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(FuncTypeDesc));
	}

	void SpvMetaVisitor::Visit(const SpvDebugCompilationUnit* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.LexicalScopes.emplace(ResultId, MakeUnique<SpvCompilationUnit>(ResultId));
	}

	void SpvMetaVisitor::Visit(const SpvDebugLexicalBlock* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLine()].Storage).Value.GetData();
		SpvLexicalScope* ParentScope = Context.LexicalScopes[Inst->GetParentId()].Get();
		Context.LexicalScopes.emplace(ResultId, MakeUnique<SpvLexicalBlock>(ResultId, Line, ParentScope));
	}

	void SpvMetaVisitor::Visit(const SpvDebugFunction* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvLexicalScope* ParentScope = Context.LexicalScopes[Inst->GetParentId()].Get();
		const FString& FuncName = Context.DebugStrs[Inst->GetNameId()];
		SpvFuncTypeDesc* FuncTypeDesc = static_cast<SpvFuncTypeDesc*>(Context.TypeDescs[Inst->GetTypeDescId()].Get());
		int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLine()].Storage).Value.GetData();
		int32 ScopeLine = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetScopeLine()].Storage).Value.GetData();
		Context.LexicalScopes.emplace(ResultId, MakeUnique<SpvFunctionDesc>(ResultId, ParentScope, FuncName, FuncTypeDesc, Line, ScopeLine));
	}

	void SpvMetaVisitor::Visit(const SpvDebugInlinedAt* Inst)
	{

	}

	void SpvMetaVisitor::Visit(const SpvDebugLocalVariable* Inst)
	{
		SpvVariableDesc VarDesc{
			.Name = Context.DebugStrs[Inst->GetNameId()],
			.TypeDesc = Context.TypeDescs[Inst->GetTypeDescId()].Get(),
			.Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLine()].Storage).Value.GetData(),
			.Parent = Context.LexicalScopes[Inst->GetParentId()].Get(),
			.bGlobal = false
		};
		Context.VariableDescs.emplace(Inst->GetId().value(), MoveTemp(VarDesc));
	}

	void SpvMetaVisitor::Visit(const SpvDebugGlobalVariable* Inst)
	{
		//PS: Interface variables don't have corresponding descs
		SpvVariableDesc VarDesc{
			.Name = Context.DebugStrs[Inst->GetNameId()],
			.TypeDesc = Context.TypeDescs[Inst->GetTypeDescId()].Get(),
			.Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLine()].Storage).Value.GetData(),
			.Parent = Context.LexicalScopes[Inst->GetParentId()].Get(),
			.bGlobal = true
		};
		Context.VariableDescs.emplace(Inst->GetId().value(), MoveTemp(VarDesc));
		Context.VariableDescMap.emplace(Inst->GetVarId(), &Context.VariableDescs[Inst->GetId().value()]);
	}

	void SpvMetaVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets)
	{
		Context.Sections = InSections;
		Context.ExtSets = InExtSets;
		for(const auto& Inst : Insts)
		{
			if(const SpvOp* OpKind = std::get_if<SpvOp>(&Inst->GetKind()) ;
			   OpKind && *OpKind == SpvOp::Function)
			{
				break;
			}
			
			Inst->Accept(this);
		}
	}

	void SpirvParser::Accept(SpvVisitor* Visitor)
	{
		Visitor->Parse(Insts, SpvCode, Sections, ExtSets);
	}

	void SpirvParser::Parse(const TArray<uint32>& SpvCode)
	{
		this->SpvCode = SpvCode;
		int32 WordOffset = 5;
		while(WordOffset < SpvCode.Num())
		{
			uint32 Inst = SpvCode[WordOffset];
			int32 InstWordLen = int32(Inst >> 16);
			SpvOp OpCode = static_cast<SpvOp>(Inst & 0xffff);
			TUniquePtr<SpvInstruction> DecodedInst;
			if (OpCode == SpvOp::Capability)
			{
				Sections.FindOrAdd(SpvSectionKind::Capability, { WordOffset });
			}
			else if (OpCode == SpvOp::Extension)
			{
				Sections.FindOrAdd(SpvSectionKind::Extension, { WordOffset });
			}
			else if (OpCode == SpvOp::ExtInstImport)
			{
				Sections.FindOrAdd(SpvSectionKind::ExtInstImport, {WordOffset});
			}
			else if (OpCode == SpvOp::MemoryModel)
			{
				Sections.FindOrAdd(SpvSectionKind::MemoryModel, { WordOffset });
			}
			else if (OpCode == SpvOp::EntryPoint)
			{
				Sections.FindOrAdd(SpvSectionKind::EntryPoint, { WordOffset });
			}
			else if (OpCode == SpvOp::ExecutionMode)
			{
				Sections.FindOrAdd(SpvSectionKind::ExecutionMode, { WordOffset });
			}
			else if (OpCode == SpvOp::String || OpCode == SpvOp::Source)
			{
				Sections.FindOrAdd(SpvSectionKind::DebugString, { WordOffset });
			}
			else if (OpCode == SpvOp::Name || OpCode == SpvOp::MemberName)
			{
				Sections.FindOrAdd(SpvSectionKind::DebugName, { WordOffset });
			}
			else if (OpCode == SpvOp::Decorate || OpCode == SpvOp::MemberDecorate)
			{
				Sections.FindOrAdd(SpvSectionKind::Annotation, { WordOffset });
			}
			else if (OpCode == SpvOp::Function)
			{
				Sections.FindOrAdd(SpvSectionKind::Function, { WordOffset, SpvCode.Num() });
			}
			else if(!Sections.Contains(SpvSectionKind::Function))
			{
				Sections.FindOrAdd(SpvSectionKind::Type, { WordOffset });
			}

			switch(OpCode)
			{
			case SpvOp::EntryPoint:
			{
				SpvExecutionModel Model = static_cast<SpvExecutionModel>(SpvCode[WordOffset + 1]);
				SpvId EntryPoint = SpvCode[WordOffset + 2];
				FString EntryPointName = (char*)&SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpEntryPoint>(Model, EntryPoint, EntryPointName);
				break;
			}
			case SpvOp::ExecutionMode:
			{
				SpvId EntryPoint = SpvCode[WordOffset + 1];
				SpvExecutionMode Mode = static_cast<SpvExecutionMode>(SpvCode[WordOffset + 2]);
				TArray<uint8> Operands = {(uint8*)&SpvCode[WordOffset + 3], (InstWordLen - 3) * 4};
				DecodedInst = MakeUnique<SpvOpExecutionMode>(EntryPoint, Mode, Operands);
				break;
			}
			case SpvOp::Name:
			{
				SpvId Target = SpvCode[WordOffset + 1];
				char* Str = (char*)&SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpName>(Target, FString(Str));
				break;
			}
			case SpvOp::String:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				char* Str = (char*)&SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpString>(FString(Str));
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Decorate:
			{
				SpvId TargetId = SpvCode[WordOffset + 1];
				SpvDecorationKind Kind = static_cast<SpvDecorationKind>(SpvCode[WordOffset + 2]);
				TArray<uint8> Operands = {(uint8*)&SpvCode[WordOffset + 3], (InstWordLen - 3) * 4};
				DecodedInst = MakeUnique<SpvOpDecorate>(TargetId, Kind, Operands);
				break;
			}
			case SpvOp::MemberDecorate:
			{
				SpvId StructureType = SpvCode[WordOffset + 1];
				uint32 Member = SpvCode[WordOffset + 2];
				SpvDecorationKind Kind = static_cast<SpvDecorationKind>(SpvCode[WordOffset + 3]);
				TArray<uint8> Operands = {(uint8*)&SpvCode[WordOffset + 4], (InstWordLen - 4) * 4};
				DecodedInst = MakeUnique<SpvOpMemberDecorate>(StructureType, Member, Kind, Operands);
				break;
			}
			case SpvOp::TypeVoid:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpTypeVoid>();
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeFloat:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				uint32 Width = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpTypeFloat>(Width);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeInt:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				uint32 Width = SpvCode[WordOffset + 2];
				uint32 Signedness = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypeInt>(Width, Signedness);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeVector:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId ComponentType = SpvCode[WordOffset + 2];
				uint32 ComponentCount = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypeVector>(ComponentType, ComponentCount);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeMatrix:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId ColumnType = SpvCode[WordOffset + 2];
				uint32 ColumnCount = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypeMatrix>(ColumnType, ColumnCount);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeBool:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpTypeBool>();
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeStruct:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				int32 MemberTypeWordLen = InstWordLen - 2;
				TArray<SpvId> MemberTypeIds = {(SpvId*)&SpvCode[WordOffset + 2], MemberTypeWordLen};
				DecodedInst = MakeUnique<SpvOpTypeStruct>(MemberTypeIds);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypePointer:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvStorageClass StorageClass = static_cast<SpvStorageClass>(SpvCode[WordOffset + 2]);
				SpvId PointeeType = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypePointer>(StorageClass, PointeeType);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeFunction:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId ReturnType = SpvCode[WordOffset + 2];
				int32 ParameterTypeWordLen = InstWordLen - 3;
				TArray<SpvId> ParameterTypeIds = { (SpvId*)&SpvCode[WordOffset + 3], ParameterTypeWordLen };
				DecodedInst = MakeUnique<SpvOpTypeFunction>(ReturnType, ParameterTypeIds);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeImage:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId SampledType = SpvCode[WordOffset + 2];
				SpvDim Dim = static_cast<SpvDim>(SpvCode[WordOffset + 3]);
				uint32 Depth = SpvCode[WordOffset + 4];
				uint32 Arrayed = SpvCode[WordOffset + 5];
				uint32 MS = SpvCode[WordOffset + 6];
				uint32 Sampled = SpvCode[WordOffset + 7];
				SpvImageFormat Format = static_cast<SpvImageFormat>(SpvCode[WordOffset + 8]);
				DecodedInst = MakeUnique<SpvOpTypeImage>(SampledType, Dim, Depth, Arrayed, MS, Sampled, Format);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeSampler:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpTypeSampler>();
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeArray:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId ElementType =  SpvCode[WordOffset + 2];
				SpvId Length =  SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypeArray>(ElementType, Length);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::TypeRuntimeArray:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId ElementType =  SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpTypeRuntimeArray>(ElementType);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Constant:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				uint8* Value = (uint8*)&SpvCode[WordOffset + 3];
				int32 ValueWordLen = InstWordLen - 3;
				DecodedInst = MakeUnique<SpvOpConstant>(ResultType, TArray{Value, ValueWordLen * 4});
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConstantTrue:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpConstantTrue>(ResultType);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConstantFalse:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpConstantFalse>(ResultType);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConstantComposite:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				int32 ConstituentsWordLen = InstWordLen - 3;
				TArray<SpvId> Constituents = {(SpvId*)&SpvCode[WordOffset + 3], ConstituentsWordLen};
				DecodedInst = MakeUnique<SpvOpConstantComposite>(ResultType, Constituents);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConstantNull:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpConstantNull>(ResultType);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Function:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvFunctionControl FunctionControl = static_cast<SpvFunctionControl>(SpvCode[WordOffset + 3]);
				SpvId FunctionTypeId = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFunction>(ResultType, FunctionControl, FunctionTypeId);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FunctionParameter:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpFunctionParameter>(ResultType);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FunctionCall:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId FunctionId = SpvCode[WordOffset + 3];
				int32 ArgumentsWordLen = InstWordLen - 4;
				TArray<SpvId> ArgumentIds = {(SpvId*)&SpvCode[WordOffset + 4], ArgumentsWordLen};
				DecodedInst = MakeUnique<SpvOpFunctionCall>(ResultType, FunctionId, ArgumentIds);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Variable:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvStorageClass StorageClass = static_cast<SpvStorageClass>(SpvCode[WordOffset + 3]);
				std::optional<SpvId> Initializer;
				if(InstWordLen - 4 > 0)
				{
					Initializer = SpvCode[WordOffset + 4];
				}
				DecodedInst = MakeUnique<SpvOpVariable>(ResultType, StorageClass, Initializer);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Phi:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				int32 OperandNum = (InstWordLen - 3) / 2;
				TArray<TPair<SpvId, SpvId>> Operands;
				for (int32 Index = 0; Index < OperandNum; Index++)
				{
					SpvId Variable = SpvCode[WordOffset + Index * 2 + 3];
					SpvId parent = SpvCode[WordOffset + Index * 2 + 4];
				}
				DecodedInst = MakeUnique<SpvOpPhi>(ResultType, Operands);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::SelectionMerge:
			{
				SpvId MergeBlock = SpvCode[WordOffset + 1];
				SpvSelectionControl SelectionControl = static_cast<SpvSelectionControl>(SpvCode[WordOffset + 2]);
				DecodedInst = MakeUnique<SpvOpSelectionMerge>(MergeBlock, SelectionControl);
				break;
			}
			case SpvOp::Label:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpLabel>();
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Load:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Pointer = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpLoad>(ResultType, Pointer);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Store:
			{
				SpvId Pointer = SpvCode[WordOffset + 1];
				SpvId Object = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpStore>(Pointer, Object);
				break;
			}
			case SpvOp::VectorShuffle:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Vector1 = SpvCode[WordOffset + 3];
				SpvId Vector2 = SpvCode[WordOffset + 4];
				int32 ComponentsWordLen = InstWordLen - 5;
				TArray<uint32> Components = { &SpvCode[WordOffset + 5], ComponentsWordLen};
				DecodedInst = MakeUnique<SpvOpVectorShuffle>(ResultType, Vector1, Vector2, Components);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::CompositeConstruct:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				int32 ConstituentsWordLen = InstWordLen - 3;
				TArray<SpvId> Constituents = {(SpvId*)&SpvCode[WordOffset + 3], ConstituentsWordLen};
				DecodedInst = MakeUnique<SpvOpCompositeConstruct>(ResultType, Constituents);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::CompositeExtract:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Composite = SpvCode[WordOffset + 3];
				int32 IndexesWordLen = InstWordLen - 4;
				TArray<uint32> Indexes = {&SpvCode[WordOffset + 4], IndexesWordLen};
				DecodedInst = MakeUnique<SpvOpCompositeExtract>(ResultType, Composite, Indexes);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Transpose:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Matrix = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTranspose>(ResultType, Matrix);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::AccessChain:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId BasePointer = SpvCode[WordOffset + 3];
				int32 IndexesWordLen = InstWordLen - 4;
				TArray<SpvId> Indexes = {(SpvId*)&SpvCode[WordOffset + 4], IndexesWordLen};
				DecodedInst = MakeUnique<SpvOpAccessChain>(ResultType, BasePointer, Indexes);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::SampledImage:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Image = SpvCode[WordOffset + 3];
				SpvId Sampler = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpSampledImage>(ResultType, Image, Sampler);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ImageSampleImplicitLod:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId SampledImage = SpvCode[WordOffset + 3];
				SpvId Coordinate = SpvCode[WordOffset + 4];
				std::optional<SpvImageOperands> ImageOperands;
				TArray<SpvId> Operands;
				int32 OperandsWordLen = InstWordLen - 6;
				if(OperandsWordLen >= 0)
				{
					ImageOperands = static_cast<SpvImageOperands>(SpvCode[WordOffset + 5]);
					Operands = {(SpvId*)&SpvCode[WordOffset + 6], OperandsWordLen};
				}
				DecodedInst = MakeUnique<SpvOpImageSampleImplicitLod>(ResultType, SampledImage, Coordinate, ImageOperands, Operands);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ImageFetch:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Image = SpvCode[WordOffset + 3];
				SpvId Coordinate = SpvCode[WordOffset + 4];
				std::optional<SpvImageOperands> ImageOperands;
				TArray<SpvId> Operands;
				int32 OperandsWordLen = InstWordLen - 6;
				if(OperandsWordLen >= 0)
				{
					ImageOperands = static_cast<SpvImageOperands>(SpvCode[WordOffset + 5]);
					Operands = {(SpvId*)&SpvCode[WordOffset + 6], OperandsWordLen};
				}
				DecodedInst = MakeUnique<SpvOpImageFetch>(ResultType, Image, Coordinate, ImageOperands, Operands);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ImageQuerySizeLod:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Image = SpvCode[WordOffset + 3];
				SpvId Lod = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpImageQuerySizeLod>(ResultType, Image, Lod);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ImageQueryLevels:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Image = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpImageQueryLevels>(ResultType, Image);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConvertFToU:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId FloatValue = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpConvertFToU>(ResultType, FloatValue);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConvertFToS:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId FloatValue = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpConvertFToS>(ResultType, FloatValue);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConvertSToF:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId SignedValue = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpConvertSToF>(ResultType, SignedValue);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ConvertUToF:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId UnsignedValue = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpConvertUToF>(ResultType, UnsignedValue);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Bitcast:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpBitcast>(ResultType, Operand);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::SNegate:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpSNegate>(ResultType, Operand);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FNegate:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpFNegate>(ResultType, Operand);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::IAdd:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpIAdd>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FAdd:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFAdd>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ISub:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpISub>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FSub:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFSub>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::IMul:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpIMul>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FMul:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFMul>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::UDiv:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpUDiv>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::SDiv:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpSDiv>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FDiv:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFDiv>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::UMod:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpUMod>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::SRem:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpSRem>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::FRem:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFRem>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::VectorTimesScalar:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Vector = SpvCode[WordOffset + 3];
				SpvId Scalar = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpVectorTimesScalar>(ResultType, Vector, Scalar);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::MatrixTimesScalar:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Matrix = SpvCode[WordOffset + 3];
				SpvId Scalar = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpMatrixTimesScalar>(ResultType, Matrix, Scalar);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::VectorTimesMatrix:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Vector = SpvCode[WordOffset + 3];
				SpvId Matrix = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpVectorTimesMatrix>(ResultType, Vector, Matrix);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::MatrixTimesVector:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Matrix = SpvCode[WordOffset + 3];
				SpvId Vector = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpMatrixTimesVector>(ResultType, Matrix, Vector);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::MatrixTimesMatrix:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId LeftMatrix = SpvCode[WordOffset + 3];
				SpvId RightMatrix = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpMatrixTimesMatrix>(ResultType, LeftMatrix, RightMatrix);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Dot:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Vector1 = SpvCode[WordOffset + 3];
				SpvId Vector2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpDot>(ResultType, Vector1, Vector2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Any:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Vector = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpAny>(ResultType, Vector);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::All:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Vector = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpAll>(ResultType, Vector);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::IsNan:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId X = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpIsNan>(ResultType, X);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::IsInf:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId X = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpIsInf>(ResultType, X);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::LogicalOr:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpLogicalOr>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::LogicalAnd:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpLogicalAnd>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::LogicalNot:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpLogicalNot>(ResultType, Operand);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Select:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Condition = SpvCode[WordOffset + 3];
				SpvId Object1 = SpvCode[WordOffset + 4];
				SpvId Object2 = SpvCode[WordOffset + 5];
				DecodedInst = MakeUnique<SpvOpSelect>(ResultType, Condition, Object1, Object2);
				DecodedInst->SetId(ResultId);
				break;
			}
#define DECODE_COMPARISON(Name)                                                                  \
			case SpvOp::Name:                                                                    \
			{                                                                                    \
				SpvId ResultType = SpvCode[WordOffset + 1];                                      \
				SpvId ResultId = SpvCode[WordOffset + 2];                                        \
				SpvId Operand1 = SpvCode[WordOffset + 3];                                        \
				SpvId Operand2 = SpvCode[WordOffset + 4];                                        \
				DecodedInst = MakeUnique<SpvOp##Name>(ResultType, Operand1, Operand2);           \
				DecodedInst->SetId(ResultId);                                                    \
				break;                                                                           \
			}
			
			DECODE_COMPARISON(IEqual)
			DECODE_COMPARISON(INotEqual)
			DECODE_COMPARISON(UGreaterThan)
			DECODE_COMPARISON(SGreaterThan)
			DECODE_COMPARISON(UGreaterThanEqual)
			DECODE_COMPARISON(SGreaterThanEqual)
			DECODE_COMPARISON(ULessThan)
			DECODE_COMPARISON(SLessThan)
			DECODE_COMPARISON(ULessThanEqual)
			DECODE_COMPARISON(SLessThanEqual)
			DECODE_COMPARISON(FOrdEqual)
			DECODE_COMPARISON(FOrdNotEqual)
			DECODE_COMPARISON(FOrdLessThan)
			DECODE_COMPARISON(FOrdGreaterThan)
			DECODE_COMPARISON(FOrdLessThanEqual)
			DECODE_COMPARISON(FOrdGreaterThanEqual)
			
			case SpvOp::ShiftRightLogical:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Base = SpvCode[WordOffset + 3];
				SpvId Shift = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpShiftRightLogical>(ResultType, Base, Shift);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ShiftRightArithmetic:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Base = SpvCode[WordOffset + 3];
				SpvId Shift = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpShiftRightArithmetic>(ResultType, Base, Shift);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::ShiftLeftLogical:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Base = SpvCode[WordOffset + 3];
				SpvId Shift = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpShiftLeftLogical>(ResultType, Base, Shift);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::BitwiseOr:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpBitwiseOr>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::BitwiseXor:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpBitwiseXor>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::BitwiseAnd:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpBitwiseAnd>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Not:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpNot>(ResultType, Operand);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::DPdx:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId P = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpDPdx>(ResultType, P);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::DPdy:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId P = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpDPdy>(ResultType, P);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Fwidth:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId P = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpFwidth>(ResultType, P);
				DecodedInst->SetId(ResultId);
				break;
			}
			case SpvOp::Branch:
			{
				SpvId TargetLabel = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpBranch>(TargetLabel);
				break;
			}
			case SpvOp::BranchConditional:
			{
				SpvId Condition = SpvCode[WordOffset + 1];
				SpvId TrueLabel = SpvCode[WordOffset + 2];
				SpvId FalseLabel = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel);
				break;
			}
			case SpvOp::Switch:
			{
				SpvId Selector = SpvCode[WordOffset + 1];
				SpvId Default = SpvCode[WordOffset + 2];
				int32 TargetNum = (InstWordLen - 3) / 2;
				TArray<TPair<TArray<uint8>, SpvId>> Targets;
				for(int32 Index = 0; Index < TargetNum; Index++)
				{
					TArray<uint8> TargetValue = {(uint8*)&SpvCode[WordOffset + Index * 2 + 3], 4};
					SpvId TargetLabel = SpvCode[WordOffset + Index * 2 + 4];
					Targets.Add({MoveTemp(TargetValue), TargetLabel});
				}
				DecodedInst = MakeUnique<SpvOpSwitch>(Selector, Default, Targets);
				break;
			}
			case SpvOp::Kill:
			{
				DecodedInst = MakeUnique<SpvOpKill>();
				break;
			}	
			case SpvOp::Return:
			{
				DecodedInst = MakeUnique<SpvOpReturn>();
				break;
			}
			case SpvOp::ReturnValue:
			{
				SpvId Value = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpReturnValue>(Value);
				break;
			}
			case SpvOp::ExtInstImport:
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				char* Str = (char*)&SpvCode[WordOffset + 2];
				int32 StrWordLen = InstWordLen - 3;
				FString ExtSetName = FString(StrWordLen * 4, Str);
				if(ExtSetName == "NonSemantic.Shader.DebugInfo.100")
				{
					ExtSets.Add(ResultId, SpvExtSet::NonSemanticShaderDebugInfo100);
				}
				else if(ExtSetName == "GLSL.std.450")
				{
					ExtSets.Add(ResultId, SpvExtSet::GLSLstd450);
				}
				
				break;
			}
			case SpvOp::ExtInst:
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId ExtSetId = SpvCode[WordOffset + 3];
				if(ExtSets[ExtSetId] == SpvExtSet::NonSemanticShaderDebugInfo100)
				{
					SpvDebugInfo100 ExtOp = static_cast<SpvDebugInfo100>(SpvCode[WordOffset + 4]);
					if(ExtOp == SpvDebugInfo100::DebugTypeBasic)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId Size = SpvCode[WordOffset + 6];
						SpvId Encoding = SpvCode[WordOffset + 7];
						SpvId Flags = SpvCode[WordOffset + 8];
						DecodedInst = MakeUnique<SpvDebugTypeBasic>(ResultType, ExtSetId, Name, Size, Encoding, Flags);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugTypeVector)
					{
						SpvId BasicType = SpvCode[WordOffset + 5];
						SpvId ComponentCount = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugTypeVector>(ResultType, ExtSetId, BasicType, ComponentCount);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugTypeMatrix)
					{
						SpvId VectorType = SpvCode[WordOffset + 5];
						SpvId VectorCount = SpvCode[WordOffset + 6];
						SpvId ColumnMajor = SpvCode[WordOffset + 7];
						DecodedInst = MakeUnique<SpvDebugTypeMatrix>(VectorType, VectorCount, ColumnMajor);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugTypeComposite)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId Size = SpvCode[WordOffset + 12];
						int32 MembersWordLen = InstWordLen - 14;
						TArray<SpvId> Members = {(SpvId*)&SpvCode[WordOffset + 14], MembersWordLen};
						DecodedInst = MakeUnique<SpvDebugTypeComposite>(Name, Size, Members);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugTypeMember)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId Type = SpvCode[WordOffset + 6];
						SpvId Offset = SpvCode[WordOffset + 10];
						SpvId Size = SpvCode[WordOffset + 11];
						DecodedInst = MakeUnique<SpvDebugTypeMember>(Name, Type, Offset, Size);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugTypeArray)
					{
						SpvId BaseType = SpvCode[WordOffset + 5];
						int32 ComponentCountsWordLen = InstWordLen - 6;
						TArray<SpvId> ComponentCounts = {(SpvId*)&SpvCode[WordOffset + 6], ComponentCountsWordLen};
						DecodedInst = MakeUnique<SpvDebugTypeArray>(BaseType, ComponentCounts);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugTypeFunction)
					{
						SpvId ReturnType = SpvCode[WordOffset + 6];
						int32 ParmTypesWordLen = InstWordLen - 7;
						TArray<SpvId> ParmTypes = {(SpvId*)&SpvCode[WordOffset + 7], ParmTypesWordLen};
						DecodedInst = MakeUnique<SpvDebugTypeFunction>(ReturnType, ParmTypes);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugCompilationUnit)
					{
						DecodedInst = MakeUnique<SpvDebugCompilationUnit>();
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugLexicalBlock)
					{
						SpvId Line = SpvCode[WordOffset + 6];
						SpvId Parent = SpvCode[WordOffset + 8];
						DecodedInst = MakeUnique<SpvDebugLexicalBlock>(Line, Parent);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugFunction)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId TypeDesc = SpvCode[WordOffset + 6];
						SpvId Line = SpvCode[WordOffset + 8];
						SpvId Parent = SpvCode[WordOffset + 10];
						SpvId ScopeLine = SpvCode[WordOffset + 13];
						DecodedInst = MakeUnique<SpvDebugFunction>(Name, TypeDesc, Line, Parent, ScopeLine);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugScope)
					{
						SpvId Scope = SpvCode[WordOffset + 5];
						std::optional<SpvId> Inlined;
						if (InstWordLen - 6 > 0)
						{
							Inlined = SpvCode[WordOffset + 6];
						}
						DecodedInst = MakeUnique<SpvDebugScope>(Scope, Inlined);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugGlobalVariable)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId TypeDesc = SpvCode[WordOffset + 6];
						SpvId Line = SpvCode[WordOffset + 8];
						SpvId Parent = SpvCode[WordOffset + 10];
						SpvId Var = SpvCode[WordOffset + 12];
						DecodedInst = MakeUnique<SpvDebugGlobalVariable>(Name, TypeDesc, Line, Parent, Var);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugInlinedAt)
					{
						SpvId Line = SpvCode[WordOffset + 5];
						SpvId Scope = SpvCode[WordOffset + 6];
						std::optional<SpvId> Inlined;
						if(InstWordLen - 7 > 0)
						{
							Inlined = SpvCode[WordOffset + 7];
						}
						DecodedInst = MakeUnique<SpvDebugInlinedAt>(Line, Scope, Inlined);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugLocalVariable)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId TypeDesc = SpvCode[WordOffset + 6];
						SpvId Line = SpvCode[WordOffset + 8];
						SpvId Parent = SpvCode[WordOffset + 10];
						DecodedInst = MakeUnique<SpvDebugLocalVariable>(Name, TypeDesc, Line, Parent);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugFunctionDefinition)
					{
						SpvId Function = SpvCode[WordOffset + 5];
						SpvId Definition = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugFunctionDefinition>(Function, Definition);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugLine)
					{
						SpvId LineStart = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugLine>(LineStart);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugDeclare)
					{
						SpvId VarDesc = SpvCode[WordOffset + 5];
						SpvId Variable = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugDeclare>(VarDesc, Variable);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugValue)
					{
						SpvId LocalVariable = SpvCode[WordOffset + 5];
						SpvId Value = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugValue>(LocalVariable, Value);
						DecodedInst->SetId(ResultId);
					}
				}
				else if(ExtSets[ExtSetId] == SpvExtSet::GLSLstd450)
				{
					SpvGLSLstd450 ExtOp = static_cast<SpvGLSLstd450>(SpvCode[WordOffset + 4]);
					if(ExtOp == SpvGLSLstd450::RoundEven)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvRoundEven>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Trunc)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvTrunc>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::FAbs)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvFAbs>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::SAbs)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvSAbs>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::FSign)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvFSign>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::SSign)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvSSign>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Floor)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvFloor>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Ceil)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvCeil>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Fract)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvFract>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Sin)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvSin>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Cos)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvCos>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Tan)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvTan>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Asin)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvAsin>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Acos)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvAcos>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Atan)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvAtan>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Pow)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvPow>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Exp)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvExp>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Log)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvLog>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Exp2)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvExp2>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Log2)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvLog2>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Sqrt)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvSqrt>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::InverseSqrt)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvInverseSqrt>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Determinant)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvDeterminant>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::UMin)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvUMin>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::SMin)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvSMin>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::UMax)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvUMax>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::SMax)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvSMax>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::FClamp)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId MinVal = SpvCode[WordOffset + 6];
						SpvId MaxVal = SpvCode[WordOffset + 7];
						DecodedInst = MakeUnique<SpvFClamp>(ResultType, X, MinVal, MaxVal);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::UClamp)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId MinVal = SpvCode[WordOffset + 6];
						SpvId MaxVal = SpvCode[WordOffset + 7];
						DecodedInst = MakeUnique<SpvUClamp>(ResultType, X, MinVal, MaxVal);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::SClamp)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId MinVal = SpvCode[WordOffset + 6];
						SpvId MaxVal = SpvCode[WordOffset + 7];
						DecodedInst = MakeUnique<SpvSClamp>(ResultType, X, MinVal, MaxVal);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::FMix)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						SpvId A = SpvCode[WordOffset + 7];
						DecodedInst = MakeUnique<SpvFMix>(ResultType, X, Y, A);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Step)
					{
						SpvId Edge = SpvCode[WordOffset + 5];
						SpvId X = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvStep>(ResultType, Edge, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::SmoothStep)
					{
						SpvId Edge0 = SpvCode[WordOffset + 5];
						SpvId Edge1 = SpvCode[WordOffset + 6];
						SpvId X = SpvCode[WordOffset + 7];
						DecodedInst = MakeUnique<SpvSmoothStep>(ResultType, Edge0, Edge1, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::PackHalf2x16)
					{
						SpvId V = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvPackHalf2x16>(ResultType, V);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::UnpackHalf2x16)
					{
						SpvId V = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvUnpackHalf2x16>(ResultType, V);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Length)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvLength>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Distance)
					{
						SpvId P0 = SpvCode[WordOffset + 5];
						SpvId P1 = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDistance>(ResultType, P0, P1);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Cross)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvCross>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Normalize)
					{
						SpvId X = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvNormalize>(ResultType, X);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Reflect)
					{
						SpvId I = SpvCode[WordOffset + 5];
						SpvId N = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvReflect>(ResultType, I, N);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::Refract)
					{
						SpvId I = SpvCode[WordOffset + 5];
						SpvId N = SpvCode[WordOffset + 6];
						SpvId Eta = SpvCode[WordOffset + 7];
						DecodedInst = MakeUnique<SpvRefract>(ResultType, I, N, Eta);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::NMin)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvNMin>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvGLSLstd450::NMax)
					{
						SpvId X = SpvCode[WordOffset + 5];
						SpvId Y = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvNMax>(ResultType, X, Y);
						DecodedInst->SetId(ResultId);
					}
				}
				break;
			}
			default: break;
			}

			if(DecodedInst)
			{
				DecodedInst->SetWordOffset(WordOffset);
				DecodedInst->SetWordLen(InstWordLen);
				Insts.Add(MoveTemp(DecodedInst));
			}
			WordOffset += InstWordLen;
		}

		for (int i = (int)SpvSectionKind::Num - 1; i > 0; i--)
		{
			if (!Sections.Contains(SpvSectionKind(i-1)))
			{
				Sections.Add(SpvSectionKind(i - 1), { Sections[SpvSectionKind(i)].StartOffset });
			}
			Sections[SpvSectionKind(i - 1)].EndOffset = Sections[SpvSectionKind(i)].StartOffset;
		}
	}
}
