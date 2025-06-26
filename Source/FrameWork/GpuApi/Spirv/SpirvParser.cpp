#include "CommonHeader.h"
#include "SpirvParser.h"

namespace FW
{

	void SpvMetaVisitor::Visit(SpvOpVariable* Inst)
	{
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		if(StorageClass == SpvStorageClass::Uniform)
		{
			if(!Context.Types.Contains(Inst->GetResultType()))
			{
				return;
			}
			
			SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
			SpvVariable Var = {{PointerType->PointeeType}, false, StorageClass};
			SpvPointer Pointer{&Var};
			Context.GlobalVariables.Add(Inst->GetId().value(), MoveTemp(Var));
			Context.GlobalPointers.Add(Inst->GetId().value(), MoveTemp(Pointer));
		}
	}

	void SpvMetaVisitor::Visit(SpvOpTypeFloat* Inst)
	{
		Context.Types.Add(Inst->GetId().value(), MakeUnique<SpvFloatType>(Inst->GetWidth()));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeInt* Inst)
	{
		Context.Types.Add(Inst->GetId().value(), MakeUnique<SpvIntegerType>(Inst->GetWidth(), !!Inst->GetSignedness()));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeVector* Inst)
	{
		SpvType* ElementType = Context.Types[Inst->GetComponentTypeId()].Get();
		check(ElementType->IsScalar());
		Context.Types.Add(Inst->GetId().value(), MakeUnique<SpvVectorType>(static_cast<SpvScalarType*>(ElementType), Inst->GetComponentCount()));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeVoid* Inst)
	{
		Context.Types.Add(Inst->GetId().value(), MakeUnique<SpvVoidType>());
	}

	void SpvMetaVisitor::Visit(SpvOpTypeBool* Inst)
	{
		Context.Types.Add(Inst->GetId().value(), MakeUnique<SpvBoolType>());
	}

	void SpvMetaVisitor::Visit(SpvOpTypePointer* Inst)
	{
		if(!Context.Types.Contains(Inst->GetPointeeType()))
		{
			return;
		}
		
		SpvType* PointeeType = Context.Types[Inst->GetPointeeType()].Get();
		Context.Types.Add(Inst->GetId().value(), MakeUnique<SpvPointerType>(Inst->GetStorageClass(), PointeeType));
	}

	void SpvMetaVisitor::Visit(SpvOpTypeStruct* Inst)
	{
		TArray<SpvType*> MemberTypes;
		for(SpvId MemberTypeId : Inst->GetMemberTypeIds())
		{
			if(!Context.Types.Contains(MemberTypeId))
			{
				return;
			}
			MemberTypes.Add(Context.Types[MemberTypeId].Get());
		}
		Context.Types.Add(Inst->GetId().value(), MakeUnique<SpvStructType>(MemberTypes));
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
		Context.DebugStrs.Add(Inst->GetId().value(), Inst->GetStr());
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
		Context.Constants.Add(Inst->GetId().value(), MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvOpConstantTrue* Inst)
	{
		if(!Context.Types.Contains(Inst->GetResultType()))
		{
			return;
		}
		
		SpvType* RetType = Context.Types[Inst->GetResultType()].Get();
		check(RetType->GetKind() == SpvTypeKind::Bool);
		SpvObject Obj = {RetType,  SpvObject::Internal{Inst->GetValue()}};
		Context.Constants.Add(Inst->GetId().value(), MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvOpConstantFalse* Inst)
	{
		SpvType* RetType = Context.Types[Inst->GetResultType()].Get();
		check(RetType->GetKind() == SpvTypeKind::Bool);
		SpvObject Obj = {RetType,  SpvObject::Internal{Inst->GetValue()}};
		Context.Constants.Add(Inst->GetId().value(), MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvOpConstantComposite* Inst)
	{
		if(!Context.Types.Contains(Inst->GetResultType()))
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
		SpvObject Obj = {RetType, SpvObject::Internal{MoveTemp(CompositeValue)}};
		Context.Constants.Add(Inst->GetId().value(), MoveTemp(Obj));
	}

	void SpvMetaVisitor::Visit(SpvDebugCompilationUnit* Inst)
	{
		Context.LexicalScopes.Add(Inst->GetId().value(), MakeUnique<SpvCompilationUnit>());
	}

	void SpvMetaVisitor::Visit(SpvDebugFunction* Inst)
	{
		SpvLexicalScope* ParentScope = Context.LexicalScopes[Inst->GetParentId()].Get();
		const FString& FuncName = Context.DebugStrs[Inst->GetNameId()];
		const SpvTypeDesc& FuncTypeDesc = Context.TypeDescs[Inst->GetTypeDescId()];
		Context.LexicalScopes.Add(Inst->GetId().value(), MakeUnique<SpvFunctionDesc>(ParentScope, FuncName, &FuncTypeDesc, Inst->GetLineNumber()));
	}

	void SpvMetaVisitor::Visit(SpvDebugLocalVariable* Inst)
	{
		
	}

	void SpvMetaVisitor::Visit(SpvDebugGlobalVariable* Inst)
	{
		
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
				int32 StrWordLen = InstWordLen - 3;
				DecodedInst = MakeUnique<SpvOpString>(FString(StrWordLen * 4, Str));
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
			else if(OpCode == SpvOp::Constant)
			{
				SpvId ResultType = SpvCode[WordOffset + 1];
				SpvId ResultId = SpvCode[WordOffset + 2];
				uint8* Value = (uint8*)&SpvCode[WordOffset + 3];
				int32 ValueWordLen = InstWordLen - 4;
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
				DecodedInst = MakeUnique<SpvOpVariable>(ResultType, StorageClass);
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
					if(ExtOp == SpvDebugInfo100::DebugCompilationUnit)
					{
						DecodedInst = MakeUnique<SpvDebugCompilationUnit>();
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugFunction)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId TypeDesc = SpvCode[WordOffset + 6];
						int32 Line = SpvCode[WordOffset + 8];
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
						uint32 Line = SpvCode[WordOffset + 8];
						SpvId Parent = SpvCode[WordOffset + 10];
						SpvId Var = SpvCode[WordOffset + 12];
						DecodedInst = MakeUnique<SpvDebugGlobalVariable>(Name, TypeDesc, Line, Parent, Var);
						DecodedInst->SetId(ResultId);
					}
					else if(ExtOp == SpvDebugInfo100::DebugLocalVariable)
					{
						SpvId Name = SpvCode[WordOffset + 5];
						SpvId TypeDesc = SpvCode[WordOffset + 6];
						int32 Line = SpvCode[WordOffset + 8];
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
						SpvId VarId = SpvCode[WordOffset + 6];
						DecodedInst = MakeUnique<SpvDebugDeclare>(VarDesc, VarId);
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
