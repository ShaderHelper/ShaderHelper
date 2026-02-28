#include "CommonHeader.h"
#include "SpirvPatcher.h"
#include "SpirvParser.h"
#include "ShaderConductor.hpp"

namespace FW
{
	auto SpvInstOffsetProjection = [](SpvInstruction* Inst) { return Inst->GetWordOffset().value(); };

	void SpvPatcher::SetSpvContext(const TArray<TUniquePtr<SpvInstruction>>& InInsts, const TArray<uint32>& InSpvCode, SpvMetaContext* InMetaContext)
	{
		OriginInsts = &InInsts;
		SpvCode = InSpvCode;
		MetaVisitor = MakeUnique<SpvMetaVisitor>(*InMetaContext);

		SortedOriginInsts.Reserve(InInsts.Num());
		for (const auto& Inst : InInsts)
		{
			SortedOriginInsts.Add(Inst.Get());
		}
		SortedOriginInsts.Sort([](SpvInstruction& A, SpvInstruction& B) {
			return A.GetWordOffset().value() < B.GetWordOffset().value();
		});
	}

	void SpvPatcher::Dump(const FString& SavedFileName) const
	{
		ShaderConductor::Compiler::DisassembleDesc SpvDisassembleDesc{
						.language = ShaderConductor::ShadingLanguage::SpirV,
						.binary = (uint8*)SpvCode.GetData(),
						.binarySize = (uint32_t)SpvCode.Num() * 4
		};
		ShaderConductor::Compiler::ResultDesc SpvTextResultDesc = ShaderConductor::Compiler::Disassemble(SpvDisassembleDesc);
		FString SpvSourceText = { (int32)SpvTextResultDesc.target.Size(), static_cast<const char*>(SpvTextResultDesc.target.Data()) };
		if (SpvTextResultDesc.hasError)
		{
			FString ErrorInfo = static_cast<const char*>(SpvTextResultDesc.errorWarningMsg.Data());
			SpvSourceText = MoveTemp(ErrorInfo);
		}
		FFileHelper::SaveStringToFile(SpvSourceText, *SavedFileName);
	}

	SpvId SpvPatcher::FindOrAddTypeDesc(TUniquePtr<SpvInstruction> InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		const SpvInstKind& Kind = InInst->GetKind();
		const auto* DebugOp = std::get_if<SpvDebugInfo100>(&Kind);

		if (DebugOp)
		{
			if (*DebugOp == SpvDebugInfo100::DebugTypeBasic)
			{
				auto* Inst = static_cast<SpvDebugTypeBasic*>(InInst.Get());
				int32 InstSize = *(int32*)std::get<SpvObject::Internal>(Context.Constants.at(Inst->GetSize()).Storage).Value.GetData();
				auto InstEncoding = *(SpvDebugBasicTypeEncoding*)std::get<SpvObject::Internal>(Context.Constants.at(Inst->GetEncoding()).Storage).Value.GetData();

				for (const auto& [Id, Desc] : Context.TypeDescs)
				{
					if (Desc->GetKind() == SpvTypeDescKind::Basic)
					{
						auto* BasicDesc = static_cast<SpvBasicTypeDesc*>(Desc.Get());
						if (BasicDesc->GetSize() == InstSize && BasicDesc->GetEncoding() == InstEncoding)
							return Id;
					}
				}
			}
			else if (*DebugOp == SpvDebugInfo100::DebugTypeVector)
			{
				auto* Inst = static_cast<SpvDebugTypeVector*>(InInst.Get());
				int32 CompCount = *(int32*)std::get<SpvObject::Internal>(Context.Constants.at(Inst->GetComponentCount()).Storage).Value.GetData();
				SpvTypeDesc* BasicTypeDesc = Context.TypeDescs.contains(Inst->GetBasicType()) ?
					Context.TypeDescs.at(Inst->GetBasicType()).Get() : nullptr;

				for (const auto& [Id, Desc] : Context.TypeDescs)
				{
					if (Desc->GetKind() == SpvTypeDescKind::Vector)
					{
						auto* VecDesc = static_cast<SpvVectorTypeDesc*>(Desc.Get());
						if (VecDesc->GetCompCount() == CompCount && VecDesc->GetBasicTypeDesc() == BasicTypeDesc)
							return Id;
					}
				}
			}
		}

		SpvId ResultId = NewId();
		InInst->SetId(ResultId);
		AddInstruction(Context.Sections[SpvSectionKind::Type].EndOffset, MoveTemp(InInst));
		return ResultId;
	}

