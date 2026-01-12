#pragma once

#include "GpuShader.h"
#include "Common/Path/PathHelper.h"
#include "magic_enum.hpp"
#include <Serialization/JsonSerializer.h>

THIRD_PARTY_INCLUDES_START
#pragma push_macro("check")
#undef check
#include "glslang/Public/ShaderLang.h"
#include "glslang/Include/intermediate.h"
#include "glslang/MachineIndependent/localintermediate.h"
#include "shaderc/shaderc.hpp"
#pragma pop_macro("check")
THIRD_PARTY_INCLUDES_END

namespace GLSL
{
	struct ShaderBuiltinParameter
	{
		FString Type;
		FString Name;
		FString Desc;
		FString IO;
	};

	struct ShaderBuiltinSignature
	{
		FString ReturnType;
		TArray<ShaderBuiltinParameter> Parameters;
	};

	struct ShaderBuiltinItem
	{
		FString Label;
		TArray<FString> Stages;
		FString Desc;
		TArray<ShaderBuiltinSignature> Signatures;
		FString Url;
	};

	struct ShaderBuiltinData
	{
		TArray<ShaderBuiltinItem> Keywords;
		TArray<ShaderBuiltinItem> Types;
		TArray<ShaderBuiltinItem> Functions;
		bool bLoaded = false;
	};

	inline ShaderBuiltinData& GetBuiltinData()
	{
		static ShaderBuiltinData Data;
		if (!Data.bLoaded)
		{
			FString JsonPath = FW::PathHelper::ShaderDir() / TEXT("GLSL.json");
			FString JsonContent;
			if (FFileHelper::LoadFileToString(JsonContent, *JsonPath))
			{
				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
				if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
				{
					auto ParseItems = [](const TSharedPtr<FJsonObject>& Obj, const FString& FieldName, TArray<ShaderBuiltinItem>& OutItems) {
						const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
						if (Obj->TryGetArrayField(FieldName, ItemsArray))
						{
							for (const auto& ItemValue : *ItemsArray)
							{
								const TSharedPtr<FJsonObject>* ItemObj;
								if (ItemValue->TryGetObject(ItemObj))
								{
									ShaderBuiltinItem Item;
									(*ItemObj)->TryGetStringField(TEXT("label"), Item.Label);
									(*ItemObj)->TryGetStringField(TEXT("desc"), Item.Desc);
									(*ItemObj)->TryGetStringField(TEXT("url"), Item.Url);
									const TArray<TSharedPtr<FJsonValue>>* StagesArray;
									if ((*ItemObj)->TryGetArrayField(TEXT("stages"), StagesArray))
									{
										for (const auto& StageValue : *StagesArray)
										{
											FString Stage;
											if (StageValue->TryGetString(Stage))
											{
												Item.Stages.Add(Stage);
											}
										}
									}
									const TArray<TSharedPtr<FJsonValue>>* SignaturesArray;
									if ((*ItemObj)->TryGetArrayField(TEXT("signatures"), SignaturesArray))
									{
										for (const auto& SigValue : *SignaturesArray)
										{
											const TSharedPtr<FJsonObject>* SigObj;
											if (SigValue->TryGetObject(SigObj))
											{
												ShaderBuiltinSignature Sig;
												(*SigObj)->TryGetStringField(TEXT("returnType"), Sig.ReturnType);
												const TArray<TSharedPtr<FJsonValue>>* ParamsArray;
												if ((*SigObj)->TryGetArrayField(TEXT("parameters"), ParamsArray))
												{
													for (const auto& ParamValue : *ParamsArray)
													{
														const TSharedPtr<FJsonObject>* ParamObj;
														if (ParamValue->TryGetObject(ParamObj))
														{
															ShaderBuiltinParameter Param;
															(*ParamObj)->TryGetStringField(TEXT("type"), Param.Type);
															(*ParamObj)->TryGetStringField(TEXT("name"), Param.Name);
															(*ParamObj)->TryGetStringField(TEXT("desc"), Param.Desc);
															(*ParamObj)->TryGetStringField(TEXT("io"), Param.IO);
															Sig.Parameters.Add(MoveTemp(Param));
														}
													}
												}
												Item.Signatures.Add(MoveTemp(Sig));
											}
										}
									}
									OutItems.Add(MoveTemp(Item));
								}
							}
						}
						};

					ParseItems(JsonObject, TEXT("Keywords"), Data.Keywords);
					ParseItems(JsonObject, TEXT("Types"), Data.Types);
					ParseItems(JsonObject, TEXT("Functions"), Data.Functions);
					Data.bLoaded = true;
				}
			}
		}
		return Data;
	}

	inline bool IsStageMatch(const TArray<FString>& ItemStages, FW::ShaderType Stage)
	{
		FString StageStr;
		switch (Stage)
		{
		case FW::ShaderType::VertexShader:  StageStr = TEXT("VS"); break;
		case FW::ShaderType::PixelShader:   StageStr = TEXT("PS"); break;
		case FW::ShaderType::ComputeShader: StageStr = TEXT("CS"); break;
		default: return false;
		}
		return ItemStages.Contains(StageStr);
	}

	inline TArray<FString> GetKeyWords(FW::ShaderType Stage) {
		TArray<FString> Result;
		const ShaderBuiltinData& Data = GetBuiltinData();
		for (const auto& Item : Data.Keywords)
		{
			if (IsStageMatch(Item.Stages, Stage))
			{
				Result.Add(Item.Label);
			}
		}
		return Result;
	};

	inline TArray<FString> GetBuiltinTypes(FW::ShaderType Stage) {
		TArray<FString> Result;
		const ShaderBuiltinData& Data = GetBuiltinData();
		for (const auto& Item : Data.Types)
		{
			if (IsStageMatch(Item.Stages, Stage))
			{
				Result.Add(Item.Label);
			}
		}
		return Result;
	};

	inline TArray<FString> GetBuiltinFuncs(FW::ShaderType Stage) {
		TArray<FString> Result;
		const ShaderBuiltinData& Data = GetBuiltinData();
		for (const auto& Item : Data.Functions)
		{
			if (IsStageMatch(Item.Stages, Stage))
			{
				Result.Add(Item.Label);
			}
		}
		return Result;
	};

	inline const ShaderBuiltinItem* FindBuiltinFunc(const FString& FuncName, FW::ShaderType Stage) {
		const ShaderBuiltinData& Data = GetBuiltinData();
		for (const auto& Item : Data.Functions)
		{
			if (Item.Label == FuncName && IsStageMatch(Item.Stages, Stage))
			{
				return &Item;
			}
		}
		return nullptr;
	};

	inline const ShaderBuiltinItem* FindBuiltinType(const FString& TypeName, FW::ShaderType Stage) {
		const ShaderBuiltinData& Data = GetBuiltinData();
		for (const auto& Item : Data.Types)
		{
			if (Item.Label == TypeName && IsStageMatch(Item.Stages, Stage))
			{
				return &Item;
			}
		}
		return nullptr;
	};

	inline const ShaderBuiltinItem* FindKeyword(const FString& Keyword, FW::ShaderType Stage) {
		const ShaderBuiltinData& Data = GetBuiltinData();
		for (const auto& Item : Data.Keywords)
		{
			if (Item.Label == Keyword && IsStageMatch(Item.Stages, Stage))
			{
				return &Item;
			}
		}
		return nullptr;
	};
}


namespace FW
{
	inline TArray<ShaderDiagnosticInfo> ParseDiagnosticInfoFromGlslang(const FString& ShaderName, FStringView GlslDiagnosticInfo)
	{
		TArray<ShaderDiagnosticInfo> Ret;
		int32 LineInfoFirstPos = GlslDiagnosticInfo.Find(ShaderName + ":");
		while (LineInfoFirstPos != INDEX_NONE)
		{
			ShaderDiagnosticInfo DiagnosticInfo;

			int32 LineInfoLastPos = GlslDiagnosticInfo.Find(TEXT("\n"), LineInfoFirstPos);
			FStringView LineStringView{ GlslDiagnosticInfo.GetData() + LineInfoFirstPos, LineInfoLastPos - LineInfoFirstPos };

			int32 LineInfoFirstColonPos = ShaderName.Len();
			int32 Pos2 = LineStringView.Find(TEXT(":"), LineInfoFirstColonPos + 1);
			DiagnosticInfo.Row = FCString::Atoi(LineStringView.SubStr(LineInfoFirstColonPos + 1, Pos2 - LineInfoFirstColonPos - 1).GetData());

			int32 ErrorPos = LineStringView.Find(TEXT("error: "), Pos2);
			if (ErrorPos != INDEX_NONE)
			{
				int32 ErrorInfoEnd = LineStringView.Len();
				DiagnosticInfo.Error = LineStringView.SubStr(ErrorPos + 7, ErrorInfoEnd - ErrorPos - 7);
				Ret.Add(MoveTemp(DiagnosticInfo));
			}
			else
			{
				int32 WarnPos = LineStringView.Find(TEXT("warning: "), Pos2);
				int32 WarnInfoEnd = LineStringView.Len();
				DiagnosticInfo.Warn = LineStringView.SubStr(WarnPos + 9, WarnInfoEnd - WarnPos - 9);
				Ret.Add(MoveTemp(DiagnosticInfo));
			}

			LineInfoFirstPos = GlslDiagnosticInfo.Find(ShaderName + ":", LineInfoLastPos);
		}
		return Ret;
	}

