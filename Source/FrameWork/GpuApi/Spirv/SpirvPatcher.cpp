#include "CommonHeader.h"
#include "SpirvPatcher.h"
#include "SpirvParser.h"
#include "ShaderConductor.hpp"

namespace FW
{
	void SpvPatcher::SetSpvContext(const TArray<TUniquePtr<SpvInstruction>>& InInsts, const TArray<uint32>& InSpvCode, SpvMetaContext* InMetaContext)
	{
		OriginInsts = &InInsts;
		SpvCode = InSpvCode;
		MetaVisitor = MakeUnique<SpvMetaVisitor>(*InMetaContext);
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

	void SpvPatcher::AddDebugName(SpvInstruction& InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		AddInstruction(Context.Sections[SpvSectionKind::DebugName].EndOffset, InInst);
	}

	void SpvPatcher::AddAnnotation(SpvInstruction& InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		AddInstruction(Context.Sections[SpvSectionKind::Annotation].EndOffset, InInst);
	}

	SpvId SpvPatcher::FindOrAddType(SpvInstruction& InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();

		//Avoid duplicate non-aggregate type declarations
		auto HasType = [&](SpvInstruction* A, SpvInstruction* B) {
			if (A->GetKind() == B->GetKind())
			{
				SpvOp OpCode = std::get<SpvOp>(A->GetKind());
				if (OpCode == SpvOp::TypeInt)
				{
					SpvOpTypeInt* TypeA = static_cast<SpvOpTypeInt*>(A);
					SpvOpTypeInt* TypeB = static_cast<SpvOpTypeInt*>(B);
					return TypeA->GetSignedness() == TypeB->GetSignedness() && TypeA->GetWidth() == TypeB->GetWidth();
				}
				else if (OpCode == SpvOp::TypeFloat)
				{
					SpvOpTypeFloat* TypeA = static_cast<SpvOpTypeFloat*>(A);
					SpvOpTypeFloat* TypeB = static_cast<SpvOpTypeFloat*>(B);
					return TypeA->GetWidth() == TypeB->GetWidth();
				}
				else if (OpCode == SpvOp::TypeMatrix)
				{
					SpvOpTypeMatrix* TypeA = static_cast<SpvOpTypeMatrix*>(A);
					SpvOpTypeMatrix* TypeB = static_cast<SpvOpTypeMatrix*>(B);
					return TypeA->GetColumnCount() == TypeB->GetColumnCount() && TypeA->GetColumnType() == TypeB->GetColumnType();
				}
				else if (OpCode == SpvOp::TypeVector)
				{
					SpvOpTypeVector* TypeA = static_cast<SpvOpTypeVector*>(A);
					SpvOpTypeVector* TypeB = static_cast<SpvOpTypeVector*>(B);
					return TypeA->GetComponentCount() == TypeB->GetComponentCount() && TypeA->GetComponentType() == TypeB->GetComponentType();
				}
				else if (OpCode == SpvOp::TypeFunction)
				{
					SpvOpTypeFunction* TypeA = static_cast<SpvOpTypeFunction*>(A);
					SpvOpTypeFunction* TypeB = static_cast<SpvOpTypeFunction*>(B);
					return TypeA->GetParameterTypes() == TypeB->GetParameterTypes() && TypeA->GetReturnType() == TypeB->GetReturnType();
				}
				else if (OpCode == SpvOp::TypePointer)
				{
					SpvOpTypePointer* TypeA = static_cast<SpvOpTypePointer*>(A);
					SpvOpTypePointer* TypeB = static_cast<SpvOpTypePointer*>(B);
					return TypeA->GetPointeeType() == TypeB->GetPointeeType() && TypeA->GetStorageClass() == TypeB->GetStorageClass();
				}
				else if (OpCode == SpvOp::TypeVoid)
				{
					return true;
				}
				else if (OpCode == SpvOp::TypeBool)
				{
					return true;
				}
			}
			return false;
		};

		SpvId ResultId;
		for (auto& Inst : *OriginInsts)
		{
			if (HasType(Inst.Get(), &InInst))
			{
				ResultId = Inst->GetId().value();
			}
		}
		for (auto& Inst : PatchedInsts)
		{
			if (HasType(Inst.Get(), &InInst))
			{
				ResultId = Inst->GetId().value();
			}
		}
		
		if(!ResultId.IsValid())
		{
			ResultId = NewId();
			InInst.SetId(ResultId);

			AddInstruction(Context.Sections[SpvSectionKind::Type].EndOffset, InInst);
		}

		return ResultId;
	}

	template<typename T>
	SpvId SpvPatcher::FindOrAddConstant(T InConstant)
	{
		SpvId ConstantType;
		if constexpr (std::is_same_v<T, uint32>)
		{
			SpvOpTypeInt UIntTypeOp{ 32, 0 };
			ConstantType = FindOrAddType(UIntTypeOp);
		}
		else if constexpr (std::is_same_v<T, int32>)
		{
			SpvOpTypeInt IntTypeOp{ 32, 1 };
			ConstantType = FindOrAddType(IntTypeOp);
		}

		SpvOpConstant InInst{ ConstantType, {(uint8*)&InConstant, sizeof(T)} };
		SpvMetaContext& Context = MetaVisitor->GetContext();
		auto HasConstant = [&](SpvInstruction* A, SpvInstruction* B) {
			if (A->GetKind() == B->GetKind())
			{
				SpvOp OpCode = std::get<SpvOp>(A->GetKind());
				if (OpCode == SpvOp::Constant)
				{
					SpvOpConstant* ConstantA = static_cast<SpvOpConstant*>(A);
					SpvOpConstant* ConstantB = static_cast<SpvOpConstant*>(B);
					return ConstantA->GetResultType() == ConstantB->GetResultType() && ConstantA->GetValue() == ConstantB->GetValue();
				}
			}
			return false;
			};

		SpvId ResultId;
		for (auto& Inst : *OriginInsts)
		{
			if (HasConstant(Inst.Get(), &InInst))
			{
				ResultId = Inst->GetId().value();
			}
		}
		for (auto& Inst : PatchedInsts)
		{
			if (HasConstant(Inst.Get(), &InInst))
			{
				ResultId = Inst->GetId().value();
			}
		}

		if (!ResultId.IsValid())
		{
			ResultId = NewId();
			InInst.SetId(ResultId);
			AddInstruction(Context.Sections[SpvSectionKind::Contant].EndOffset, InInst);
		}
		return ResultId;
	}

	template SpvId SpvPatcher::FindOrAddConstant<uint32>(uint32);
	template SpvId SpvPatcher::FindOrAddConstant<int32>(int32);

	void SpvPatcher::AddGlobalVariable(SpvInstruction& InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		AddInstruction(Context.Sections[SpvSectionKind::GlobalVar].EndOffset, InInst);
	}

	void SpvPatcher::AddFunction(TArray<std::reference_wrapper<SpvInstruction>>& Function)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		for (auto& Inst : Function)
		{
			AddInstruction(Context.Sections[SpvSectionKind::Function].EndOffset, Inst);
		}
	}

	void SpvPatcher::AddInstruction(int WordOffset, SpvInstruction& InInst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		TArray<uint32> InstBin = InInst.ToBinary();
		SpvCode.Insert(InstBin, WordOffset);
		InInst.SetWordOffset(WordOffset);
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
		UpdateOriginInsts(WordOffset, InstBin.Num());
		if ((int)TargetSection <= (int)SpvSectionKind::Function)
		{
			InInst.Accept(MetaVisitor.Get());
		}
		PatchedInsts.Add(InInst.Clone());
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

	void SpvPatcher::UpdateOriginInsts(int WordOffset, int WordSize)
	{
		for (auto& Inst : *OriginInsts)
		{
			int CurInstOffset = Inst->GetWordOffset().value();
			if (CurInstOffset >= WordOffset)
			{
				Inst->SetWordOffset(CurInstOffset + WordSize);
			}
		}
	}
}
