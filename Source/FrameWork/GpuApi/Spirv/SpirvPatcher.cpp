#include "CommonHeader.h"
#include "SpirvPatcher.h"
#include "SpirvParser.h"
#include "ShaderConductor.hpp"

namespace FW
{
	void SpvPatcher::SetSpvContext(const TArray<uint32>& InSpvCode, SpvMetaContext* InMetaContext)
	{
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

	void SpvPatcher::AddAnnotation(const SpvInstruction* Inst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		TArray<uint32> InstBin = Inst->ToBinary();
		SpvCode.Insert(InstBin, Context.Sections[SpvSectionKind::Annotation].EndOffset);
		UpdateSection(SpvSectionKind::Annotation, InstBin.Num());
		Inst->Accept(MetaVisitor.Get());
	}

	void SpvPatcher::AddType(const SpvInstruction* Inst)
	{
		SpvMetaContext& Context = MetaVisitor->GetContext();
		TArray<uint32> InstBin = Inst->ToBinary();
		SpvCode.Insert(InstBin, Context.Sections[SpvSectionKind::Type].EndOffset);
		UpdateSection(SpvSectionKind::Type, InstBin.Num());
		Inst->Accept(MetaVisitor.Get());
	}

	void SpvPatcher::AddConstant(const SpvInstruction* Inst)
	{
		AddType(Inst);
	}

	void SpvPatcher::AddGlobalVariable(const SpvInstruction* Inst)
	{
		AddType(Inst);
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
}