	inline shaderc_shader_kind MapShadercKind(ShaderType InType)
	{
		switch (InType)
		{
		case ShaderType::VertexShader:   return shaderc_shader_kind::shaderc_vertex_shader;
		case ShaderType::PixelShader:    return shaderc_shader_kind::shaderc_fragment_shader;
		case ShaderType::ComputeShader:  return shaderc_shader_kind::shaderc_compute_shader;
		default:
			AUX::Unreachable();
		}
	}

	class ShadercIncludeHandler : public shaderc::CompileOptions::IncluderInterface
	{
		struct IncludeData {
			std::string Source;
			std::string Content;
		};
	public:
		ShadercIncludeHandler(TRefCountPtr<GpuShader> InShader) : Shader(MoveTemp(InShader)) {}
		shaderc_include_result* GetInclude(
			const char* requested_source,
			shaderc_include_type type,
			const char* requesting_source,
			size_t include_depth) override
		{
			auto data = new IncludeData;
			shaderc_include_result* Result = new shaderc_include_result;
			Result->user_data = data;
			for (const FString& IncludeDir : Shader->GetIncludeDirs())
			{
				FString IncludedFile = FPaths::Combine(IncludeDir, ANSI_TO_TCHAR(requested_source));
				if (IFileManager::Get().FileExists(*IncludedFile))
				{
					FString ShaderText;
					FFileHelper::LoadFileToString(ShaderText, *IncludedFile);
					ShaderText = GpuShaderPreProcessor{ ShaderText, Shader->GetShaderLanguage() }
						.ReplacePrintStringLiteral()
						.Finalize();
					data->Source = requested_source;
					data->Content = { TCHAR_TO_UTF8(*ShaderText) };
					break;
				}
			}

			Result->source_name = data->Source.c_str();
			Result->source_name_length = data->Source.size();
			Result->content = data->Content.c_str();
			Result->content_length = data->Content.size();

			return Result;
		}

		void ReleaseInclude(shaderc_include_result* data) override
		{
			delete static_cast<IncludeData*>(data->user_data);
			delete data;
		}

	private:
		TRefCountPtr<GpuShader> Shader;
	};

	inline void ParseExtraArgs(const TArray<FString>& ExtraArgs, shaderc::CompileOptions& OutOptions)
	{
		for (int32 i = 0; i < ExtraArgs.Num(); ++i)
		{
			// Parse -D macro definitions: -D is a separate arg, macro definition is the next arg
			if (ExtraArgs[i] == TEXT("-D") && i + 1 < ExtraArgs.Num())
			{
				FString MacroDef = ExtraArgs[i + 1];
				++i; // Skip the macro definition arg in next iteration

				int32 EqualsPos = MacroDef.Find(TEXT("="));
				if (EqualsPos != INDEX_NONE)
				{
					// NAME=VALUE format
					FString MacroName = MacroDef.Left(EqualsPos);
					FString MacroValue = MacroDef.Mid(EqualsPos + 1);

					if (!MacroName.IsEmpty())
					{
						auto NameUTF8 = StringCast<UTF8CHAR>(*MacroName);
						auto ValueUTF8 = StringCast<UTF8CHAR>(*MacroValue);
						OutOptions.AddMacroDefinition(
							(const char*)NameUTF8.Get(), NameUTF8.Length(),
							(const char*)ValueUTF8.Get(), ValueUTF8.Length());
					}
				}
				else
				{
					// NAME format (valueless macro)
					if (!MacroDef.IsEmpty())
					{
						auto NameUTF8 = StringCast<UTF8CHAR>(*MacroDef);
						OutOptions.AddMacroDefinition(
							(const char*)NameUTF8.Get(), NameUTF8.Length(),
							nullptr, 0u);
					}
				}
			}
		}
	}

	inline auto CompileGlsl(GpuShader* InShader, const TArray<FString>& ExtraArgs)
	{
		static shaderc::Compiler GlslCompiler;
		shaderc::CompileOptions Options;
		ParseExtraArgs(ExtraArgs, Options);
		if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
		{
			Options.SetGenerateDebugInfo();
			Options.SetOptimizationLevel(shaderc_optimization_level_zero);
			Options.SetNonSemanticShaderDebugSource();
		}
		Options.SetIncluder(std::make_unique<ShadercIncludeHandler>(InShader));
		auto Result = GlslCompiler.CompileGlslToSpv(TCHAR_TO_UTF8(*InShader->GetProcessedSourceText()),
			MapShadercKind(InShader->GetShaderType()), TCHAR_TO_UTF8(*InShader->GetShaderName()), Options);

		return Result;
	}

	class GlslDumpTraverser : public glslang::TIntermTraverser
	{
	public: 
		GlslDumpTraverser(FString InDumpFilePath)
			: DumpFilePath(MoveTemp(InDumpFilePath))
		{}

		void SaveDump()
		{
			if (DumpFilePath.IsEmpty())
			{
				return;
			}

			const FString Dir = FPaths::GetPath(DumpFilePath);
			IFileManager::Get().MakeDirectory(*Dir, /*Tree*/true);
			FFileHelper::SaveStringToFile(DumpText, *DumpFilePath);
		}

		bool visitBranch(glslang::TVisit, glslang::TIntermBranch* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Branch] %u:%u file=%s"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName));
			return true;
		}

