#include "CommonHeader.h"
#include "SpirvParser.h"

namespace FW
{

	void SpvMetaVisitor::Visit(SpvOpVariable* Inst)
	{
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvId ResultId = Inst->GetId().value();
		
		if(!Context.Types.contains(Inst->GetResultType()))
		{
			return;
		}
		
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{ResultId, PointerType->PointeeType}, false, StorageClass};
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
			std::get<SpvObject::Internal>(Var.Storage).Value = std::get<SpvObject::Internal>(Constant->Storage).Value;
			Var.Initialized = true;
		}
		
		Context.GlobalVariables.emplace(ResultId, MoveTemp(Var));
		SpvPointer Pointer{ &Context.GlobalVariables[ResultId] };
		Context.GlobalPointers.emplace(ResultId, MoveTemp(Pointer));
		
	}

	void SpvMetaVisitor::Visit(SpvOpTypeFloat* Inst)
	{
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvFloatType>(Inst->GetWidth()));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeInt* Inst)
	{
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvIntegerType>(Inst->GetWidth(), !!Inst->GetSignedness()));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeVector* Inst)
	{
		SpvType* ElementType = Context.Types[Inst->GetComponentTypeId()].Get();
		check(ElementType->IsScalar());
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvVectorType>(static_cast<SpvScalarType*>(ElementType), Inst->GetComponentCount()));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeVoid* Inst)
	{
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvVoidType>());
	}

	void SpvMetaVisitor::Visit(SpvOpTypeBool* Inst)
	{
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvBoolType>());
	}

	void SpvMetaVisitor::Visit(SpvOpTypePointer* Inst)
	{
		if(!Context.Types.contains(Inst->GetPointeeType()))
		{
			return;
		}
		
		SpvType* PointeeType = Context.Types[Inst->GetPointeeType()].Get();
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvPointerType>(Inst->GetStorageClass(), PointeeType));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeStruct* Inst)
	{
		TArray<SpvType*> MemberTypes;
		for(SpvId MemberTypeId : Inst->GetMemberTypeIds())
		{
			if(!Context.Types.contains(MemberTypeId))
			{
				return;
			}
			MemberTypes.Add(Context.Types[MemberTypeId].Get());
		}
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvStructType>(MemberTypes));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeArray* Inst)
	{
		SpvType* ElementType = Context.Types[Inst->GetElementType()].Get();
		uint32 Length = *(uint32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLength()].Storage).Value.GetData();
		Context.Types.emplace(Inst->GetId().value(), MakeUnique<SpvArrayType>(ElementType, Length));
	}

	void SpvMetaVisitor::Visit(SpvOpExecutionMode* Inst)
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

	void SpvMetaVisitor::Visit(SpvOpString* Inst)
	{
		Context.DebugStrs.emplace(Inst->GetId().value(), Inst->GetStr());
	}

	void SpvMetaVisitor::Visit(SpvOpDecorate* Inst)
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
	}

	void SpvMetaVisitor::Visit(SpvOpConstant* Inst)
	{
		SpvObject Obj = {
			.Type = Context.Types[Inst->GetResultType()].Get(),
			.Storage = SpvObject::Internal{Inst->GetValue()}
		};
		Context.Constants.emplace(Inst->GetId().value(), MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvOpConstantTrue* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		if(!Context.Types.contains(Inst->GetResultType()))
		{
			return;
		}
		
		SpvType* RetType = Context.Types[Inst->GetResultType()].Get();
		check(RetType->GetKind() == SpvTypeKind::Bool);
		SpvObject Obj = {ResultId, RetType, SpvObject::Internal{Inst->GetValue()}};
		Context.Constants.emplace(ResultId, MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvOpConstantFalse* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvType* RetType = Context.Types[Inst->GetResultType()].Get();
		check(RetType->GetKind() == SpvTypeKind::Bool);
		SpvObject Obj = {ResultId, RetType, SpvObject::Internal{Inst->GetValue()}};
		Context.Constants.emplace(ResultId, MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvOpConstantComposite* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		if(!Context.Types.contains(Inst->GetResultType()))
		{
			return;
		}
		
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

	void SpvMetaVisitor::Visit(SpvOpConstantNull* Inst)
	{
		SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(ResultType));
		SpvObject Obj = {
			.Type = ResultType,
			.Storage = SpvObject::Internal{MoveTemp(Value)}
		};
		Context.Constants.emplace(Inst->GetId().value(), MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvDebugTypeBasic* Inst)
	{
		int32 Size = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetSize()].Storage).Value.GetData();
		auto BasicTypeDesc = MakeUnique<SpvBasicTypeDesc>(Context.DebugStrs[Inst->GetName()],
														  Size,
														  Inst->GetEncoding());
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(BasicTypeDesc));
	}

	void SpvMetaVisitor::Visit(SpvDebugTypeVector* Inst)
	{
		int32 CompCount = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetComponentCount()].Storage).Value.GetData();
		auto VectorTypeDesc = MakeUnique<SpvVectorTypeDesc>(static_cast<SpvBasicTypeDesc*>(Context.TypeDescs[Inst->GetBasicType()].Get()), CompCount);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(VectorTypeDesc));
	}

	void SpvMetaVisitor::Visit(SpvDebugTypeComposite* Inst)
	{
		TArray<SpvTypeDesc*> MemberTypeDescs;
		for(SpvId MemberTypeDescId : Inst->GetMembers())
		{
			MemberTypeDescs.Add(Context.TypeDescs[MemberTypeDescId].Get());
		}
		//Skip opaque type
		if(MemberTypeDescs.IsEmpty())
		{
			return;
		}
		
		int32 Size = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetSize()].Storage).Value.GetData();
		auto CompositeTypeDesc = MakeUnique<SpvCompositeTypeDesc>(Context.DebugStrs[Inst->GetName()],
																  Size,
																  MemberTypeDescs);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(CompositeTypeDesc));
	}

	void SpvMetaVisitor::Visit(SpvDebugTypeMember* Inst)
	{
		int32 Offset = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetOffset()].Storage).Value.GetData();
		int32 Size = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetSize()].Storage).Value.GetData();
		auto MemberTypeDesc = MakeUnique<SpvMemberTypeDesc>(Context.DebugStrs[Inst->GetName()],
															Context.TypeDescs[Inst->GetType()].Get(),
															Offset, Size);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(MemberTypeDesc));
	}

	void SpvMetaVisitor::Visit(SpvDebugTypeArray* Inst)
	{
		SpvTypeDesc* BaseTypeDesc = Context.TypeDescs[Inst->GetBaseType()].Get();
		TArray<uint32> CompCounts;
		for(SpvId CompCountId : Inst->GetComponentCounts())
		{
			CompCounts.Add(*(uint32*)std::get<SpvObject::Internal>(Context.Constants[CompCountId].Storage).Value.GetData());
		}
		auto ArrayTypeDesc = MakeUnique<SpvArrayTypeDesc>(BaseTypeDesc, CompCounts);
		Context.TypeDescs.emplace(Inst->GetId().value(), MoveTemp(ArrayTypeDesc));
	}

	void SpvMetaVisitor::Visit(SpvDebugTypeFunction* Inst)
	{
		TArray<SpvTypeDesc*> ParmTypeDescs;
		for(SpvId ParmTypeDescId : Inst->GetParmTypes())
		{
			ParmTypeDescs.Add(Context.TypeDescs[ParmTypeDescId].Get());
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

	void SpvMetaVisitor::Visit(SpvDebugCompilationUnit* Inst)
	{
		Context.LexicalScopes.emplace(Inst->GetId().value(), MakeUnique<SpvCompilationUnit>());
	}

	void SpvMetaVisitor::Visit(SpvDebugLexicalBlock* Inst)
	{
		int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineNumber()].Storage).Value.GetData();
		SpvLexicalScope* ParentScope = Context.LexicalScopes[Inst->GetParentId()].Get();
		Context.LexicalScopes.emplace(Inst->GetId().value(), MakeUnique<SpvLexicalBlock>(Line, ParentScope));
	}

	void SpvMetaVisitor::Visit(SpvDebugFunction* Inst)
	{
		SpvLexicalScope* ParentScope = Context.LexicalScopes[Inst->GetParentId()].Get();
		const FString& FuncName = Context.DebugStrs[Inst->GetNameId()];
		SpvTypeDesc* FuncTypeDesc = Context.TypeDescs[Inst->GetTypeDescId()].Get();
		int32 Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineNumber()].Storage).Value.GetData();
		Context.LexicalScopes.emplace(Inst->GetId().value(), MakeUnique<SpvFunctionDesc>(ParentScope, FuncName, FuncTypeDesc, Line));
	}

	void SpvMetaVisitor::Visit(SpvDebugLocalVariable* Inst)
	{
		SpvVariableDesc VarDesc{
			.Name = Context.DebugStrs[Inst->GetNameId()],
			.TypeDesc = Context.TypeDescs[Inst->GetTypeDescId()].Get(),
			.Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineNumber()].Storage).Value.GetData(),
			.Parent = Context.LexicalScopes[Inst->GetParentId()].Get()
		};
		Context.VariableDescs.emplace(Inst->GetId().value(), MoveTemp(VarDesc));
	}

	void SpvMetaVisitor::Visit(SpvDebugGlobalVariable* Inst)
	{
		SpvVariableDesc VarDesc{
			.Name = Context.DebugStrs[Inst->GetNameId()],
			.TypeDesc = Context.TypeDescs.contains(Inst->GetTypeDescId()) ? Context.TypeDescs[Inst->GetTypeDescId()].Get() : nullptr,
			.Line = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineNumber()].Storage).Value.GetData(),
			.Parent = Context.LexicalScopes[Inst->GetParentId()].Get()
		};
		Context.VariableDescs.emplace(Inst->GetId().value(), MoveTemp(VarDesc));
		Context.VariableDescMap.emplace(Inst->GetVarId(), &Context.VariableDescs[Inst->GetId().value()]);
	}

	void SpvMetaVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts)
	{
		for(const auto& Inst : Insts)
		{
			if(const SpvOp* OpKind = std::get_if<SpvOp>(&Inst->GetKind()) ;
			   OpKind && *OpKind == SpvOp::Function)
			{
				break;
			}
			
			Inst->Accpet(this);
		}
	}

	void SpirvParser::Accept(SpvVisitor* Visitor)
	{
		Visitor->Parse(Insts);
	}

	void SpirvParser::Parse(const TArray<uint32>& SpvCode)
	{
		int32 WordOffset = 5;
		while(WordOffset < SpvCode.Num())
		{
			uint32 Inst = SpvCode[WordOffset];
			int32 InstWordLen = int32(Inst >> 16);
			SpvOp OpCode = static_cast<SpvOp>(Inst & 0xffff);
			TUniquePtr<SpvInstruction> DecodedInst;
			if(OpCode == SpvOp::EntryPoint)
			{
				SpvExecutionModel Model = static_cast<SpvExecutionModel>(SpvCode[WordOffset + 1]);
				SpvId EntryPoint = SpvCode[WordOffset + 2];
				FString EntryPointName = (char*)&SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpEntryPoint>(Model, EntryPoint, EntryPointName);
			}
			else if(OpCode == SpvOp::ExecutionMode)
			{
				SpvId EntryPoint = SpvCode[WordOffset + 1];
				SpvExecutionMode Mode = static_cast<SpvExecutionMode>(SpvCode[WordOffset + 2]);
				TArray<uint8> Operands = {(uint8*)&SpvCode[WordOffset + 3], (InstWordLen - 3) * 4};
				DecodedInst = MakeUnique<SpvOpExecutionMode>(EntryPoint, Mode, Operands);
			}
			else if(OpCode == SpvOp::String)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				char* Str = (char*)&SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpString>(FString(Str));
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::Decorate)
			{
				SpvId TargetId = SpvCode[WordOffset + 1];
				SpvDecorationKind Kind = static_cast<SpvDecorationKind>(SpvCode[WordOffset + 2]);
				TArray<uint8> Operands = {(uint8*)&SpvCode[WordOffset + 3], (InstWordLen - 3) * 4};
				DecodedInst = MakeUnique<SpvOpDecorate>(TargetId, Kind, Operands);
			}
			else if(OpCode == SpvOp::TypeVoid)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpTypeVoid>();
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::TypeFloat)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				uint32 Width = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpTypeFloat>(Width);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::TypeInt)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				uint32 Width = SpvCode[WordOffset + 2];
				uint32 Signedness = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypeInt>(Width, Signedness);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::TypeVector)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId ComponentType = SpvCode[WordOffset + 2];
				uint32 ComponentCount = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypeVector>(ComponentType, ComponentCount);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::TypeBool)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpTypeBool>();
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::TypeStruct)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				int32 MemberTypeWordLen = InstWordLen - 2;
				TArray<SpvId> MemberTypeIds = {(SpvId*)&SpvCode[WordOffset + 2], MemberTypeWordLen};
				DecodedInst = MakeUnique<SpvOpTypeStruct>(MemberTypeIds);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::TypePointer)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvStorageClass StorageClass = static_cast<SpvStorageClass>(SpvCode[WordOffset + 2]);
				SpvId PointeeType = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypePointer>(StorageClass, PointeeType);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::TypeArray)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				SpvId ElementType =  SpvCode[WordOffset + 2];
				SpvId Length =  SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpTypeArray>(ElementType, Length);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::Constant)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				uint8* Value = (uint8*)&SpvCode[WordOffset + 3];
				int32 ValueWordLen = InstWordLen - 3;
				DecodedInst = MakeUnique<SpvOpConstant>(ResultType, TArray{Value, ValueWordLen * 4});
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::ConstantTrue)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpConstantTrue>(ResultType);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::ConstantFalse)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpConstantFalse>(ResultType);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::ConstantComposite)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				int32 ConstituentsWordLen = InstWordLen - 3;
				TArray<SpvId> Constituents = {(SpvId*)&SpvCode[WordOffset + 3], ConstituentsWordLen};
				DecodedInst = MakeUnique<SpvOpConstantComposite>(ResultType, Constituents);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::ConstantNull)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpConstantNull>(ResultType);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::Function)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId FunctionTypeId = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFunction>(ResultType, FunctionTypeId);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::FunctionParameter)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpFunctionParameter>(ResultType);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::FunctionCall)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId FunctionId = SpvCode[WordOffset + 3];
				int32 ArgumentsWordLen = InstWordLen - 4;
				TArray<SpvId> ArgumentIds = {(SpvId*)&SpvCode[WordOffset + 4], ArgumentsWordLen};
				DecodedInst = MakeUnique<SpvOpFunctionCall>(ResultType, FunctionId, ArgumentIds);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::Variable)
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
			}
			else if(OpCode == SpvOp::Label)
			{
				SpvId ResultId = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpLabel>();
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::Load)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Pointer = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpLoad>(ResultType, Pointer);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::Store)
			{
				SpvId Pointer = SpvCode[WordOffset + 1];
				SpvId Object = SpvCode[WordOffset + 2];
				DecodedInst = MakeUnique<SpvOpStore>(Pointer, Object);
			}
			else if(OpCode == SpvOp::CompositeConstruct)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				int32 ConstituentsWordLen = InstWordLen - 3;
				TArray<SpvId> Constituents = {(SpvId*)&SpvCode[WordOffset + 3], ConstituentsWordLen};
				DecodedInst = MakeUnique<SpvOpCompositeConstruct>(ResultType, Constituents);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::CompositeExtract)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Composite = SpvCode[WordOffset + 3];
				int32 IndexesWordLen = InstWordLen - 4;
				TArray<uint32> Indexes = {&SpvCode[WordOffset + 4], IndexesWordLen};
				DecodedInst = MakeUnique<SpvOpCompositeExtract>(ResultType, Composite, Indexes);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::AccessChain)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId BasePointer = SpvCode[WordOffset + 3];
				int32 IndexesWordLen = InstWordLen - 4;
				TArray<SpvId> Indexes = {(SpvId*)&SpvCode[WordOffset + 4], IndexesWordLen};
				DecodedInst = MakeUnique<SpvOpAccessChain>(ResultType, BasePointer, Indexes);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::FDiv)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpFDiv>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::IEqual)
			{
				
			}
			else if(OpCode == SpvOp::INotEqual)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				SpvId Operand1 = SpvCode[WordOffset + 3];
				SpvId Operand2 = SpvCode[WordOffset + 4];
				DecodedInst = MakeUnique<SpvOpINotEqual>(ResultType, Operand1, Operand2);
				DecodedInst->SetId(ResultId);
			}
			else if(OpCode == SpvOp::DPdx)
			{
				
			}
			else if(OpCode == SpvOp::BranchConditional)
			{
				SpvId Condition = SpvCode[WordOffset + 1];
				SpvId TrueLabel = SpvCode[WordOffset + 2];
				SpvId FalseLabel = SpvCode[WordOffset + 3];
				DecodedInst = MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel);
			}
			else if(OpCode == SpvOp::Return)
			{
				DecodedInst = MakeUnique<SpvOpReturn>();
			}
			else if(OpCode == SpvOp::ReturnValue)
			{
				SpvId Value = SpvCode[WordOffset + 1];
				DecodedInst = MakeUnique<SpvOpReturnValue>(Value);
			}
			else if(OpCode == SpvOp::ExtInstImport)
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
				
			}
			else if(OpCode == SpvOp::ExtInst)
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
						SpvDebugBasicTypeEncoding Encoding = static_cast<SpvDebugBasicTypeEncoding>(SpvCode[WordOffset + 7]);
						DecodedInst = MakeUnique<SpvDebugTypeBasic>(Name, Size, Encoding);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugTypeVector)
					{
						SpvId BasicType = SpvCode[WordOffset + 5];
						SpvId ComponentCount = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugTypeVector>(BasicType, ComponentCount);
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
						DecodedInst = MakeUnique<SpvDebugFunction>(Name, TypeDesc, Line, Parent);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugScope)
					{
						SpvId Scope = SpvCode[WordOffset + 5];
						DecodedInst = MakeUnique<SpvDebugScope>(Scope);
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
					else if(ExtOp == SpvDebugInfo100::DebugLocalVariable)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId TypeDesc = SpvCode[WordOffset + 6];
						SpvId Line = SpvCode[WordOffset + 8];
						SpvId Parent = SpvCode[WordOffset + 10];
						DecodedInst = MakeUnique<SpvDebugLocalVariable>(Name, TypeDesc, Line, Parent);
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
						SpvId PointerId = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugDeclare>(VarDesc, PointerId);
						DecodedInst->SetId(ResultId);
					}
				}
				else if(ExtSets[ExtSetId] == SpvExtSet::GLSLstd450)
				{
					
				}
			}
			
			if(DecodedInst)
			{
				Insts.Add(MoveTemp(DecodedInst));
			}
			WordOffset += InstWordLen;
		}
	}
}