	SpvId SpvPatcher::FindOrAddDebugStr(const FString& Str)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();

		for (const auto& [Id, ExistingStr] : Context.DebugStrs)
		{
			if (ExistingStr == Str)
				return Id;
		}

		TUniquePtr<SpvInstruction> InInst = MakeUnique<SpvOpString>(Str);
		SpvId ResultId = NewId();
		InInst->SetId(ResultId);
		AddInstruction(Context.Sections[SpvSectionKind::DebugString].EndOffset, MoveTemp(InInst));
		return ResultId;
	}

	void SpvPatcher::AddDebugName(TUniquePtr<SpvInstruction> InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		AddInstruction(Context.Sections[SpvSectionKind::DebugName].EndOffset, MoveTemp(InInst));
	}

	void SpvPatcher::AddAnnotation(TUniquePtr<SpvInstruction> InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		AddInstruction(Context.Sections[SpvSectionKind::Annotation].EndOffset, MoveTemp(InInst));
	}

	SpvId SpvPatcher::FindOrAddType(TUniquePtr<SpvInstruction> InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		const SpvInstKind& Kind = InInst->GetKind();
		const auto* Op = std::get_if<SpvOp>(&Kind);

		if (Op)
		{
			for (const auto& [Id, Type] : Context.Types)
			{
				if (*Op == SpvOp::TypeInt && Type->GetKind() == SpvTypeKind::Integer)
				{
					auto* Src = static_cast<SpvOpTypeInt*>(InInst.Get());
					auto* Dst = static_cast<SpvIntegerType*>(Type.Get());
					if (Src->GetWidth() == Dst->GetWidth() && (Src->GetSignedness() == 1) == Dst->IsSigend())
						return Id;
				}
				else if (*Op == SpvOp::TypeFloat && Type->GetKind() == SpvTypeKind::Float)
				{
					auto* Src = static_cast<SpvOpTypeFloat*>(InInst.Get());
					auto* Dst = static_cast<SpvFloatType*>(Type.Get());
					if (Src->GetWidth() == Dst->GetWidth())
						return Id;
				}
				else if (*Op == SpvOp::TypeVector && Type->GetKind() == SpvTypeKind::Vector)
				{
					auto* Src = static_cast<SpvOpTypeVector*>(InInst.Get());
					auto* Dst = static_cast<SpvVectorType*>(Type.Get());
					if (Src->GetComponentType() == Dst->ElementType->GetId() && Src->GetComponentCount() == Dst->ElementCount)
						return Id;
				}
				else if (*Op == SpvOp::TypeMatrix && Type->GetKind() == SpvTypeKind::Matrix)
				{
					auto* Src = static_cast<SpvOpTypeMatrix*>(InInst.Get());
					auto* Dst = static_cast<SpvMatrixType*>(Type.Get());
					if (Src->GetColumnType() == Dst->ElementType->GetId() && Src->GetColumnCount() == Dst->ElementCount)
						return Id;
				}
				else if (*Op == SpvOp::TypeArray && Type->GetKind() == SpvTypeKind::Array)
				{
					auto* Src = static_cast<SpvOpTypeArray*>(InInst.Get());
					auto* Dst = static_cast<SpvArrayType*>(Type.Get());
					if (Src->GetElementType() == Dst->ElementType->GetId() && Dst->Length == *(uint32*)std::get<SpvObject::Internal>(Context.Constants.at(Src->GetLength()).Storage).Value.GetData())
						return Id;
				}
				else if (*Op == SpvOp::TypePointer && Type->GetKind() == SpvTypeKind::Pointer)
				{
					auto* Src = static_cast<SpvOpTypePointer*>(InInst.Get());
					auto* Dst = static_cast<SpvPointerType*>(Type.Get());
					if (Src->GetPointeeType() == Dst->PointeeType->GetId() && Src->GetStorageClass() == Dst->StorageClass)
						return Id;
				}
				else if (*Op == SpvOp::TypeFunction && Type->GetKind() == SpvTypeKind::Function)
				{
					auto* Src = static_cast<SpvOpTypeFunction*>(InInst.Get());
					auto* Dst = static_cast<SpvFunctionType*>(Type.Get());
					if (Src->GetReturnType() == Dst->ReturnType->GetId() && Src->GetParameterTypes().Num() == Dst->ParameterTypes.Num())
					{
						bool Match = true;
						for (int i = 0; i < Src->GetParameterTypes().Num(); i++)
						{
							if (Src->GetParameterTypes()[i] != Dst->ParameterTypes[i]->GetId()) { Match = false; break; }
						}
						if (Match) return Id;
					}
				}
				else if (*Op == SpvOp::TypeVoid && Type->GetKind() == SpvTypeKind::Void)
				{
					return Id;
				}
				else if (*Op == SpvOp::TypeBool && Type->GetKind() == SpvTypeKind::Bool)
				{
					return Id;
				}
			}
		}

		SpvId ResultId = NewId();
		InInst->SetId(ResultId);
		AddInstruction(Context.Sections[SpvSectionKind::Type].EndOffset, MoveTemp(InInst));
		return ResultId;
	}

	SpvId SpvPatcher::FindOrAddConstant(TUniquePtr<SpvInstruction> InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		const SpvInstKind& Kind = InInst->GetKind();
		SpvOp OpCode = std::get<SpvOp>(Kind);

		SpvId ResultTypeId;
		TArray<uint8> ValueBytes;

		if (OpCode == SpvOp::Constant)
		{
			auto* C = static_cast<SpvOpConstant*>(InInst.Get());
			ResultTypeId = C->GetResultType();
			ValueBytes = C->GetValue();
		}
		else if (OpCode == SpvOp::ConstantComposite)
		{
			auto* C = static_cast<SpvOpConstantComposite*>(InInst.Get());
			ResultTypeId = C->GetResultType();
			for (SpvId ConstituentId : C->GetConstituents())
			{
				const auto& Obj = Context.Constants.at(ConstituentId);
				ValueBytes.Append(std::get<SpvObject::Internal>(Obj.Storage).Value);
			}
		}

		for (const auto& [Id, Obj] : Context.Constants)
		{
			if (Obj.Type && Obj.Type->GetId() == ResultTypeId)
			{
				const auto& ObjValue = std::get<SpvObject::Internal>(Obj.Storage).Value;
				if (ObjValue == ValueBytes)
					return Id;
			}
		}

		SpvId ResultId = NewId();
		InInst->SetId(ResultId);
		AddInstruction(Context.Sections[SpvSectionKind::Contant].EndOffset, MoveTemp(InInst));
		return ResultId;
	}

	template<typename T>
	requires requires { T{} + T{}; }
	SpvId SpvPatcher::FindOrAddConstant(T InConstant)
	{
		SpvId ConstantType;
		TUniquePtr<SpvInstruction> ConstantInst;
		if constexpr (std::is_same_v<T, uint32>)
		{
			ConstantType = FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			ConstantInst = MakeUnique<SpvOpConstant>(ConstantType, TArray<uint8>{(uint8*)&InConstant, sizeof(T)});
		}
		else if constexpr (std::is_same_v<T, uint64>)
		{
			ConstantType = FindOrAddType(MakeUnique<SpvOpTypeInt>(64, 0));
			ConstantInst = MakeUnique<SpvOpConstant>(ConstantType, TArray<uint8>{(uint8*)&InConstant, sizeof(T)});
		}
		else if constexpr (std::is_same_v<T, int32>)
		{
			ConstantType = FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
			ConstantInst = MakeUnique<SpvOpConstant>(ConstantType, TArray<uint8>{(uint8*)&InConstant, sizeof(T)});
		}
		else if constexpr (std::is_same_v<T, int64>)
		{
			ConstantType = FindOrAddType(MakeUnique<SpvOpTypeInt>(64, 1));
			ConstantInst = MakeUnique<SpvOpConstant>(ConstantType, TArray<uint8>{(uint8*)&InConstant, sizeof(T)});
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			ConstantType = FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			ConstantInst = MakeUnique<SpvOpConstant>(ConstantType, TArray<uint8>{(uint8*)&InConstant, sizeof(T)});
		}
		else if constexpr (std::is_same_v<T, Vector2u>)
		{
			SpvId BaseType = FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			ConstantType = FindOrAddType(MakeUnique<SpvOpTypeVector>(BaseType, 2));
			ConstantInst = MakeUnique<SpvOpConstantComposite>(ConstantType, TArray<SpvId>{
				FindOrAddConstant(InConstant.X),
				FindOrAddConstant(InConstant.Y)
			});
		}
		else if constexpr (std::is_same_v<T, Vector2i>)
		{
			SpvId BaseType = FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
			ConstantType = FindOrAddType(MakeUnique<SpvOpTypeVector>(BaseType, 2));
			ConstantInst = MakeUnique<SpvOpConstantComposite>(ConstantType, TArray<SpvId>{
				FindOrAddConstant(InConstant.X),
				FindOrAddConstant(InConstant.Y)
			});
		}

		return FindOrAddConstant(MoveTemp(ConstantInst));
	}

	template SpvId SpvPatcher::FindOrAddConstant<uint32>(uint32);
	template SpvId SpvPatcher::FindOrAddConstant<uint64>(uint64);
	template SpvId SpvPatcher::FindOrAddConstant<int32>(int32);
	template SpvId SpvPatcher::FindOrAddConstant<int64>(int64);
	template SpvId SpvPatcher::FindOrAddConstant<float>(float);
	template SpvId SpvPatcher::FindOrAddConstant<Vector2i>(Vector2i);
	template SpvId SpvPatcher::FindOrAddConstant<Vector2u>(Vector2u);

	void SpvPatcher::AddGlobalVariable(TUniquePtr<SpvInstruction> InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		AddInstruction(Context.Sections[SpvSectionKind::GlobalVar].EndOffset, MoveTemp(InInst));
	}

	void SpvPatcher::AddFunction(TArray<TUniquePtr<SpvInstruction>>&& Function)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		for (auto& Inst : Function)
		{
			AddInstruction(Context.Sections[SpvSectionKind::Function].EndOffset, MoveTemp(Inst));
		}
	}

	void SpvPatcher::AddInstructions(int WordOffset, TArray<TUniquePtr<SpvInstruction>>&& InInsts)
	{
		for (int i = InInsts.Num() - 1; i >= 0; i--)
		{
			AddInstruction(WordOffset, MoveTemp(InInsts[i]));
		}
	}

	void SpvPatcher::AddInstruction(int WordOffset, TUniquePtr<SpvInstruction> InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		TArray<uint32> InstBin = InInst->ToBinary();
		SpvCode.Insert(InstBin, WordOffset);
		InInst->SetWordOffset(WordOffset);
		InInst->SetWordLen(InstBin.Num());
		SpvSectionKind TargetSection{};
		for (int i = (int)SpvSectionKind::Capability; i < (int)SpvSectionKind::Num; i++)
		{
			if (WordOffset <= Context.Sections[SpvSectionKind(i)].EndOffset)
			{
				TargetSection = SpvSectionKind(i);
				break;
			}
		}
		UpdateSection(TargetSection, InstBin.Num());
		UpdateInsts(WordOffset, InstBin.Num());
		if ((int)TargetSection < (int)SpvSectionKind::Function)
		{
			InInst->Accept(MetaVisitor.Get());
		}
		SpvInstruction* RawPtr = InInst.Get();
		PatchedInsts.Add(MoveTemp(InInst));

		int InsertIdx = Algo::LowerBoundBy(SortedPatchedInsts, WordOffset, SpvInstOffsetProjection);
		SortedPatchedInsts.Insert(RawPtr, InsertIdx);
	}

	void SpvPatcher::UpdateSection(SpvSectionKind DirtySection, int WordSize)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		Context.Sections[DirtySection].EndOffset += WordSize;
		for (int i = (int)DirtySection + 1; i < (int)SpvSectionKind::Num; i++)
		{
			Context.Sections[SpvSectionKind(i)].StartOffset += WordSize;
			Context.Sections[SpvSectionKind(i)].EndOffset += WordSize;
		}
	}

	void SpvPatcher::UpdateInsts(int WordOffset, int WordSize)
	{
		int OriginStart = Algo::LowerBoundBy(SortedOriginInsts, WordOffset, SpvInstOffsetProjection);
		for (int i = OriginStart; i < SortedOriginInsts.Num(); i++)
		{
			int CurInstOffset = SortedOriginInsts[i]->GetWordOffset().value();
			SortedOriginInsts[i]->SetWordOffset(CurInstOffset + WordSize);
		}

		int PatchedStart = Algo::LowerBoundBy(SortedPatchedInsts, WordOffset, SpvInstOffsetProjection);
		for (int i = PatchedStart; i < SortedPatchedInsts.Num(); i++)
		{
			int CurInstOffset = SortedPatchedInsts[i]->GetWordOffset().value();
			SortedPatchedInsts[i]->SetWordOffset(CurInstOffset + WordSize);
		}
	}
}