		bool visitSwitch(glslang::TVisit, glslang::TIntermSwitch* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Switch] %u:%u file=%s"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName));
			return true;
		}

		bool visitLoop(glslang::TVisit, glslang::TIntermLoop* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Loop] %u:%u file=%s"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName));
			return true;
		}

		bool visitSelection(glslang::TVisit, glslang::TIntermSelection* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Selection] %u:%u file=%s"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName));
			return true;
		}

		void visitConstantUnion(glslang::TIntermConstantUnion* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			FString Type, Value;
			if (Node->getType().isStruct())
			{
				Type = UTF8_TO_TCHAR(Node->getType().getTypeName().c_str());
			}
			else
			{
				Type = UTF8_TO_TCHAR(Node->getType().getCompleteString(true).c_str());
			}
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Constant] %u:%u file=%s type=%s"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName, *Type));
		}

		void visitSymbol(glslang::TIntermSymbol* Symbol) override
		{
			const glslang::TSourceLoc Loc = Symbol->getLoc();
			const FString Name = UTF8_TO_TCHAR(Symbol->getName().c_str());
			const FString Mangled = UTF8_TO_TCHAR(Symbol->getMangledName().c_str());
			FString Type;
			if (Symbol->getType().isStruct())
			{
				Type = UTF8_TO_TCHAR(Symbol->getType().getTypeName().c_str());
			}
			bool FromMacro = Loc.flags & glslang::TSourceLoc::FromMacroExpansion;
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Symbol] %u:%u file=%s name=%s mangled=%s type=%s fromMacro=%d"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName,
				*Name, *Mangled, *Type, FromMacro));
		}

		bool visitVariableDecl(glslang::TVisit, glslang::TIntermVariableDecl* VarDecl) override
		{ 
			const glslang::TIntermSymbol* DeclSymbol = VarDecl->getDeclSymbol();
			if (DeclSymbol)
			{
				const glslang::TSourceLoc& Loc = DeclSymbol->getLoc();
				const FString Name = UTF8_TO_TCHAR(DeclSymbol->getName().c_str());
				FString Type;
				if (DeclSymbol->getType().isStruct())
				{
					Type = UTF8_TO_TCHAR(DeclSymbol->getType().getTypeName().c_str());
				}
				bool FromMacro = Loc.flags & glslang::TSourceLoc::FromMacroExpansion;
				const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
				AppendDumpLine(FString::Printf(TEXT("%s[VarDecl] %u:%u file=%s name=%s type=%s fromMacro=%d"),
					*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName,
					*Name, *Type, FromMacro));
			}
			return true; 
		}

		bool visitUnary(glslang::TVisit, glslang::TIntermUnary* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			const auto OpName = magic_enum::enum_name(Node->getOp());
			const FString OpStr = OpName.empty() ? FString::FromInt(int32(Node->getOp())) : ANSI_TO_TCHAR(std::string(OpName).c_str());
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Unary] %u:%u file=%s op=%s(%d)"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName, *OpStr, int32(Node->getOp())));
			return true;
		}

		bool visitBinary(glslang::TVisit, glslang::TIntermBinary* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			const auto OpName = magic_enum::enum_name(Node->getOp());
			const FString OpStr = OpName.empty() ? FString::FromInt(int32(Node->getOp())) : ANSI_TO_TCHAR(std::string(OpName).c_str());
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Binary] %u:%u file=%s op=%s(%d)"),
				*GetIndent(), uint32(Loc.line), uint32(Loc.column), *LocName, *OpStr, int32(Node->getOp())));
			return true;
		}

		bool visitAggregate(glslang::TVisit, glslang::TIntermAggregate* Node) override
		{
			const glslang::TSourceLoc Loc = Node->getLoc();
			const glslang::TSourceLoc StartLoc = Node->getStartLoc();
			const glslang::TSourceLoc EndLoc = Node->getEndLoc();
			const FString Name = UTF8_TO_TCHAR(Node->getName().c_str());
			FString Type;
			if (Node->getType().isStruct())
			{
				Type = UTF8_TO_TCHAR(Node->getType().getTypeName().c_str());
			}
			else
			{
				Type = UTF8_TO_TCHAR(Node->getType().getCompleteString(true).c_str());
			}
			const auto OpName = magic_enum::enum_name(Node->getOp());
			const FString OpStr = OpName.empty() ? FString::FromInt(int32(Node->getOp())) : ANSI_TO_TCHAR(std::string(OpName).c_str());
			const FString LocName = Loc.name ? UTF8_TO_TCHAR(Loc.getFilenameStr()) : TEXT("<main>");
			AppendDumpLine(FString::Printf(TEXT("%s[Aggregate] %u:%u-%u:%u-%u:%u file=%s op=%s(%d) name=%s type=%s seq=%d"),
				*GetIndent(), uint32(StartLoc.line), uint32(StartLoc.column), uint32(Loc.line), uint32(Loc.column), uint32(EndLoc.line), uint32(EndLoc.column),
				*LocName, *OpStr, int32(Node->getOp()),
				*Name, *Type, int32(Node->getSequence().size())));
			return true;
		}

	private:
		FString GetIndent() const
		{
			FString Indent;
			for (int32 i = 0; i < depth; ++i)
			{
				Indent += TEXT("  ");
			}
			return Indent;
		}

		void AppendDumpLine(const FString& Line)
		{
			DumpText += Line;
			DumpText += TEXT("\n");
		}

		FString DumpFilePath;
		FString DumpText;
	};

	struct GlslSymbol
	{
		FString Id;
		ShaderTokenType Kind;
		FString Name;
		FString File;
		Vector2i Location;
		ShaderScope Scope;
		FString TypeName;
		bool bIsStruct = false;

		bool operator==(const GlslSymbol& Other) const { return Id == Other.Id && File == Other.File && Location == Other.Location; };
		friend uint32 GetTypeHash(const GlslSymbol& Key)
		{
			uint32 Hash = ::GetTypeHash(Key.Id);
			Hash = HashCombine(Hash, GetTypeHash(Key.File));
			Hash = HashCombine(Hash, GetTypeHash(Key.Location));
			return Hash;
		}
	};

	struct GlslSymbolRef
	{
		FString Name;
		FString File;
		Vector2i Location;

		bool operator==(const GlslSymbolRef&) const = default;
	};

	struct GlslStructMember
	{
		FString Name;
		FString TypeName;
	};

	struct GlslContext
	{
		TArray<ShaderFunc>& Funcs;
		TArray<ShaderScope>& GuideLineScopes;
		TMultiMap<GlslSymbol, GlslSymbolRef> SymbolRefs;
		TArray<GlslSymbol> SymbolTable;
		TArray<glslang::TShader::MacroDefinition> MacroDefinitions;
		TArray<glslang::TShader::MacroExpansion> MacroExpansions;
		TMap<FString, TArray<GlslStructMember>> StructMembers; // TypeName -> Members
	};

	inline bool IsPrintMacro(const std::string& Name)
	{
		return Name == "PrintAtMouse" || Name == "Print" || Name == "Assert" || Name == "AssertFormat";
	}

	inline FString GetTypeName(const glslang::TType& type) {
		FString TypeName;
		if (type.isStruct()) {
			TypeName = UTF8_TO_TCHAR(type.getTypeName().c_str());
		}
		else if (type.isVector() || type.isMatrix() || type.isArray()) {
			TypeName = UTF8_TO_TCHAR(type.getCompleteString(true, false, false, true).c_str());
		}
		else
		{
			TypeName = UTF8_TO_TCHAR(type.getBasicTypeString().c_str());
		}

		FString Qualifier;
		switch (type.getQualifier().storage) {
		case glslang::EvqConst:          Qualifier = "const";          break;
		case glslang::EvqVaryingIn:      Qualifier = "in";             break;
		case glslang::EvqVaryingOut:     Qualifier = "out";            break;
		case glslang::EvqUniform:        Qualifier = "uniform";        break;
		case glslang::EvqBuffer:         Qualifier = "buffer";         break;
		case glslang::EvqShared:         Qualifier = "shared";         break;
		case glslang::EvqIn:             Qualifier = "in";             break;
		case glslang::EvqOut:            Qualifier = "out";            break;
		case glslang::EvqInOut:          Qualifier = "inout";          break;
		default: break;
		}

		return Qualifier + " " + TypeName.TrimStartAndEnd();
	}

	inline TMap<glslang::TOperator, FString> BuiltInOps = {
		{glslang::EOpRadians,          "radians"},
		{glslang::EOpDegrees,          "degrees"},
		{glslang::EOpSin,              "sin"},
		{glslang::EOpCos,              "cos"},
		{glslang::EOpTan,              "tan"},
		{glslang::EOpAsin,             "asin"},
		{glslang::EOpAcos,             "acos"},
		{glslang::EOpAtan,             "atan"},
		{glslang::EOpAtan,             "atan"},
		{glslang::EOpPow,              "pow"},
		{glslang::EOpExp,              "exp"},
		{glslang::EOpLog,              "log"},
		{glslang::EOpExp2,             "exp2"},
		{glslang::EOpLog2,             "log2"},
		{glslang::EOpSqrt,             "sqrt"},
		{glslang::EOpInverseSqrt,      "inversesqrt"},
		{glslang::EOpAbs,              "abs"},
		{glslang::EOpSign,             "sign"},
		{glslang::EOpFloor,            "floor"},
		{glslang::EOpCeil,             "ceil"},
		{glslang::EOpFract,            "fract"},
		{glslang::EOpMod,              "mod"},
		{glslang::EOpMin,              "min"},
		{glslang::EOpMax,              "max"},
		{glslang::EOpClamp,            "clamp"},
		{glslang::EOpMix,              "mix"},
		{glslang::EOpStep,             "step"},
		{glslang::EOpSmoothStep,       "smoothstep"},
		{glslang::EOpNormalize,        "normalize"},
		{glslang::EOpFaceForward,      "faceforward"},
		{glslang::EOpReflect,          "reflect"},
		{glslang::EOpRefract,          "refract"},
		{glslang::EOpLength,           "length"},
		{glslang::EOpDistance,         "distance"},
		{glslang::EOpDot,              "dot"},
		{glslang::EOpCross,            "cross"},
		{glslang::EOpLessThan,         "lessThan"},
		{glslang::EOpLessThanEqual,    "lessThanEqual"},
		{glslang::EOpGreaterThan,      "greaterThan"},
		{glslang::EOpGreaterThanEqual, "greaterThanEqual"},
		{glslang::EOpVectorEqual,      "equal"},
		{glslang::EOpVectorNotEqual,   "notEqual"},
		{glslang::EOpAny,              "any"},
		{glslang::EOpAll,              "all"},
		{glslang::EOpVectorLogicalNot, "not"},
		{glslang::EOpSinh,             "sinh"},
		{glslang::EOpCosh,             "cosh"},
		{glslang::EOpTanh,             "tanh"},
		{glslang::EOpAsinh,            "asinh"},
		{glslang::EOpAcosh,            "acosh"},
		{glslang::EOpAtanh,            "atanh"},
		{glslang::EOpAbs,              "abs"},
		{glslang::EOpSign,             "sign"},
		{glslang::EOpTrunc,            "trunc"},
		{glslang::EOpRound,            "round"},
		{glslang::EOpRoundEven,        "roundEven"},
		{glslang::EOpModf,             "modf"},
		{glslang::EOpMin,              "min"},
		{glslang::EOpMax,              "max"},
		{glslang::EOpClamp,            "clamp"},
		{glslang::EOpMix,              "mix"},
		{glslang::EOpIsInf,            "isinf"},
		{glslang::EOpIsNan,            "isnan"},
		{glslang::EOpLessThan,         "lessThan"},
		{glslang::EOpLessThanEqual,    "lessThanEqual"},
		{glslang::EOpGreaterThan,      "greaterThan"},
		{glslang::EOpGreaterThanEqual, "greaterThanEqual"},
		{glslang::EOpVectorEqual,      "equal"},
		{glslang::EOpVectorNotEqual,   "notEqual"},
		{glslang::EOpAtomicAdd,        "atomicAdd"},
		{glslang::EOpAtomicMin,        "atomicMin"},
		{glslang::EOpAtomicMax,        "atomicMax"},
		{glslang::EOpAtomicAnd,        "atomicAnd"},
		{glslang::EOpAtomicOr,         "atomicOr"},
		{glslang::EOpAtomicXor,        "atomicXor"},
		{glslang::EOpAtomicExchange,   "atomicExchange"},
		{glslang::EOpAtomicCompSwap,   "atomicCompSwap"},
		{glslang::EOpMix,              "mix"},
		{glslang::EOpMix,              "mix"},

		{glslang::EOpDPdx,             "dFdx"},
		{glslang::EOpDPdy,             "dFdy"},
		{glslang::EOpFwidth,           "fwidth"},
	};

	class GlslVisitor : public glslang::TIntermTraverser
	{
	public:
		GlslVisitor(GlslContext& InContext, int32 FileEndLine)
			: glslang::TIntermTraverser(true, false, true) // preVisit, inVisit, postVisit
			, Context(InContext)
		{
			ScopeStack.Push({ Vector2i(1, 1), Vector2i(FileEndLine, 1) });
			for (const auto& Def : Context.MacroDefinitions)
			{
				Context.SymbolTable.AddUnique({
					.Id = UTF8_TO_TCHAR(Def.name.c_str()),
					.Kind = ShaderTokenType::Preprocess,
					.Name = UTF8_TO_TCHAR(Def.name.c_str()),
					.File = UTF8_TO_TCHAR(Def.define.file.c_str()),
					.Location = {Def.define.line, Def.define.column},
					.Scope = ScopeStack.Top()
				});
				Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
					.Name = UTF8_TO_TCHAR(Def.name.c_str()),
					.File = UTF8_TO_TCHAR(Def.define.file.c_str()),
					.Location = {Def.define.line,Def.define.column}
				});
			}
			for (const auto& Exp : Context.MacroExpansions)
			{
				if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == UTF8_TO_TCHAR(Exp.name.c_str()); }))
				{
					Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
						.Name = UTF8_TO_TCHAR(Exp.name.c_str()),
						.File = UTF8_TO_TCHAR(Exp.call.file.c_str()),
						.Location = {Exp.call.line, Exp.call.column}
					});
				}
			}
		}

		void visitConstantUnion(glslang::TIntermConstantUnion* Node) override
		{
			if (Node->getType().isStruct())
			{
				FString TypeName = GetTypeName(Node->getType());
				auto TypeLoc = Node->getType().loc;
				if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == TypeName; }))
				{
					Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
						.Name = TypeName,
						.File = TypeLoc.getFilenameStr(),
						.Location = {TypeLoc.line, TypeLoc.column}
					});
				}
			}
		}

		void visitSymbol(glslang::TIntermSymbol* Node) override
		{
			FString NodeName = UTF8_TO_TCHAR(Node->getName().c_str());
			if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == LexToString(Node->getId()); }))
			{
				bool FromMacro = Node->getLoc().flags & glslang::TSourceLoc::FromMacroExpansion;
				Vector2i Location = { Node->getLoc().line, Node->getLoc().column };
				if (FromMacro)
				{
					const auto& Exp = Context.MacroExpansions[Node->getLoc().macroExpansionId];
					if (IsPrintMacro(Exp.name))
					{
						Location.y -= 1;
					}
				}
				Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
					.Name = NodeName,
					.File = Node->getLoc().getFilenameStr(),
					.Location = Location
				});
			}
		}

		bool visitVariableDecl(glslang::TVisit Visit, glslang::TIntermVariableDecl* Node) override
		{
			if (Visit == glslang::EvPostVisit) return true;
			const glslang::TIntermSymbol* DeclSymbol = Node->getDeclSymbol();
			FString SymbolName = UTF8_TO_TCHAR(DeclSymbol->getName().c_str());
			//uniform buffer block without instance name
			if(SymbolName == "anon@0" && DeclSymbol->getType().isStruct())
			{
				for (const auto& Member : *DeclSymbol->getType().getStruct())
				{
					FString MemberName = UTF8_TO_TCHAR(Member.type->getFieldName().c_str());
					bool bMemberIsStruct = Member.type->isStruct();
					FString MemberTypeName = GetTypeName(*Member.type);

					Context.SymbolTable.AddUnique({
						.Id = MemberName,
						.Kind = ShaderTokenType::Var,
						.Name = MemberName,
						.File = Member.loc.getFilenameStr(),
						.Location = {Member.loc.line, Member.loc.column},
						.Scope = ScopeStack.Top(),
						.TypeName = MemberTypeName,
						.bIsStruct = bMemberIsStruct
					});
					Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
						.Name = MemberName,
						.File = Member.loc.getFilenameStr(),
						.Location = {Member.loc.line, Member.loc.column}
					});
				}
			}
			else
			{
				bool bIsStruct = DeclSymbol->getType().isStruct();
				FString TypeName = GetTypeName(DeclSymbol->getType());

				Context.SymbolTable.AddUnique({
					.Id = LexToString(DeclSymbol->getId()),
					.Kind = depth > 2 ? ShaderTokenType::LocalVar : ShaderTokenType::Var,
					.Name = SymbolName,
					.File = DeclSymbol->getLoc().getFilenameStr(),
					.Location = {DeclSymbol->getLoc().line, DeclSymbol->getLoc().column},
					.Scope = ScopeStack.Top(),
					.TypeName = TypeName,
					.bIsStruct = bIsStruct
				});
				Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
					.Name = SymbolName,
					.File = DeclSymbol->getLoc().getFilenameStr(),
					.Location = {DeclSymbol->getLoc().line, DeclSymbol->getLoc().column}
				});
			}

			if (DeclSymbol->getType().isStruct())
			{
				FString TypeName = UTF8_TO_TCHAR(DeclSymbol->getType().getTypeName().c_str());
				auto TypeLoc = DeclSymbol->getType().loc;
				if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == TypeName; }))
				{
					Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
						.Name = TypeName,
						.File = TypeLoc.getFilenameStr(),
						.Location = {TypeLoc.line, TypeLoc.column}
					});
				}
			}

			return true;
		}

		bool visitSelection(glslang::TVisit Visit, glslang::TIntermSelection* Node) override
		{
			if (Visit == glslang::EvPostVisit) return true;
			const auto& StartLoc = Node->getLoc();
			if (Node->getFalseBlock() && Node->getFalseBlock()->getAsAggregate())
			{
				const auto& EndLoc = Node->getFalseBlock()->getAsAggregate()->getEndLoc();
				Context.GuideLineScopes.Emplace(Vector2i(StartLoc.line, StartLoc.column),
					Vector2i(EndLoc.line, EndLoc.column));
			}
			else if ( Node->getTrueBlock() && Node->getTrueBlock()->getAsAggregate())
			{
				const auto& EndLoc = Node->getTrueBlock()->getAsAggregate()->getEndLoc();
				if (EndLoc.line > StartLoc.line)
				{
					Context.GuideLineScopes.Emplace(Vector2i(StartLoc.line, StartLoc.column),
						Vector2i(EndLoc.line, EndLoc.column));
				}
			}
			return true;
		}

		bool visitBinary(glslang::TVisit Visit, glslang::TIntermBinary* Node) override
		{
			if (Visit == glslang::EvPostVisit) return true;
			//anonymous uniform buffer block member access
			if (Node->getOp() == glslang::EOpIndexDirectStruct && Node->getLeft()->getAsSymbolNode() && Node->getRight()->getAsConstantUnion())
			{
				const glslang::TIntermSymbol* LeftSymbol = Node->getLeft()->getAsSymbolNode();
				FString SymbolName = UTF8_TO_TCHAR(LeftSymbol->getName().c_str());
				if (SymbolName == "anon@0" && LeftSymbol->getType().isStruct())
				{
					int32 MemberIndex = Node->getRight()->getAsConstantUnion()->getConstArray()[0].getIConst();
					FString MemberName = UTF8_TO_TCHAR((*LeftSymbol->getType().getStruct())[MemberIndex].type->getFieldName().c_str());
					if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == MemberName; }))
					{
						Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
							.Name = MemberName,
							.File = LeftSymbol->getLoc().getFilenameStr(),
							.Location = {LeftSymbol->getLoc().line, LeftSymbol->getLoc().column}
						});
					}
					return false;
				}
				
			}
			return true;
		} 

		bool visitUnary(glslang::TVisit Visit, glslang::TIntermUnary* Node) override
		{
			if (Visit == glslang::EvPostVisit) return true;

			if (Node->getOp() == glslang::EOpDeclare)
			{
				const glslang::TIntermSymbol* Symbol = Node->getOperand()->getAsSymbolNode();
				if (Symbol->getType().isStruct()) 
				{
					const auto& StartLoc = Symbol->getType().startLoc;
					const auto& EndLoc = Symbol->getType().endLoc;
					Context.GuideLineScopes.Emplace(Vector2i(StartLoc.line, StartLoc.column),
						Vector2i(EndLoc.line, EndLoc.column));

					FString TypeName = UTF8_TO_TCHAR(Symbol->getType().getTypeName().c_str());
					Context.SymbolTable.AddUnique({
						.Id = TypeName,
						.Kind = ShaderTokenType::Type,
						.Name = TypeName,
						.File = Node->getLoc().getFilenameStr(),
						.Location = {Node->getLoc().line, Node->getLoc().column},
						.Scope = ScopeStack.Top(),
					});
					Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
						.Name = TypeName,
						.File = Node->getLoc().getFilenameStr(),
						.Location = {Node->getLoc().line, Node->getLoc().column}
					});

					// Collect struct members
					if (const glslang::TTypeList* StructMembers = Symbol->getType().getStruct())
					{
						TArray<GlslStructMember>& Members = Context.StructMembers.FindOrAdd(TypeName);
						for (const auto& Member : *StructMembers)
						{
							FString MemberName = UTF8_TO_TCHAR(Member.type->getFieldName().c_str());
							FString MemberTypeName;
							if (Member.type->isStruct())
							{
								MemberTypeName = UTF8_TO_TCHAR(Member.type->getTypeName().c_str());
							}
							else
							{
								MemberTypeName = UTF8_TO_TCHAR(Member.type->getBasicTypeString().c_str());
							}
							Members.Add({ MemberName, MemberTypeName });
						}
					}
					return false;
				}
			}
			else
			{
				HandleBuiltInFunction(Node);
			}
			return true;
		}

		bool visitAggregate(glslang::TVisit Visit, glslang::TIntermAggregate* Node) override
		{
			const glslang::TOperator Op = Node->getOp();

			// Post-visit: pop scope when exiting function or scope block
			if (Visit == glslang::EvPostVisit)
			{
				if (Op == glslang::EOpFunction || Op == glslang::EOpScope)
				{
					if (ScopeStack.Num() > 1)
					{
						ScopeStack.Pop();
					}
				}
				return true;
			}

			// Pre-visit
			if (Op == glslang::EOpFunction) 
			{
				Vector2i StartLoc = { Node->getStartLoc().line, Node->getStartLoc().column };
				Vector2i EndLoc = { Node->getEndLoc().line, Node->getEndLoc().column };
				ShaderScope FuncScope = { StartLoc, EndLoc };
				Context.GuideLineScopes.Emplace(FuncScope);

				FString ParamsTypeName;
				TArray<ShaderParameter> Params;
				const glslang::TIntermSequence& ParamsSequence = Node->getSequence()[0]->getAsAggregate()->getSequence();
				for (int i = 0; i < ParamsSequence.size(); i++)
				{
					const TIntermNode* Child = ParamsSequence[i];
					if (const glslang::TIntermSymbol* ParamSymbol = Child->getAsSymbolNode())
					{
						FString NodeName = UTF8_TO_TCHAR(ParamSymbol->getName().c_str());
						bool bIsStruct = ParamSymbol->getType().isStruct();

						FString TypeName = GetTypeName(ParamSymbol->getType());
						ParamsTypeName += TypeName;
						if (i < ParamsSequence.size() - 1)
						{
							ParamsTypeName += ",";
						}

						// Extract ParamSemaFlag from TypeName
						ParamSemaFlag SemaFlag = ParamSemaFlag::None;
						FString TrimmedTypeName = TypeName.TrimStartAndEnd();
						if (TrimmedTypeName.StartsWith(TEXT("inout ")))
						{
							SemaFlag = ParamSemaFlag::Inout;
						}
						else if (TrimmedTypeName.StartsWith(TEXT("out ")))
						{
							SemaFlag = ParamSemaFlag::Out;
						}
						else if (TrimmedTypeName.StartsWith(TEXT("in ")))
						{
							SemaFlag = ParamSemaFlag::In;
						}

						Params.Add({.Name = NodeName, .SemaFlag = SemaFlag});

						//Append symbol if the parameter name exists
						if (!NodeName.IsEmpty())
						{
							Context.SymbolTable.AddUnique({
								.Id = LexToString(ParamSymbol->getId()),
								.Kind = ShaderTokenType::Param,
								.Name = NodeName,
								.File = ParamSymbol->getLoc().getFilenameStr(),
								.Location = {ParamSymbol->getLoc().line, ParamSymbol->getLoc().column},
								.Scope = ScopeStack.Top(),
								.TypeName = TypeName,
								.bIsStruct = bIsStruct
							});
							Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
								.Name = NodeName,
								.File = ParamSymbol->getLoc().getFilenameStr(),
								.Location = {ParamSymbol->getLoc().line, ParamSymbol->getLoc().column}
							});
						}

						if (bIsStruct)
						{
							auto TypeLoc = ParamSymbol->getType().loc;
							if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == TypeName; }))
							{
								Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
									.Name = TypeName,
									.File = TypeLoc.getFilenameStr(),
									.Location = {TypeLoc.line, TypeLoc.column}
								});
							}
						}
					}
				}

				FString ReturnTypeName = GetTypeName(Node->getType());
				FString FuncTypeName = ReturnTypeName + "(" + ParamsTypeName + ")";

				FString Name = UTF8_TO_TCHAR(Node->getName().c_str());
				FString FuncName, _;
				Name.Split(TEXT("("), &FuncName, &_);
				Context.Funcs.Add(ShaderFunc{
					.Name = FuncName,
					.FullName = Name,
					.Start = StartLoc,
					.End = EndLoc,
					.Params = Params
				});
				Context.SymbolTable.AddUnique({
					.Id = Name,
					.Kind = ShaderTokenType::Func,
					.Name = FuncName,
					.File = Node->getLoc().getFilenameStr(),
					.Location = {Node->getLoc().line, Node->getLoc().column},
					.Scope = ScopeStack.Top(),
					.TypeName = FuncTypeName
				});
				Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
					.Name = FuncName,
					.File = Node->getLoc().getFilenameStr(),
					.Location = {Node->getLoc().line, Node->getLoc().column}
				});
				ScopeStack.Push(FuncScope);
			}
			else if (Op == glslang::EOpParameters)
			{
				// Skip processing to avoid duplicate traversal (already processed in EOpFunction)
				return false;
			}
			else if (Op == glslang::EOpFunctionCall)
			{
				FString Name = UTF8_TO_TCHAR(Node->getName().c_str());
				FString FuncName, _;
				Name.Split(TEXT("("), &FuncName, &_);
				if (auto* Def  = Context.SymbolTable.FindByPredicate([&, this](auto&& Item){ return Item.Id == Name; }))
				{
					Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
						.Name = FuncName,
						.File = Node->getLoc().getFilenameStr(),
						.Location = {Node->getLoc().line, Node->getLoc().column}
					});
				}
			}
			else if (Op == glslang::EOpScope)
			{
				const auto& StartLoc = Node->getLoc();
				const auto& EndLoc = Node->getEndLoc();
				ShaderScope Scope = { Vector2i(StartLoc.line, StartLoc.column), Vector2i(EndLoc.line, EndLoc.column) };
				if (!getParentNode()->getAsSelectionNode() && !getParentNode()->getAsLoopNode())
				{
					Context.GuideLineScopes.Add(Scope);
				}
				ScopeStack.Push(Scope);
			}
			else if (Op == glslang::EOpConstructStruct)
			{
				if (Node->getType().isStruct())
				{
					FString TypeName = UTF8_TO_TCHAR(Node->getType().getTypeName().c_str());
					auto TypeLoc = Node->getType().loc;
					if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == TypeName; }))
					{
						Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
							.Name = TypeName,
							.File = TypeLoc.getFilenameStr(),
							.Location = {TypeLoc.line, TypeLoc.column}
						});
					}
				}
			}
			return true;
		}

		void HandleBuiltInFunction(glslang::TIntermUnary* Node)
		{
			if (BuiltInOps.Contains(Node->getOp()))
			{
				FString FuncName = BuiltInOps[Node->getOp()];
				FString ReturnTypeName = GetTypeName(Node->getType());
				FString ParamTypeName = GetTypeName(Node->getOperand()->getType());
				FString FuncTypeName = ReturnTypeName + "(" + ParamTypeName + ")";
				FString FullFuncName = ReturnTypeName + " " + FuncName + "(" + ParamTypeName + ")";

				Context.SymbolTable.AddUnique({
					.Id = FullFuncName,
					.Kind = ShaderTokenType::BuiltinFunc,
					.Name = FuncName,
					.TypeName = FuncTypeName
				});

				if (auto* Def = Context.SymbolTable.FindByPredicate([&, this](auto&& Item) { return Item.Id == FullFuncName; }))
				{
					Context.SymbolRefs.AddUnique(*Def, GlslSymbolRef{
						.Name = FuncName,
						.File = Node->getLoc().getFilenameStr(),
						.Location = {Node->getLoc().line, Node->getLoc().column}
					});
				}
			}
		}

	private:
		GlslContext& Context;
		TArray<ShaderScope> ScopeStack;
	};

	class FRAMEWORK_API GlslTU : public ShaderTU
	{
	public:
		GlslTU(TRefCountPtr<GpuShader> InShader)
			: ShaderTU(InShader->GetSourceText(), InShader->GetShaderName())
			, Context(Funcs, GuideLineScopes)
			, Stage(InShader->GetShaderType())
		{
			shaderc::Compiler GlslCompiler;
			shaderc::CompileOptions Options;
			Options.AddMacroDefinition("EDITOR_ISENSE", "1");
			Options.AddMacroDefinition("ENABLE_PRINT", "0");
			Options.AddMacroDefinition("ENABLE_ASSERT", "0");
			Options.SetGenerateDebugInfo();
			Options.SetNonSemanticShaderDebugSource();
			Options.SetOptimizationLevel(shaderc_optimization_level_zero);
			Options.SetIncluder(std::make_unique<ShadercIncludeHandler>(InShader));

			FString SourceText = GpuShaderPreProcessor{ ShaderSource, GpuShaderLanguage::GLSL }
				.ReplacePrintStringLiteral(true)
				.Finalize();
			auto ShaderNameUTF8 = StringCast<UTF8CHAR>(*ShaderName);
			ParsedShader = GlslCompiler.ParseGlslToGlslangShader(TCHAR_TO_UTF8(*SourceText), MapShadercKind(InShader->GetShaderType()), (char*)ShaderNameUTF8.Get(), Options);

			if (ParsedShader.IsValid() && ParsedShader.GetTShader() && ParsedShader.GetTShader()->getIntermediate())
			{
				auto NormalizePrint = [&](std::string& Name)
				{
					if ((Name.size() == 6 && Name.rfind("Print", 0) == 0 && Name.back() >= '0' && Name.back() <= '3') ||
						(Name.size() == 13 && Name.rfind("PrintAtMouse", 0) == 0 && Name.back() >= '0' && Name.back() <= '3') ||
						(Name.size() == 7 && Name.rfind("Assert", 0) == 0 && Name.back() >= '0' && Name.back() <= '1') ||
						(Name.size() == 13 && Name.rfind("AssertFormat", 0) == 0 && Name.back() >= '1' && Name.back() <= '3'))
					{
						if (Name.rfind("PrintAtMouse", 0) == 0)
						{
							Name = "PrintAtMouse";
						}
						else if (Name.rfind("AssertFormat", 0) == 0)
						{
							Name = "AssertFormat";
						}
						else if (Name.rfind("Assert", 0) == 0)
						{
							Name = "Assert";
						}
						else
						{
							Name = "Print";
						}
					}
				};
				for(const auto& Def : ParsedShader.GetTShader()->getMacroDefinitions())
				{
					auto NormalizedDef = Def;
					NormalizePrint(NormalizedDef.name);
					Context.MacroDefinitions.Add(MoveTemp(NormalizedDef));
				}
				for (const auto& Exp : ParsedShader.GetTShader()->getMacroExpansions())
				{
					auto NormalizedExp = Exp;
					NormalizePrint(NormalizedExp.name);
					Context.MacroExpansions.Add(MoveTemp(NormalizedExp));
				}

				auto AST = ParsedShader.GetTShader()->getIntermediate();
				if (auto* Root = AST->getTreeRoot())
				{
#if DEBUG_SHADER
					const FString DumpPath = PathHelper::SavedShaderDir() / InShader->GetShaderName() / InShader->GetShaderName() + TEXT(".glsl.ast");
					GlslDumpTraverser DumpTraverser(DumpPath);
					Root->traverse(&DumpTraverser);
					DumpTraverser.SaveDump();
#endif
					GlslVisitor Visitor(Context, LineRanges.Num());
					Root->traverse(&Visitor);
				}

			}
		}

		TArray<ShaderDiagnosticInfo> GetDiagnostic() override
		{
			FString Msg = UTF8_TO_TCHAR(ParsedShader.GetErrorMessage().c_str());
			return ParseDiagnosticInfoFromGlslang(ShaderName, Msg);
		}

		ShaderSymbol GetSymbolInfo(uint32 Row, uint32 Col, uint32 Size) override
		{
			FString TokenStr = GetStr(Row, Col, Size);
			if (TokenStr.IsEmpty())
			{
				return {};
			}

			// Tokenize type name, splitting keyword modifiers from core type
			auto TokenizeTypeName = [this](const FString& TypeName, TArray<TPair<FString, ShaderTokenType>>& OutTokens) {
				FString Remaining = TypeName.TrimStartAndEnd();

				// Extract keyword prefixes (e.g. const, in, out, uniform, etc.)
				while (!Remaining.IsEmpty())
				{
					int32 SpacePos = Remaining.Find(TEXT(" "));
					if (SpacePos == INDEX_NONE) break;

					FString FirstWord = Remaining.Left(SpacePos);
					if (GLSL::FindKeyword(FirstWord, Stage))
					{
						OutTokens.Add({ FirstWord, ShaderTokenType::Keyword });
						OutTokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
						Remaining = Remaining.Mid(SpacePos + 1).TrimStart();
					}
					else
					{
						break;
					}
				}

				// Add the core type name
				if (!Remaining.IsEmpty())
				{
					ShaderTokenType TypeTokenType = GLSL::FindBuiltinType(Remaining, Stage) ? ShaderTokenType::BuiltinType : ShaderTokenType::Type;
					OutTokens.Add({ Remaining, TypeTokenType });
				}
			};

			ShaderSymbol Symbol{};
			for (const auto& [Def, Ref] : Context.SymbolRefs)
			{
				if (Ref.File == ShaderName && Row == (uint32)Ref.Location.X && Col >= (uint32)Ref.Location.Y && Col <= (uint32)Ref.Location.Y + Ref.Name.Len())
				{
					if (const GLSL::ShaderBuiltinItem* BuiltinFunc = GLSL::FindBuiltinFunc(TokenStr, Stage))
					{
						Symbol.Type = ShaderTokenType::BuiltinFunc;
						Symbol.Desc = BuiltinFunc->Desc;
						Symbol.Url = BuiltinFunc->Url;

						// Parse TypeName format: ReturnType(ParamType1,ParamType2)
						if (!Def.TypeName.IsEmpty())
						{
							FString TypeName = Def.TypeName;
							int32 ParenPos = TypeName.Find(TEXT("("));
							if (ParenPos != INDEX_NONE)
							{
								// Extract return type
								FString ReturnType = TypeName.Left(ParenPos).TrimStartAndEnd();
								TokenizeTypeName(ReturnType, Symbol.Tokens);
								Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
								Symbol.Tokens.Add({ TokenStr, ShaderTokenType::BuiltinFunc });
								Symbol.Tokens.Add({ TEXT("("), ShaderTokenType::Punctuation });

								// Extract parameter types
								FString ParamsStr = TypeName.Mid(ParenPos + 1);
								if (ParamsStr.EndsWith(TEXT(")")))
								{
									ParamsStr = ParamsStr.Left(ParamsStr.Len() - 1);
								}
								ParamsStr.TrimStartAndEndInline();

								if (!ParamsStr.IsEmpty())
								{
									TArray<FString> ParamTypes;
									ParamsStr.ParseIntoArray(ParamTypes, TEXT(","), true);
									
									// Try to find matching signature from BuiltinFunc to get parameter names
									const GLSL::ShaderBuiltinSignature* MatchedSig = nullptr;
									for (const auto& Sig : BuiltinFunc->Signatures)
									{
										if (Sig.Parameters.Num() == ParamTypes.Num())
										{
											MatchedSig = &Sig;
											break; // Use first matching signature
										}
									}
									
									for (int32 i = 0; i < ParamTypes.Num(); i++)
									{
										if (i > 0) Symbol.Tokens.Add({ TEXT(", "), ShaderTokenType::Punctuation });
										FString ParamType = ParamTypes[i].TrimStartAndEnd();
										TokenizeTypeName(ParamType, Symbol.Tokens);
										
										// Append parameter name if available from signature
										if (MatchedSig && MatchedSig->Parameters.IsValidIndex(i))
										{
											const auto& Param = MatchedSig->Parameters[i];
											if (!Param.Name.IsEmpty())
											{
												Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
												Symbol.Tokens.Add({ Param.Name, ShaderTokenType::Param });
											}
										}
									}
								}
								Symbol.Tokens.Add({ TEXT(")"), ShaderTokenType::Punctuation });
							}
							else
							{
								// No parameters, just return type
								TokenizeTypeName(TypeName, Symbol.Tokens);
								Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
								Symbol.Tokens.Add({ TokenStr, ShaderTokenType::BuiltinFunc });
								Symbol.Tokens.Add({ TEXT("()"), ShaderTokenType::Punctuation });
							}

							// Build overloads from JSON for reference display
							for (const auto& Sig : BuiltinFunc->Signatures)
							{
								ShaderSymbol::FuncOverload Overload;
								TokenizeTypeName(Sig.ReturnType, Overload.Tokens);
								Overload.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
								Overload.Tokens.Add({ BuiltinFunc->Label, ShaderTokenType::BuiltinFunc });
								Overload.Tokens.Add({ TEXT("("), ShaderTokenType::Punctuation });
								for (int32 j = 0; j < Sig.Parameters.Num(); j++)
								{
									const auto& Param = Sig.Parameters[j];
									if (j > 0) Overload.Tokens.Add({ TEXT(", "), ShaderTokenType::Punctuation });

									TokenizeTypeName(Param.Type, Overload.Tokens);
									if (!Param.Name.IsEmpty())
									{
										Overload.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
										Overload.Tokens.Add({ Param.Name, ShaderTokenType::Param });
									}

									ParamSemaFlag SemaFlag = ParamSemaFlag::None;
									if (Param.IO == TEXT("in")) SemaFlag = ParamSemaFlag::In;
									else if (Param.IO == TEXT("out")) SemaFlag = ParamSemaFlag::Out;
									else if (Param.IO == TEXT("inout")) SemaFlag = ParamSemaFlag::Inout;

									Overload.Params.Add({ Param.Name, Param.Desc, SemaFlag });
								}
								Overload.Tokens.Add({ TEXT(")"), ShaderTokenType::Punctuation });
								Symbol.Overloads.Add(MoveTemp(Overload));
							}
						}
						return Symbol;
					}

					if (Def.Kind == ShaderTokenType::Var || Def.Kind == ShaderTokenType::LocalVar || Def.Kind == ShaderTokenType::Param)
					{
						Symbol.Type = Def.Kind;
						Symbol.File = Def.File;
						Symbol.Row = Def.Location.X;
						TokenizeTypeName(Def.TypeName, Symbol.Tokens);
						Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
						Symbol.Tokens.Add({ Def.Name, Def.Kind });
						return Symbol;
					}

					if (Def.Kind == ShaderTokenType::Func)
					{
						Symbol.Type = ShaderTokenType::Func;
						Symbol.File = Def.File;
						Symbol.Row = Def.Location.X;

						// Helper lambda to build function tokens
						auto BuildFuncTokens = [&TokenizeTypeName](const FString& FuncTypeName, const FString& FuncName, const TArray<FString>& ParamNames, TArray<TPair<FString, ShaderTokenType>>& OutTokens) {
							int32 ParenPos = FuncTypeName.Find(TEXT("("));
							if (ParenPos != INDEX_NONE)
							{
								// Extract return type
								FString ReturnType = FuncTypeName.Left(ParenPos).TrimStartAndEnd();
								TokenizeTypeName(ReturnType, OutTokens);
								OutTokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
								OutTokens.Add({ FuncName, ShaderTokenType::Func });
								OutTokens.Add({ TEXT("("), ShaderTokenType::Punctuation });

								// Extract parameter types
								FString ParamsStr = FuncTypeName.Mid(ParenPos + 1);
								if (ParamsStr.EndsWith(TEXT(")")))
								{
									ParamsStr = ParamsStr.Left(ParamsStr.Len() - 1);
								}
								ParamsStr.TrimStartAndEndInline();

								if (!ParamsStr.IsEmpty())
								{
									TArray<FString> ParamTypes;
									ParamsStr.ParseIntoArray(ParamTypes, TEXT(","), true);
									for (int32 i = 0; i < ParamTypes.Num(); i++)
									{
										if (i > 0) OutTokens.Add({ TEXT(", "), ShaderTokenType::Punctuation });
										FString ParamType = ParamTypes[i].TrimStartAndEnd();
										TokenizeTypeName(ParamType, OutTokens);
										
										// Append parameter name if available
										if (ParamNames.IsValidIndex(i) && !ParamNames[i].IsEmpty())
										{
											OutTokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
											OutTokens.Add({ ParamNames[i], ShaderTokenType::Param });
										}
									}
								}
								OutTokens.Add({ TEXT(")"), ShaderTokenType::Punctuation });
							}
							else
							{
								// No parameters
								TokenizeTypeName(FuncTypeName, OutTokens);
								OutTokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
								OutTokens.Add({ FuncName, ShaderTokenType::Func });
								OutTokens.Add({ TEXT("()"), ShaderTokenType::Punctuation });
							}
						};

						// Find parameters for current definition from Funcs
						TArray<FString> DefParamNames;
						const ShaderFunc* DefFunc = Funcs.FindByPredicate([&](const ShaderFunc& Func) {
							return Func.FullName == Def.Id;
						});
						if (DefFunc)
						{
							for (const ShaderParameter& Param : DefFunc->Params)
							{
								DefParamNames.Add(Param.Name);
							}
						}

						// Build Symbol.Tokens for current definition
						BuildFuncTokens(Def.TypeName, Def.Name, DefParamNames, Symbol.Tokens);

						// Find overloads with the same name
						for (const ShaderFunc& Func : Funcs)
						{
							if (Func.Name == Def.Name)
							{
								// Find the function symbol to get its TypeName
								const GlslSymbol* FuncSymbol = Context.SymbolTable.FindByPredicate([&](const GlslSymbol& Sym) {
									return Sym.Kind == ShaderTokenType::Func && Sym.Name == Func.Name && Sym.Id == Func.FullName;
								});

								if (FuncSymbol)
								{
									ShaderSymbol::FuncOverload Overload;
									
									// Extract parameter names from Func.Params
									TArray<FString> OverloadParamNames;
									for (const ShaderParameter& Param : Func.Params)
									{
										OverloadParamNames.Add(Param.Name);
										Overload.Params.Add({ Param.Name, Param.Desc, Param.SemaFlag });
									}
									
									BuildFuncTokens(FuncSymbol->TypeName, Func.Name, OverloadParamNames, Overload.Tokens);

									Symbol.Overloads.Add(MoveTemp(Overload));
								}
							}
						}

						return Symbol;
					}

					if (Def.Kind == ShaderTokenType::Type)
					{
						Symbol.Type = Def.Kind;
						Symbol.File = Def.File;
						Symbol.Row = Def.Location.X;
						Symbol.Tokens.Add({ TEXT("struct"), ShaderTokenType::Keyword });
						Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
						Symbol.Tokens.Add({ Def.Name, ShaderTokenType::Type });
						return Symbol;
					}

					if (Def.Kind == ShaderTokenType::Preprocess)
					{
						Symbol.Type = Def.Kind;
						Symbol.File = Def.File;
						Symbol.Row = Def.Location.X;
						Symbol.Tokens.Add({ TokenStr, ShaderTokenType::Preprocess });
						return Symbol;
					}
				}
			}

			if (const GLSL::ShaderBuiltinItem* BuiltinFunc = GLSL::FindBuiltinFunc(TokenStr, Stage))
			{
				Symbol.Type = ShaderTokenType::BuiltinFunc;
				Symbol.Desc = BuiltinFunc->Desc;
				Symbol.Url = BuiltinFunc->Url;

				Symbol.Tokens.Add({ TokenStr, ShaderTokenType::BuiltinFunc });
				Symbol.Tokens.Add({ TEXT("()"), ShaderTokenType::Punctuation });

				// Build overloads from JSON for reference display
				for (const auto& Sig : BuiltinFunc->Signatures)
				{
					ShaderSymbol::FuncOverload Overload;
					TokenizeTypeName(Sig.ReturnType, Overload.Tokens);
					Overload.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
					Overload.Tokens.Add({ BuiltinFunc->Label, ShaderTokenType::BuiltinFunc });
					Overload.Tokens.Add({ TEXT("("), ShaderTokenType::Punctuation });
					for (int32 j = 0; j < Sig.Parameters.Num(); j++)
					{
						const auto& Param = Sig.Parameters[j];
						if (j > 0) Overload.Tokens.Add({ TEXT(", "), ShaderTokenType::Punctuation });

						TokenizeTypeName(Param.Type, Overload.Tokens);
						if (!Param.Name.IsEmpty())
						{
							Overload.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
							Overload.Tokens.Add({ Param.Name, ShaderTokenType::Param });
						}

						ParamSemaFlag SemaFlag = ParamSemaFlag::None;
						if (Param.IO == TEXT("in")) SemaFlag = ParamSemaFlag::In;
						else if (Param.IO == TEXT("out")) SemaFlag = ParamSemaFlag::Out;
						else if (Param.IO == TEXT("inout")) SemaFlag = ParamSemaFlag::Inout;

						Overload.Params.Add({ Param.Name, Param.Desc, SemaFlag });
					}
					Overload.Tokens.Add({ TEXT(")"), ShaderTokenType::Punctuation });
					Symbol.Overloads.Add(MoveTemp(Overload));
				}

				return Symbol;
			}

			if (const GLSL::ShaderBuiltinItem* BuiltinType = GLSL::FindBuiltinType(TokenStr, Stage))
			{
				if (BuiltinType->Desc.IsEmpty()) return {};
				Symbol.Type = ShaderTokenType::BuiltinType;
				Symbol.Tokens.Add({ TokenStr, ShaderTokenType::BuiltinType });
				Symbol.Desc = BuiltinType->Desc;
				Symbol.Url = BuiltinType->Url;
				return Symbol;
			}

			if (const GLSL::ShaderBuiltinItem* Keyword = GLSL::FindKeyword(TokenStr, Stage))
			{
				if (Keyword->Desc.IsEmpty()) return {};
				Symbol.Type = ShaderTokenType::Keyword;
				Symbol.Tokens.Add({ TokenStr, ShaderTokenType::Keyword });
				Symbol.Desc = Keyword->Desc;
				Symbol.Url = Keyword->Url;
				return Symbol;
			}

			return Symbol;
		}

		ShaderTokenType GetTokenType(ShaderTokenType InType, uint32 Row, uint32 Col, uint32 Size) override
		{
			if (InType == ShaderTokenType::Identifier)
			{
				FString TokenStr = GetStr(Row, Col, Size);
				for (const auto& [Def, Ref] : Context.SymbolRefs)
				{
					if (Ref.File == ShaderName && Row == (uint32)Ref.Location.X && Col >= (uint32)Ref.Location.Y && Col <= (uint32)Ref.Location.Y + Ref.Name.Len())
					{
						return Def.Kind;
					}
				}

				if (GLSL::GetBuiltinTypes(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::BuiltinType;
				}
				else if (GLSL::GetBuiltinFuncs(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::BuiltinFunc;
				}
				else if (GLSL::GetKeyWords(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::Keyword;
				}

			}

			return InType;
		}

		TArray<ShaderCandidateInfo> GetCodeComplete(uint32 Row, uint32 Col) override
		{

			TArray<ShaderCandidateInfo> Candidates;
			
			// Helper to check if a position is within a scope
			auto IsInScope = [](const ShaderScope& Scope, int32 InRow, int32 InCol) {
				if (InRow < Scope.Start.X || InRow > Scope.End.X) return false;
				if (InRow == Scope.Start.X && InCol < Scope.Start.Y) return false;
				if (InRow == Scope.End.X && InCol > Scope.End.Y) return false;
				return true;
			};

			// Helper to calculate scope size (smaller = more specific)
			auto GetScopeSize = [](const ShaderScope& Scope) {
				return (Scope.End.X - Scope.Start.X) * 10000 + (Scope.End.Y - Scope.Start.Y);
			};

			int32 LineStart = LineRanges[Row - 1].BeginIndex;
			int32 CursorPos = LineStart + Col - 1;
			// Look for '.' before cursor to determine if we need member completion
			FString LineBeforeCursor = ShaderSource.Mid(LineStart, Col - 1);
			int32 DotPos = LineBeforeCursor.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);

			auto StructMemberComplete = [&] {
				TArray<ShaderCandidateInfo> Candidates;

				// Extract the identifier before the dot
				FString BeforeDot = LineBeforeCursor.Left(DotPos);
				BeforeDot.TrimEndInline();

				// Find the identifier (variable name) before the dot
				int32 IdentStart = BeforeDot.Len() - 1;
				while (IdentStart >= 0)
				{
					TCHAR Ch = BeforeDot[IdentStart];
					if (!FChar::IsAlnum(Ch) && Ch != '_')
					{
						break;
					}
					IdentStart--;
				}
				IdentStart++;

				FString VarName = BeforeDot.Mid(IdentStart);
				if (VarName.IsEmpty())
				{
					return Candidates;
				}

				// Find the variable considering scope - prefer innermost scope
				const GlslSymbol* VarSymbol = nullptr;
				int32 BestScopeSize = INT32_MAX;

				for (const auto& Sym : Context.SymbolTable)
				{
					if (Sym.Name == VarName && Sym.bIsStruct)
					{
						// Check if current position is within the symbol's scope
						if (IsInScope(Sym.Scope, Row, Col))
						{
							int32 ScopeSize = GetScopeSize(Sym.Scope);
							if (ScopeSize < BestScopeSize)
							{
								BestScopeSize = ScopeSize;
								VarSymbol = &Sym;
							}
						}
					}
				}

				if (!VarSymbol)
				{
					return Candidates;
				}
				// Get members of this struct type
				if (const TArray<GlslStructMember>* Members = Context.StructMembers.Find(VarSymbol->TypeName))
				{
					for (const auto& Member : *Members)
					{
						Candidates.Add({ ShaderCandidateKind::Var, Member.Name });
					}
				}
				return Candidates;
			};

			if (DotPos != INDEX_NONE)
			{
				return StructMemberComplete();
			}

			// Track added names to avoid duplicates, preferring innermost scope
			TMap<FString, int32> AddedSymbols; // Name -> ScopeSize

			for (const auto& Sym : Context.SymbolTable)
			{
				// Check if current position is within the symbol's scope
				if (!IsInScope(Sym.Scope, Row, Col))
				{
					continue;
				}

				int32 ScopeSize = GetScopeSize(Sym.Scope);

				// Check if we already have this symbol from a more specific scope
				if (int32* ExistingScopeSize = AddedSymbols.Find(Sym.Name))
				{
					if (*ExistingScopeSize <= ScopeSize)
					{
						continue; // Already have a more specific one
					}
					// Remove the less specific one and add this one
					Candidates.RemoveAll([&](const ShaderCandidateInfo& C) { return C.Text == Sym.Name; });
				}

				AddedSymbols.Add(Sym.Name, ScopeSize);

				ShaderCandidateKind Kind;
				switch (Sym.Kind)
				{
				case ShaderTokenType::Preprocess:
					Kind = ShaderCandidateKind::Macro;
					break;
				case ShaderTokenType::Func:
					Kind = ShaderCandidateKind::Func;
					break;
				case ShaderTokenType::Type:
					Kind = ShaderCandidateKind::Type;
					break;
				default:
					Kind = ShaderCandidateKind::Var;
					break;
				}

				Candidates.AddUnique({ Kind, Sym.Name });
			}

			return Candidates;
		}

		TArray<ShaderOccurrence> GetOccurrences(uint32 Row, uint32 Col) override
		{
			TArray<ShaderOccurrence> Occurrences;
			for (const auto& [Def, Ref] : Context.SymbolRefs)
			{
				if (Ref.File == ShaderName && Row == (uint32)Ref.Location.X && Col >= (uint32)Ref.Location.Y && Col <= (uint32)Ref.Location.Y + Ref.Name.Len())
				{
					TArray<GlslSymbolRef> Refs;
					Context.SymbolRefs.MultiFind(Def, Refs);
					for (const auto& Ref : Refs)
					{
						Occurrences.Emplace(Ref.Location.X, Ref.Location.Y);
					}
					break;
				}
			}
			return Occurrences;
		}

	private:
		shaderc::ParsedGlslangShader ParsedShader;
		GlslContext Context;
		ShaderType Stage;
	};
}
