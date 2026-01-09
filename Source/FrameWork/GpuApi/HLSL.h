#pragma once

#include "GpuShader.h"
#include "Common/Path/PathHelper.h"
#include <Serialization/JsonSerializer.h>

THIRD_PARTY_INCLUDES_START
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <ole2.h>
#include "dxcisense.h"
#include "Windows/HideWindowsPlatformTypes.h"
#else
#define BOOL RESOLVED_BOOL
#include "dxcisense.h"
#endif
THIRD_PARTY_INCLUDES_END

namespace HLSL
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
			FString JsonPath = FW::PathHelper::ShaderDir() / TEXT("HLSL.json");
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

	TArray<ShaderDiagnosticInfo> ParseDiagnosticInfoFromDxc(FStringView HlslDiagnosticInfo)
	{
		TArray<ShaderDiagnosticInfo> Ret;

		int32 LineInfoFirstPos = HlslDiagnosticInfo.Find(TEXT("hlsl:"));
		while (LineInfoFirstPos != INDEX_NONE)
		{
			ShaderDiagnosticInfo DiagnosticInfo;

			int32 LineInfoLastPos = HlslDiagnosticInfo.Find(TEXT("\n"), LineInfoFirstPos);
			FStringView LineStringView{ HlslDiagnosticInfo.GetData() + LineInfoFirstPos, LineInfoLastPos - LineInfoFirstPos };

			int32 LineInfoFirstColonPos = 4;
			int32 Pos2 = LineStringView.Find(TEXT(":"), LineInfoFirstColonPos + 1);
			DiagnosticInfo.Row = FCString::Atoi(LineStringView.SubStr(LineInfoFirstColonPos + 1, Pos2 - LineInfoFirstColonPos - 1).GetData());
			int32 Pos3 = LineStringView.Find(TEXT(":"), Pos2 + 1);
			DiagnosticInfo.Col = FCString::Atoi(LineStringView.SubStr(Pos2 + 1, Pos3 - Pos2 - 1).GetData());

			int32 ErrorPos = LineStringView.Find(TEXT("error: "), Pos3);
			if (ErrorPos != INDEX_NONE)
			{
				int32 ErrorInfoEnd = LineStringView.Find(TEXT("["), ErrorPos + 7);
				if (ErrorInfoEnd == INDEX_NONE) {
					ErrorInfoEnd = LineStringView.Len();
				}
				DiagnosticInfo.Error = LineStringView.SubStr(ErrorPos + 7, ErrorInfoEnd - ErrorPos - 7);
				Ret.Add(MoveTemp(DiagnosticInfo));
			}
			else
			{
				int32 WarnPos = LineStringView.Find(TEXT("warning: "), Pos3);
				int32 WarnInfoEnd = LineStringView.Find(TEXT("["), WarnPos + 9);
				if (WarnInfoEnd == INDEX_NONE) {
					WarnInfoEnd = LineStringView.Len();
				}
				DiagnosticInfo.Warn = LineStringView.SubStr(WarnPos + 9, WarnInfoEnd - WarnPos - 9);
				Ret.Add(MoveTemp(DiagnosticInfo));
			}

			LineInfoFirstPos = HlslDiagnosticInfo.Find(TEXT("hlsl:"), LineInfoLastPos);
		}

		return Ret;
	}


	ShaderCandidateKind MapCursorDecl(DxcCursorKind InDecl)
	{
		switch (InDecl)
		{
		case DxcCursor_VarDecl:
		case DxcCursor_ParmDecl:
		case DxcCursor_FieldDecl:
			return ShaderCandidateKind::Var;
		case DxcCursor_MacroDefinition:
			return ShaderCandidateKind::Macro;
		case DxcCursor_FunctionDecl:
		case DxcCursor_CXXMethod:
		case DxcCursor_FunctionTemplate:
			return ShaderCandidateKind::Func;
		case DxcCursor_TypedefDecl:
		case DxcCursor_StructDecl:
			return ShaderCandidateKind::Type;
		default:
			return ShaderCandidateKind::Unknown;
		}
	}

	class FRAMEWORK_API HlslTU : public ShaderTU
	{
	public:
		HlslTU(TRefCountPtr<GpuShader> InShader)
			: ShaderTU(InShader->GetSourceText())
			, Stage(InShader->GetShaderType())
		{
			DxcCreateInstance(CLSID_DxcIntelliSense, IID_PPV_ARGS(ISense.GetInitReference()));
			ISense->CreateIndex(Index.GetInitReference());
			auto SourceText = StringCast<UTF8CHAR>(*ShaderSource);
			ISense->CreateUnsavedFile("Temp.hlsl", (char*)SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR), Unsaved.GetInitReference());

			TArray<const char*> DxcArgs;
			DxcArgs.Add("-D");
			DxcArgs.Add("ENABLE_PRINT=0");
			DxcArgs.Add("-D");
			DxcArgs.Add("EDITOR_ISENSE=1");

			TArray<FTCHARToUTF8> CharIncludeDirs;
			for (const FString& IncludeDir : InShader->GetIncludeDirs())
			{
				DxcArgs.Add("-I");
				CharIncludeDirs.Emplace(*IncludeDir);
				DxcArgs.Add(CharIncludeDirs.Last().Get());
			}

			DxcTranslationUnitFlags UnitFlag = DxcTranslationUnitFlags(DxcTranslationUnitFlags_UseCallerThread | DxcTranslationUnitFlags_DetailedPreprocessingRecord);
			Index->ParseTranslationUnit("Temp.hlsl", DxcArgs.GetData(), DxcArgs.Num(), AUX::GetAddrExt(Unsaved.GetReference()), 1, UnitFlag, TU.GetInitReference());

			TU->GetFile("Temp.hlsl", DxcFile.GetInitReference());
			TU->GetCursor(DxcRootCursor.GetInitReference());
			TraversalAST(DxcRootCursor);
		}

		TArray<ShaderDiagnosticInfo> GetDiagnostic() override
		{
			uint32 NumDiag{};
			TU->GetNumDiagnostics(&NumDiag);
			FString Diags;
			for (uint32 i = 0; i < NumDiag; i++)
			{
				TRefCountPtr<IDxcDiagnostic> Diag;
				TU->GetDiagnostic(i, Diag.GetInitReference());
				char* DiagResult;
				DxcDiagnosticDisplayOptions Options = DxcDiagnosticDisplayOptions(DxcDiagnostic_DisplaySourceLocation | DxcDiagnostic_DisplayColumn | DxcDiagnostic_DisplaySeverity);
				Diag->FormatDiagnostic(Options, &DiagResult);
				Diags += DiagResult;
				Diags += "\n";
				CoTaskMemFree(DiagResult);
			}
			return ParseDiagnosticInfoFromDxc(Diags);
		}

		ShaderSymbol GetSymbolInfo(uint32 Row, uint32 Col) override
		{
			ShaderSymbol Symbol{};

			TRefCountPtr<IDxcSourceLocation> SrcLoc;
			TU->GetLocation(DxcFile, Row, Col, SrcLoc.GetInitReference());
			TRefCountPtr<IDxcCursor> DxcCursor;
			TU->GetCursorForLocation(SrcLoc, DxcCursor.GetInitReference());

			LPSTR CursorSpellingPtr;
			DxcCursor->GetSpelling(&CursorSpellingPtr);
			FString CursorSpelling = CursorSpellingPtr;
			CoTaskMemFree(CursorSpellingPtr);

			DxcCursorKind CursorKind;
			DxcCursor->GetKind(&CursorKind);

			// Check for builtin functions first
			if (const HLSL::ShaderBuiltinItem* BuiltinFunc = HLSL::FindBuiltinFunc(CursorSpelling, Stage))
			{
				Symbol.Type = ShaderTokenType::BuildtinFunc;
				Symbol.Desc = BuiltinFunc->Desc;
				Symbol.Url = BuiltinFunc->Url;

				// Simplify DXC template type names to HLSL short form
				// e.g. "vector<float, 3> (vector<float, 3>)" -> "float3"
				auto SimplifyTypeName = [](const FString& TypeName) {
					FString Result = TypeName;
					int32 ParenPos = Result.Find(TEXT(" ("));
					if (ParenPos != INDEX_NONE) Result = Result.Left(ParenPos);

					if (Result.StartsWith(TEXT("vector<")))
					{
						int32 CommaPos = Result.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, 7);
						int32 EndPos = Result.Find(TEXT(">"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 7);
						if (CommaPos != INDEX_NONE && EndPos != INDEX_NONE)
							Result = Result.Mid(7, CommaPos - 7).TrimStartAndEnd() + Result.Mid(CommaPos + 1, EndPos - CommaPos - 1).TrimStartAndEnd();
					}
					else if (Result.StartsWith(TEXT("matrix<")))
					{
						int32 Comma1 = Result.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, 7);
						int32 Comma2 = Result.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, Comma1 + 1);
						int32 EndPos = Result.Find(TEXT(">"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 7);
						if (Comma1 != INDEX_NONE && Comma2 != INDEX_NONE && EndPos != INDEX_NONE)
							Result = Result.Mid(7, Comma1 - 7).TrimStartAndEnd() + Result.Mid(Comma1 + 1, Comma2 - Comma1 - 1).TrimStartAndEnd() + TEXT("x") + Result.Mid(Comma2 + 1, EndPos - Comma2 - 1).TrimStartAndEnd();
					}
					return Result;
				};

				// Get return type from DXC cursor type (the type of a function call expression is its return type)
				TRefCountPtr<IDxcType> CursorType;
				DxcCursor->GetCursorType(CursorType.GetInitReference());
				LPSTR ReturnTypeNamePtr;
				CursorType->GetSpelling(&ReturnTypeNamePtr);
				FString ReturnTypeName = SimplifyTypeName(ReturnTypeNamePtr);
				CoTaskMemFree(ReturnTypeNamePtr);

				// Build Symbol.Tokens entirely from DXC
				Symbol.Tokens.Add({ ReturnTypeName, ShaderTokenType::BuildtinType });
				Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
				Symbol.Tokens.Add({ CursorSpelling, ShaderTokenType::BuildtinFunc });
				Symbol.Tokens.Add({ TEXT("("), ShaderTokenType::Punctuation });

				// Get referenced function cursor to obtain parameter information
				TRefCountPtr<IDxcCursor> RefCursor;
				DxcCursor->GetReferencedCursor(RefCursor.GetInitReference());
				BOOL NullRefCursor;
				RefCursor->IsNull(&NullRefCursor);
				if (!NullRefCursor)
				{
					int NumArgs;
					RefCursor->GetNumArguments(&NumArgs);
					for (int i = 0; i < NumArgs; i++)
					{
						if (i > 0) Symbol.Tokens.Add({ TEXT(", "), ShaderTokenType::Punctuation });
						TRefCountPtr<IDxcCursor> ArgCursor;
						RefCursor->GetArgumentAt(i, ArgCursor.GetInitReference());

						TRefCountPtr<IDxcType> ArgType;
						ArgCursor->GetCursorType(ArgType.GetInitReference());
						LPSTR ArgTypeNamePtr;
						ArgType->GetSpelling(&ArgTypeNamePtr);
						FString ArgTypeName = SimplifyTypeName(ArgTypeNamePtr);
						CoTaskMemFree(ArgTypeNamePtr);

						LPSTR ArgNamePtr;
						ArgCursor->GetSpelling(&ArgNamePtr);
						FString ArgName = ArgNamePtr;
						CoTaskMemFree(ArgNamePtr);

						Symbol.Tokens.Add({ ArgTypeName, ShaderTokenType::BuildtinType });
						if (!ArgName.IsEmpty())
						{
							Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
							Symbol.Tokens.Add({ ArgName, ShaderTokenType::Param });
						}
					}
				}
				Symbol.Tokens.Add({ TEXT(")"), ShaderTokenType::Punctuation });

				// Build overloads from JSON for reference display
				for (const auto& Sig : BuiltinFunc->Signatures)
				{
					ShaderSymbol::FuncOverload Overload;
					Overload.Tokens.Add({ Sig.ReturnType, ShaderTokenType::BuildtinType });
					Overload.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
					Overload.Tokens.Add({ BuiltinFunc->Label, ShaderTokenType::BuildtinFunc });
					Overload.Tokens.Add({ TEXT("("), ShaderTokenType::Punctuation });
					for (int32 j = 0; j < Sig.Parameters.Num(); j++)
					{
						const auto& Param = Sig.Parameters[j];
						if (j > 0) Overload.Tokens.Add({ TEXT(", "), ShaderTokenType::Punctuation });

						ParamSemaFlag SemaFlag = ParamSemaFlag::None;
						if (Param.IO == TEXT("in")) SemaFlag = ParamSemaFlag::In;
						else if (Param.IO == TEXT("out")) SemaFlag = ParamSemaFlag::Out;
						else if (Param.IO == TEXT("inout")) SemaFlag = ParamSemaFlag::Inout;

						if (SemaFlag != ParamSemaFlag::None)
						{
							Overload.Tokens.Add({ Param.IO, ShaderTokenType::Keyword });
							Overload.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
						}
						Overload.Tokens.Add({ Param.Type, ShaderTokenType::BuildtinType });
						Overload.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
						Overload.Tokens.Add({ Param.Name, ShaderTokenType::Param });
						Overload.Params.Add({ Param.Name, Param.Desc, SemaFlag });
					}
					Overload.Tokens.Add({ TEXT(")"), ShaderTokenType::Punctuation });
					Symbol.Overloads.Add(MoveTemp(Overload));
				}

				return Symbol;
			}

			// Check for builtin types
			if (const HLSL::ShaderBuiltinItem* BuiltinType = HLSL::FindBuiltinType(CursorSpelling, Stage))
			{
				Symbol.Type = ShaderTokenType::BuildtinType;
				Symbol.Tokens.Add({ CursorSpelling, ShaderTokenType::BuildtinType });
				Symbol.Desc = BuiltinType->Desc;
				Symbol.Url = BuiltinType->Url;
				return Symbol;
			}

			// Check for keywords
			if (const HLSL::ShaderBuiltinItem* Keyword = HLSL::FindKeyword(CursorSpelling, Stage))
			{
				Symbol.Type = ShaderTokenType::Keyword;
				Symbol.Tokens.Add({ CursorSpelling, ShaderTokenType::Keyword });
				Symbol.Desc = Keyword->Desc;
				Symbol.Url = Keyword->Url;
				return Symbol;
			}

			// Get definition cursor for user-defined symbols
			TRefCountPtr<IDxcCursor> DefDxcCursor;
			DxcCursor->GetDefinitionCursor(DefDxcCursor.GetInitReference());
			BOOL NullDefDxcCursor;
			DefDxcCursor->IsNull(&NullDefDxcCursor);

			if (!NullDefDxcCursor)
			{
				DxcCursorKind DefCursorKind;
				DefDxcCursor->GetKind(&DefCursorKind);

				// Variable or parameter
				if (DefCursorKind == DxcCursor_VarDecl || DefCursorKind == DxcCursor_ParmDecl)
				{
					LPSTR DefCursorNamePtr;
					DefDxcCursor->GetSpelling(&DefCursorNamePtr);
					FString DefName = DefCursorNamePtr;
					CoTaskMemFree(DefCursorNamePtr);

					TRefCountPtr<IDxcType> DefCursorType;
					DefDxcCursor->GetCursorType(DefCursorType.GetInitReference());
					LPSTR DefTypeNamePtr;
					DefCursorType->GetSpelling(&DefTypeNamePtr);
					FString DefTypeName = DefTypeNamePtr;
					CoTaskMemFree(DefTypeNamePtr);

					// Determine if it's a local variable or parameter
					TRefCountPtr<IDxcCursor> ParentCursor;
					DefDxcCursor->GetLexicalParent(ParentCursor.GetInitReference());
					DxcCursorKind ParentKind;
					ParentCursor->GetKind(&ParentKind);

					if (DefCursorKind == DxcCursor_ParmDecl)
					{
						Symbol.Type = ShaderTokenType::Param;
					}
					else if (ParentKind == DxcCursor_FunctionDecl || ParentKind == DxcCursor_CXXMethod || ParentKind == DxcCursor_FunctionTemplate)
					{
						Symbol.Type = ShaderTokenType::LocalVar;
					}
					else
					{
						Symbol.Type = ShaderTokenType::Var;
					}

					ShaderTokenType DefTypeTokenType = HLSL::FindBuiltinType(DefTypeName, Stage) ? ShaderTokenType::BuildtinType : ShaderTokenType::Type;
					Symbol.Tokens.Add({ DefTypeName, DefTypeTokenType });
					Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
					Symbol.Tokens.Add({ DefName, Symbol.Type });
					GetCursorLocation(DefDxcCursor, Symbol.File, Symbol.Row);
					return Symbol;
				}

				// User-defined function
				if (DefCursorKind == DxcCursor_FunctionDecl || DefCursorKind == DxcCursor_CXXMethod)
				{
					Symbol.Type = ShaderTokenType::Func;

					LPSTR FuncNamePtr;
					DefDxcCursor->GetSpelling(&FuncNamePtr);
					FString FuncName = FuncNamePtr;
					CoTaskMemFree(FuncNamePtr);

					// Helper lambda to build tokens for a function cursor
					auto BuildFuncTokens = [this](IDxcCursor* InFuncCursor, TArray<TPair<FString, ShaderTokenType>>& OutTokens) {
						LPSTR NamePtr;
						InFuncCursor->GetSpelling(&NamePtr);
						FString Name = NamePtr;
						CoTaskMemFree(NamePtr);

						TRefCountPtr<IDxcType> Type;
						InFuncCursor->GetCursorType(Type.GetInitReference());
						LPSTR TypeSpellingPtr;
						Type->GetSpelling(&TypeSpellingPtr);
						FString TypeSpelling = TypeSpellingPtr;
						CoTaskMemFree(TypeSpellingPtr);

						int32 ParenPos = TypeSpelling.Find(TEXT("("));
						FString ReturnType = ParenPos != INDEX_NONE ? TypeSpelling.Left(ParenPos).TrimEnd() : TypeSpelling;

						ShaderTokenType ReturnTokenType = HLSL::FindBuiltinType(ReturnType, Stage) ? ShaderTokenType::BuildtinType : ShaderTokenType::Type;
						OutTokens.Add({ ReturnType, ReturnTokenType });
						OutTokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
						OutTokens.Add({ Name, ShaderTokenType::Func });
						OutTokens.Add({ TEXT("("), ShaderTokenType::Punctuation });

						int NumArgs;
						InFuncCursor->GetNumArguments(&NumArgs);
						for (int i = 0; i < NumArgs; i++)
						{
							if (i > 0) OutTokens.Add({ TEXT(", "), ShaderTokenType::Punctuation });
							TRefCountPtr<IDxcCursor> ArgCursor;
							InFuncCursor->GetArgumentAt(i, ArgCursor.GetInitReference());

							TRefCountPtr<IDxcType> ArgType;
							ArgCursor->GetCursorType(ArgType.GetInitReference());
							LPSTR ArgTypeNamePtr;
							ArgType->GetSpelling(&ArgTypeNamePtr);
							FString ArgTypeName = ArgTypeNamePtr;
							CoTaskMemFree(ArgTypeNamePtr);

							LPSTR ArgNamePtr;
							ArgCursor->GetSpelling(&ArgNamePtr);
							FString ArgName = ArgNamePtr;
							CoTaskMemFree(ArgNamePtr);

							ShaderTokenType ArgTokenType = HLSL::FindBuiltinType(ArgTypeName, Stage) ? ShaderTokenType::BuildtinType : ShaderTokenType::Type;
							OutTokens.Add({ ArgTypeName, ArgTokenType });
							if (!ArgName.IsEmpty())
							{
								OutTokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
								OutTokens.Add({ ArgName, ShaderTokenType::Param });
							}
						}
						OutTokens.Add({ TEXT(")"), ShaderTokenType::Punctuation });
					};

					// Build Symbol.Tokens for current definition
					BuildFuncTokens(DefDxcCursor, Symbol.Tokens);
					GetCursorLocation(DefDxcCursor, Symbol.File, Symbol.Row);

					// Find overloads with the same name
					for (const ShaderFunc& Func : Funcs)
					{
						if (Func.Name == FuncName)
						{
							TRefCountPtr<IDxcSourceLocation> FuncLoc;
							TU->GetLocation(DxcFile, Func.Start.X, Func.Start.Y, FuncLoc.GetInitReference());
							TRefCountPtr<IDxcCursor> FuncCursor;
							TU->GetCursorForLocation(FuncLoc, FuncCursor.GetInitReference());

							ShaderSymbol::FuncOverload Overload;
							BuildFuncTokens(FuncCursor, Overload.Tokens);

							// Add parameter info
							for (const ShaderParameter& Param : Func.Params)
							{
								Overload.Params.Add({ Param.Name, Param.Desc, Param.SemaFlag });
							}

							Symbol.Overloads.Add(MoveTemp(Overload));
						}
					}

					return Symbol;
				}

				// Struct/type definition
				if (DefCursorKind == DxcCursor_StructDecl || DefCursorKind == DxcCursor_TypedefDecl)
				{
					Symbol.Type = ShaderTokenType::Type;

					LPSTR TypeNamePtr;
					DefDxcCursor->GetSpelling(&TypeNamePtr);
					FString TypeName = TypeNamePtr;
					CoTaskMemFree(TypeNamePtr);

					Symbol.Tokens.Add({ TEXT("struct"), ShaderTokenType::Keyword });
					Symbol.Tokens.Add({ TEXT(" "), ShaderTokenType::Unknown });
					Symbol.Tokens.Add({ TypeName, ShaderTokenType::Type });
					GetCursorLocation(DefDxcCursor, Symbol.File, Symbol.Row);
					return Symbol;
				}
			}

			// Macro
			if (CursorKind == DxcCursor_MacroExpansion || CursorKind == DxcCursor_MacroDefinition)
			{
				Symbol.Type = ShaderTokenType::Macro;
				Symbol.Tokens.Add({ CursorSpelling, ShaderTokenType::Macro });

				TRefCountPtr<IDxcCursor> MacroDefCursor;
				if (CursorKind == DxcCursor_MacroExpansion)
				{
					DxcCursor->GetReferencedCursor(MacroDefCursor.GetInitReference());
				}
				else
				{
					MacroDefCursor = DxcCursor;
				}

				BOOL NullMacroDef;
				MacroDefCursor->IsNull(&NullMacroDef);
				if (!NullMacroDef)
				{
					GetCursorLocation(MacroDefCursor, Symbol.File, Symbol.Row);
				}
				return Symbol;
			}

			return Symbol;
		}

		ShaderTokenType GetTokenType(ShaderTokenType InType, uint32 Row, uint32 Col, uint32 Size) override
		{
			TRefCountPtr<IDxcSourceLocation> SrcLoc;
			TU->GetLocation(DxcFile, Row, Col, SrcLoc.GetInitReference());

			TRefCountPtr<IDxcCursor> DxcCursor;
			DxcCursorKind CursorKind;
			TU->GetCursorForLocation(SrcLoc, DxcCursor.GetInitReference());
			DxcCursor->GetKind(&CursorKind);

			TRefCountPtr<IDxcCursor> DxcReferencedCursor;
			DxcCursorKind ReferencedCursorKind;
			DxcCursor->GetReferencedCursor(DxcReferencedCursor.GetInitReference());
			DxcReferencedCursor->GetKind(&ReferencedCursorKind);

			TRefCountPtr<IDxcSourceRange> ReferencedExtent;
			DxcReferencedCursor->GetExtent(ReferencedExtent.GetInitReference());

			TRefCountPtr<IDxcCursor> DxcReferencedCursorLexPar;
			DxcCursorKind ReferencedCursorLexParKind;
			DxcReferencedCursor->GetLexicalParent(DxcReferencedCursorLexPar.GetInitReference());
			DxcReferencedCursorLexPar->GetKind(&ReferencedCursorLexParKind);

			//BSTR CursorName;
			//BSTR ReferencedCursorName;
			//DxcCursor->GetDisplayName(&CursorName);
			//DxcReferencedCursor->GetDisplayName(&ReferencedCursorName);

			LPSTR CursorSpelling;
			DxcCursor->GetSpelling(&CursorSpelling);
			FString Spelling = CursorSpelling;
			CoTaskMemFree(CursorSpelling);

			LPSTR CursorTypeSpelling;
			TRefCountPtr<IDxcType> CursorType;
			DxcCursor->GetCursorType(CursorType.GetInitReference());
			CursorType->GetSpelling(&CursorTypeSpelling);
			FString TypeSpelling = CursorTypeSpelling;
			CoTaskMemFree(CursorTypeSpelling);

			if (InType == ShaderTokenType::Identifier)
			{
				FString TokenStr = GetStr(Row, Col, Size);
				BOOL NullReferencedExtent;
				ReferencedExtent->IsNull(&NullReferencedExtent);
				if (Spelling == TokenStr && !NullReferencedExtent)
				{
					if (ReferencedCursorKind == DxcCursor_VarDecl)
					{
						if (ReferencedCursorLexParKind == DxcCursor_FunctionDecl || ReferencedCursorLexParKind == DxcCursor_CXXMethod || ReferencedCursorLexParKind == DxcCursor_FunctionTemplate)
						{
							return ShaderTokenType::LocalVar;
						}
						else
						{
							return ShaderTokenType::Var;
						}
					}

					if (ReferencedCursorKind == DxcCursor_ParmDecl)
					{
						return ShaderTokenType::Param;
					}

					if (ReferencedCursorKind == DxcCursor_FunctionDecl || ReferencedCursorKind == DxcCursor_CXXMethod)
					{
						return ShaderTokenType::Func;
					}
				}

				if (TypeSpelling == TokenStr)
				{
					if (ReferencedCursorKind == DxcCursor_StructDecl || CursorKind == DxcCursor_InitListExpr)
					{
						return ShaderTokenType::Type;
					}
				}

				if (HLSL::GetBuiltinTypes(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::BuildtinType;
				}
				else if (HLSL::GetBuiltinFuncs(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::BuildtinFunc;
				}
				else if (HLSL::GetKeyWords(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::Keyword;
				}

				if (CursorKind == DxcCursor_MacroExpansion)
				{
					return ShaderTokenType::Preprocess;
				}

			}


			return InType;
		}

		TArray<ShaderCandidateInfo> GetCodeComplete(uint32 Row, uint32 Col) override
		{
			TArray<ShaderCandidateInfo> Candidates;

			TRefCountPtr<IDxcCodeCompleteResults> CodeCompleteResults;
			uint32 NumResult{};
			TU->CodeCompleteAt("Temp.hlsl", Row, Col, AUX::GetAddrExt(Unsaved.GetReference()), 1, DxcCodeCompleteFlags_IncludeMacros, CodeCompleteResults.GetInitReference());
			CodeCompleteResults->GetNumResults(&NumResult);
			for (uint32 i = 0; i < NumResult; i++)
			{
				TRefCountPtr<IDxcCompletionResult> CompletionResult;
				TRefCountPtr<IDxcCompletionString> CompletionString;
				CodeCompleteResults->GetResultAt(i, CompletionResult.GetInitReference());

				DxcCursorKind Kind;
				CompletionResult->GetCursorKind(&Kind);

				ShaderCandidateKind CandidateKind = MapCursorDecl(Kind);
				if (CandidateKind != ShaderCandidateKind::Unknown)
				{
					//SH_LOG(LogTemp,Display,TEXT("-------%d-------"),  Kind);
					CompletionResult->GetCompletionString(CompletionString.GetInitReference());
					uint32 NumChunk{};
					CompletionString->GetNumCompletionChunks(&NumChunk);
					for (uint32 ChunkNumber = 0; ChunkNumber < NumChunk; ChunkNumber++)
					{
						DxcCompletionChunkKind CompletionChunkKind;
						CompletionString->GetCompletionChunkKind(ChunkNumber, &CompletionChunkKind);
						char* CompletionText;
						CompletionString->GetCompletionChunkText(ChunkNumber, &CompletionText);
						if (CompletionChunkKind == DxcCompletionChunk_ResultType)
						{
							if (FString{ CompletionText }.Contains(TEXT("__attribute__((ext_vector_type(")))
							{
								break;
							}
						}

						//SH_LOG(LogTemp,Display,TEXT("----%d----"), CompletionChunkKind);
						if (CompletionChunkKind == DxcCompletionChunk_TypedText)
						{
							//SH_LOG(LogTemp,Display,TEXT("%s"), ANSI_TO_TCHAR(CompletionText));
							Candidates.AddUnique({ CandidateKind, CompletionText });
						}
						CoTaskMemFree(CompletionText);
					}
				}


			}

			return Candidates;
		}

		TArray<ShaderOccurrence> GetOccurrences(uint32 Row, uint32 Col) override
		{
			TRefCountPtr<IDxcSourceLocation> SrcLoc;
			TU->GetLocation(DxcFile, Row, Col, SrcLoc.GetInitReference());
			TRefCountPtr<IDxcCursor> DxcCursor;
			TU->GetCursorForLocation(SrcLoc, DxcCursor.GetInitReference());
			TRefCountPtr<IDxcCursor> DefDxcCursor;
			DxcCursor->GetDefinitionCursor(DefDxcCursor.GetInitReference());
			uint32 ResultLen;
			IDxcCursor** CursorOccurrences;
			DefDxcCursor->FindReferencesInFile(DxcFile, 0, -1, &ResultLen, &CursorOccurrences);
			TArray<ShaderOccurrence> Occurrences;
			for (uint32 i = 0; i < ResultLen; i++)
			{
				//TODO: multiple files
				if (IsMainFile(CursorOccurrences[i]))
				{
					TRefCountPtr<IDxcSourceLocation> Loc;
					CursorOccurrences[i]->GetLocation(Loc.GetInitReference());
					uint32 Row, Col;
					Loc->GetSpellingLocation(nullptr, &Row, &Col, nullptr);
					Occurrences.Emplace(Row, Col);
				}

				CursorOccurrences[i]->Release();
			}
			CoTaskMemFree(CursorOccurrences);
			return Occurrences;
		}

	private:
		void GetCursorLocation(IDxcCursor* InCursor, FString& OutFile, uint32& OutRow)
		{
			TRefCountPtr<IDxcSourceLocation> Loc;
			InCursor->GetLocation(Loc.GetInitReference());
			TRefCountPtr<IDxcFile> File;
			Loc->GetSpellingLocation(File.GetInitReference(), &OutRow, nullptr, nullptr);
			if (File)
			{
				LPSTR FileNamePtr;
				File->GetName(&FileNamePtr);
				OutFile = FileNamePtr;
				CoTaskMemFree(FileNamePtr);
			}
		}

		void GetCursorRange(IDxcCursor* InCursor, Vector2i& OutStart, Vector2i& OutEnd)
		{
			TRefCountPtr<IDxcSourceRange> Extent;
			InCursor->GetExtent(Extent.GetInitReference());

			TRefCountPtr<IDxcSourceLocation> StartLoc, EndLoc;
			Extent->GetStart(StartLoc.GetInitReference());
			Extent->GetEnd(EndLoc.GetInitReference());

			unsigned StartLine, StartCol, EndLine, EndCol;
			StartLoc->GetSpellingLocation(nullptr, &StartLine, &StartCol, nullptr);
			EndLoc->GetSpellingLocation(nullptr, &EndLine, &EndCol, nullptr);
			OutStart = Vector2i(StartLine, StartCol);
			OutEnd = Vector2i(EndLine, EndCol);
		}

		bool IsMainFile(IDxcCursor* InCursor)
		{
			TRefCountPtr<IDxcSourceRange> Extent;
			InCursor->GetExtent(Extent.GetInitReference());

			TRefCountPtr<IDxcSourceLocation> StartLoc, EndLoc;
			Extent->GetStart(StartLoc.GetInitReference());
			Extent->GetEnd(EndLoc.GetInitReference());

			TRefCountPtr<IDxcFile> CursorFile;
			unsigned StartLine, StartCol, EndLine, EndCol;
			StartLoc->GetSpellingLocation(CursorFile.GetInitReference(), &StartLine, &StartCol, nullptr);
			EndLoc->GetSpellingLocation(CursorFile.GetInitReference(), &EndLine, &EndCol, nullptr);

			BOOL IsInMainFile;
			CursorFile->IsEqualTo(DxcFile, &IsInMainFile);

			return IsInMainFile;
		}

		void TraversalAST(IDxcCursor* InCursor)
		{
			unsigned int NumChildren = 0;
			IDxcCursor** Children = nullptr;
			InCursor->GetChildren(0, -1, &NumChildren, &Children);
			for (unsigned int i = 0; i < NumChildren; ++i)
			{
				IDxcCursor* ChildCursor = Children[i];
				DxcCursorKind Kind;
				ChildCursor->GetKind(&Kind);

				TRefCountPtr<IDxcSourceRange> Extent;
				ChildCursor->GetExtent(Extent.GetInitReference());

				if (IsMainFile(ChildCursor))
				{
					if (Kind == DxcCursor_StructDecl || 
						Kind == DxcCursor_FunctionTemplate ||
						Kind == DxcCursor_UnexposedDecl ||
						Kind == DxcCursor_FunctionDecl ||
						Kind == DxcCursor_CXXMethod ||
						Kind == DxcCursor_IfStmt ||
						Kind == DxcCursor_ForStmt ||
						Kind == DxcCursor_WhileStmt ||
						Kind == DxcCursor_DoStmt ||
						Kind == DxcCursor_SwitchStmt ||
						Kind == DxcCursor_CompoundStmt)
					{
						Vector2i ScopeStart, ScopeEnd;
						GetCursorRange(ChildCursor, ScopeStart, ScopeEnd);
						DxcCursorKind ParKind;
						InCursor->GetKind(&ParKind);
						if (Kind == DxcCursor_CompoundStmt)
						{
							if (ParKind == DxcCursor_CompoundStmt)
							{
								GuideLineScopes.Emplace(ScopeStart, ScopeEnd);
							}
						}
						else if (Kind == DxcCursor_IfStmt)
						{
							if (ParKind != DxcCursor_IfStmt)
							{
								GuideLineScopes.Emplace(ScopeStart, ScopeEnd);
							}
						}
						else
						{
							GuideLineScopes.Emplace(ScopeStart, ScopeEnd);
						}

					}

					if (Kind == DxcCursor_FunctionDecl || Kind == DxcCursor_CXXMethod)
					{
						BOOL IsDefinition;
						ChildCursor->IsDefinition(&IsDefinition);
						if (IsDefinition)
						{
							LPSTR CursorName;
							ChildCursor->GetSpelling(&CursorName);
							Vector2i FuncStart, FuncEnd;
							GetCursorRange(ChildCursor, FuncStart, FuncEnd);

							if (Kind == DxcCursor_CXXMethod)
							{
								LPSTR ParCursorName;
								InCursor->GetSpelling(&ParCursorName);
								ShaderFunc Func{
									.Name = ANSI_TO_TCHAR(CursorName),
									.FullName = FString(ANSI_TO_TCHAR(ParCursorName)) + "." + ANSI_TO_TCHAR(CursorName),
									.Start = FuncStart,
									.End = FuncEnd,
									.Params = {ShaderParameter{"this"}}
								};
								Funcs.Add(MoveTemp(Func));
							}
							else
							{
								Funcs.Emplace(ANSI_TO_TCHAR(CursorName), ANSI_TO_TCHAR(CursorName), FuncStart, FuncEnd);
							}

							CoTaskMemFree(CursorName);
						}
					}
					else if (Kind == DxcCursor_ParmDecl)
					{
						LPSTR ParamName;
						ChildCursor->GetSpelling(&ParamName);

						ParamSemaFlag Flag = ParamSemaFlag::None;
						unsigned int TokenCount;
						IDxcToken** Tokens;
						TU->Tokenize(Extent, &Tokens, &TokenCount);
						for (unsigned int j = 0; j < TokenCount; j++)
						{
							LPSTR TokenCStr;
							Tokens[j]->GetSpelling(&TokenCStr);
							FString TokenStr = TokenCStr;
							CoTaskMemFree(TokenCStr);

							if (TokenStr == "out")
							{
								Flag = ParamSemaFlag::Out;
								break;
							}
							else if (TokenStr == "in")
							{
								Flag = ParamSemaFlag::In;
								break;
							}
							else if (TokenStr == "inout")
							{
								Flag = ParamSemaFlag::Inout;
								break;
							}
						}

						TRefCountPtr<IDxcCursor> FuncCursor;
						ChildCursor->GetSemanticParent(FuncCursor.GetInitReference());
						LPSTR FuncName;
						FuncCursor->GetSpelling(&FuncName);

						Vector2i FuncStart, FuncEnd;
						GetCursorRange(FuncCursor, FuncStart, FuncEnd);
						ShaderFunc* Func = Funcs.FindByPredicate([&](const ShaderFunc& InItem) {
							return InItem.Name == ANSI_TO_TCHAR(FuncName) && InItem.Start == FuncStart && InItem.End == FuncEnd;
							});
						if (Func)
						{
							Func->Params.Emplace(ANSI_TO_TCHAR(ParamName), "", Flag);
						}

						for (unsigned int j = 0; j < TokenCount; j++)
						{
							Tokens[j]->Release();
						}
						CoTaskMemFree(Tokens);
						CoTaskMemFree(FuncName);
						CoTaskMemFree(ParamName);
					}
				}
				TraversalAST(ChildCursor);
				ChildCursor->Release();
			}
			CoTaskMemFree(Children);
		}

	private:
		TRefCountPtr<IDxcIntelliSense> ISense;
		TRefCountPtr<IDxcIndex> Index;
		TRefCountPtr<IDxcUnsavedFile> Unsaved;
		TRefCountPtr<IDxcTranslationUnit> TU;
		TRefCountPtr<IDxcFile> DxcFile;
		TRefCountPtr<IDxcCursor> DxcRootCursor;
		ShaderType Stage;
	};
}
