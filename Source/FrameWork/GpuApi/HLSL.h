
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
	TArray<FString> KeyWords = {
		"register", "packoffset", "static", "const", "out", "in", "inout", "extern", "inline", "precise",
		"break", "continue", "discard", "do", "for", "if", "struct", "typedef",
		"else", "switch", "while", "case", "default", "return", "true", "false",
		// Storage and interpolation modifiers
		"groupshared", "globallycoherent", "linear", "nointerpolation", "centroid", "noperspective", "sample",
		// Matrix layout
		"row_major", "column_major",
		// Compiler hints
		"unroll", "loop", "flatten", "branch", "forcecase", "call",
		// Geometry shader primitives
		"point", "line", "triangle", "lineadj", "triangleadj",
		// Semantics
		"SV_Coverage", "SV_Depth", "SV_DispatchThreadID", "SV_DomainLocation", "SV_GroupID", "SV_GroupIndex", "SV_GroupThreadID", "SV_GSInstanceID",
		"SV_InsideTessFactor", "SV_IsFrontFace", "SV_OutputControlPointID", "SV_POSITION", "SV_Position", "SV_RenderTargetArrayIndex",
		"SV_SampleIndex", "SV_TessFactor", "SV_ViewportArrayIndex", "SV_InstanceID", "SV_PrimitiveID", "SV_VertexID", "SV_TargetID",
		"SV_TARGET", "SV_Target", "SV_Target0", "SV_Target1", "SV_Target2", "SV_Target3", "SV_Target4", "SV_Target5", "SV_Target6", "SV_Target7",
		"SV_StencilRef", "SV_Barycentrics", "SV_ShadingRate",
		"template", "typename", "sizeof", "numthreads", "_Static_assert",
	};

	TArray<FString> BuiltinTypes = {
		"bool", "bool1", "bool2", "bool3", "bool4", "bool1x1", "bool1x2", "bool1x3", "bool1x4",
		"bool2x1", "bool2x2", "bool2x3", "bool2x4", "bool3x1", "bool3x2", "bool3x3", "bool3x4",
		"bool4x1", "bool4x2", "bool4x3", "bool4x4",
		"int", "int1", "int2", "int3", "int4", "int1x1", "int1x2", "int1x3", "int1x4",
		"int2x1", "int2x2", "int2x3", "int2x4", "int3x1", "int3x2", "int3x3", "int3x4",
		"int4x1", "int4x2", "int4x3", "int4x4", "int64_t",
		"uint", "uint1", "uint2", "uint3", "uint4", "uint1x1", "uint1x2", "uint1x3", "uint1x4",
		"uint2x1", "uint2x2", "uint2x3", "uint2x4", "uint3x1", "uint3x2", "uint3x3", "uint3x4",
		"uint4x1", "uint4x2", "uint4x3", "uint4x4", "uint64_t",
		"dword", "dword1", "dword2", "dword3", "dword4", "dword1x1", "dword1x2", "dword1x3", "dword1x4",
		"dword2x1", "dword2x2", "dword2x3", "dword2x4", "dword3x1", "dword3x2", "dword3x3", "dword3x4",
		"dword4x1", "dword4x2", "dword4x3", "dword4x4",
		"half", "half1", "half2", "half3", "half4", "half1x1", "half1x2", "half1x3", "half1x4",
		"half2x1", "half2x2", "half2x3", "half2x4", "half3x1", "half3x2", "half3x3", "half3x4",
		"half4x1", "half4x2", "half4x3", "half4x4",
		"float", "float1", "float2", "float3", "float4", "float1x1", "float1x2", "float1x3", "float1x4",
		"float2x1", "float2x2", "float2x3", "float2x4", "float3x1", "float3x2", "float3x3", "float3x4",
		"float4x1", "float4x2", "float4x3", "float4x4",
		"double", "double1", "double2", "double3", "double4", "double1x1", "double1x2", "double1x3", "double1x4",
		"double2x1", "double2x2", "double2x3", "double2x4", "double3x1", "double3x2", "double3x3", "double3x4",
		"double4x1", "double4x2", "double4x3", "double4x4",
		"snorm", "unorm", "string", "void", "cbuffer",
		"Buffer", "AppendStructuredBuffer", "ByteAddressBuffer", "ConsumeStructuredBuffer", "StructuredBuffer", "ConstantBuffer",
		"RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D",
		"sampler", "sampler1D", "sampler2D", "sampler3D", "samplerCUBE", "SamplerComparisonState", "SamplerState",
		"texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS", "Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray",
		// Min precision types
		"min16float", "min16float1", "min16float2", "min16float3", "min16float4",
		"min16int", "min16int1", "min16int2", "min16int3", "min16int4",
		"min16uint", "min16uint1", "min16uint2", "min16uint3", "min16uint4",
		"min10float", "min10float1", "min10float2", "min10float3", "min10float4",
		// 16-bit types (SM6.2+)
		"float16_t", "float16_t2", "float16_t3", "float16_t4",
		"int16_t", "int16_t2", "int16_t3", "int16_t4",
		"uint16_t", "uint16_t2", "uint16_t3", "uint16_t4",
		// Geometry/Tessellation shader types
		"TriangleStream", "PointStream", "LineStream", "InputPatch", "OutputPatch",
	};

	TArray<FString> BuiltinFuncs = {
		"abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble",
		"asfloat", "asin", "asint", "asuint", "atan", "atan2", "ceil", "clamp", "clip", "cos", "cosh", "countbits",
		"cross", "D3DCOLORtoUBYTE4", "ddx", "ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine",
		"degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
		"distance", "dot", "dst", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample",
		"EvaluateAttributeSnapped", "exp", "exp2", "f16tof32", "f32tof16", "faceforward", "firstbithigh",
		"firstbitlow", "floor", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
		"GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync",
		"InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange", "InterlockedCompareStore",
		"InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor",
		"isfinite", "isinf", "isnan", "ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "mul",
		"noise", "normalize", "pow", "Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax",
		"Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
		"ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg",
		"ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin", "radians", "rcp", "reflect", "refract",
		"reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
		"tan", "tanh", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2Dbias",
		"tex2Dgrad", "tex2Dlod", "tex2Dproj", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj",
		"texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc", "and", "or", "select",
		// Wave intrinsics (SM6.0+)
		"WaveGetLaneCount", "WaveGetLaneIndex", "WaveIsFirstLane",
		"WaveActiveAnyTrue", "WaveActiveAllTrue", "WaveActiveBallot",
		"WaveReadLaneFirst", "WaveReadLaneAt", "WaveActiveAllEqual",
		"WaveActiveCountBits", "WaveActiveSum", "WaveActiveProduct", "WaveActiveMin", "WaveActiveMax",
		"WaveActiveBitAnd", "WaveActiveBitOr", "WaveActiveBitXor",
		"WavePrefixCountBits", "WavePrefixSum", "WavePrefixProduct",
		"QuadReadLaneAt", "QuadReadAcrossDiagonal", "QuadReadAcrossX", "QuadReadAcrossY",
		// Misc
		"NonUniformResourceIndex", "CheckAccessFullyMapped", "GetAttributeAtVertex"
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
		HlslTU(const FString& InShaderSource, const TArray<FString>& IncludeDirs)
			: ShaderTU(InShaderSource)
		{
			DxcCreateInstance(CLSID_DxcIntelliSense, IID_PPV_ARGS(ISense.GetInitReference()));
			ISense->CreateIndex(Index.GetInitReference());
			auto SourceText = StringCast<UTF8CHAR>(*InShaderSource);
			ISense->CreateUnsavedFile("Temp.hlsl", (char*)SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR), Unsaved.GetInitReference());

			TArray<const char*> DxcArgs;
			DxcArgs.Add("-D");
			DxcArgs.Add("ENABLE_PRINT=0");
			DxcArgs.Add("-D");
			DxcArgs.Add("EDITOR_ISENSE=1");

			TArray<FTCHARToUTF8> CharIncludeDirs;
			for (const FString& IncludeDir : IncludeDirs)
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
						return ShaderTokenType::Parm;
					}

					if (ReferencedCursorKind == DxcCursor_FunctionDecl || ReferencedCursorKind == DxcCursor_CXXMethod)
					{
						return ShaderTokenType::Func;
					}
				}

				if (TypeSpelling == TokenStr && !NullReferencedExtent)
				{
					if (ReferencedCursorKind == DxcCursor_StructDecl)
					{
						return ShaderTokenType::Type;
					}
				}

				if (HLSL::BuiltinTypes.Contains(TokenStr))
				{
					return ShaderTokenType::BuildtinType;
				}
				else if (HLSL::BuiltinFuncs.Contains(TokenStr))
				{
					return ShaderTokenType::BuildtinFunc;
				}
				else if (HLSL::KeyWords.Contains(TokenStr))
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
			TArray<ShaderOccurrence> Occurrences;

			TRefCountPtr<IDxcSourceLocation> SrcLoc;
			TU->GetLocation(DxcFile, Row, Col, SrcLoc.GetInitReference());
			TRefCountPtr<IDxcCursor> DxcCursor;
			TU->GetCursorForLocation(SrcLoc, DxcCursor.GetInitReference());
			uint32 ResultLen;
			IDxcCursor** CursorOccurrences;
			DxcCursor->FindReferencesInFile(DxcFile, 0, -1, &ResultLen, &CursorOccurrences);

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
								Scopes.Emplace(ScopeStart, ScopeEnd);
							}
						}
						else if (Kind == DxcCursor_IfStmt)
						{
							if (ParKind != DxcCursor_IfStmt)
							{
								Scopes.Emplace(ScopeStart, ScopeEnd);
							}
						}
						else
						{
							Scopes.Emplace(ScopeStart, ScopeEnd);
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
						LPSTR ParmName;
						ChildCursor->GetSpelling(&ParmName);

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
							Func->Params.Emplace(ANSI_TO_TCHAR(ParmName), Flag);
						}

						for (unsigned int j = 0; j < TokenCount; j++)
						{
							Tokens[j]->Release();
						}
						CoTaskMemFree(Tokens);
						CoTaskMemFree(FuncName);
						CoTaskMemFree(ParmName);
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
	};
}