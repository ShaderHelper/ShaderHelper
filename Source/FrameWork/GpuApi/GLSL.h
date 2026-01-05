#pragma once

#include "GpuShader.h"
#include "Common/Path/PathHelper.h"
#include "magic_enum.hpp"

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
	// ===================== KeyWords =====================
	// Common keywords (all shader types)
	inline TArray<FString> KeyWordsCommon = {
		// Control flow
		"struct", "if", "else", "switch", "case", "default", "break", "return", "for", "while", "do", "continue",
		// Qualifiers
		"const", "in", "out", "inout", "uniform", "buffer",
		"layout", "location", "binding", "set", "push_constant",
		"highp", "mediump", "lowp", "precision",
		"invariant", "precise",
		// Misc
		"true", "false",
	};

	// Vertex Shader keywords
	inline TArray<FString> KeyWordsVS = {
		// Built-in variables
		"gl_Position", "gl_PointSize", "gl_ClipDistance", "gl_CullDistance",
		"gl_VertexIndex", "gl_InstanceIndex", "gl_VertexID", "gl_InstanceID",
	};

	// Pixel/Fragment Shader keywords
	inline TArray<FString> KeyWordsPS = {
		"discard",
		// Interpolation modifiers
		"flat", "noperspective", "smooth", "centroid", "sample",
		// Layout qualifier
		"input_attachment_index",
		// Built-in variables
		"gl_FragCoord", "gl_FrontFacing", "gl_PointCoord", "gl_SampleID", "gl_SamplePosition", "gl_SampleMaskIn", "gl_SampleMask",
		"gl_FragDepth", "gl_HelperInvocation",
		"gl_PrimitiveID", "gl_Layer", "gl_ViewportIndex",
	};

	// Compute Shader keywords
	inline TArray<FString> KeyWordsCS = {
		// Storage qualifiers
		"shared", "coherent", "volatile", "restrict", "readonly", "writeonly",
		// Built-in variables
		"gl_WorkGroupSize", "gl_WorkGroupID", "gl_LocalInvocationID", "gl_GlobalInvocationID", "gl_LocalInvocationIndex", "gl_NumWorkGroups",
	};

	inline TArray<FString> GetKeyWords(FW::ShaderType Stage) {
		TArray<FString> Result;
		Result.Append(KeyWordsCommon);
		if (Stage == FW::ShaderType::VertexShader)
		{
			Result.Append(KeyWordsVS);
		}
		else if (Stage == FW::ShaderType::PixelShader)
		{
			Result.Append(KeyWordsPS);
		}
		else if (Stage == FW::ShaderType::ComputeShader)
		{
			Result.Append(KeyWordsCS);
		}
		return Result;
	};

	// ===================== BuiltinTypes =====================
	// Common types (all shader types)
	inline TArray<FString> BuiltinTypesCommon = {
		// Scalars
		"void", "bool", "int", "uint", "float", "double",
		// Vectors
		"vec2", "vec3", "vec4", "bvec2", "bvec3", "bvec4",
		"ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4",
		"dvec2", "dvec3", "dvec4",
		// Matrices
		"mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4",
		"dmat2", "dmat3", "dmat4", "dmat2x2", "dmat2x3", "dmat2x4", "dmat3x2", "dmat3x3", "dmat3x4", "dmat4x2", "dmat4x3", "dmat4x4",
		// Samplers
		"sampler", "samplerShadow",
		"sampler1D", "sampler2D", "sampler3D", "samplerCube", "sampler2DRect", "samplerBuffer",
		"sampler1DArray", "sampler2DArray", "samplerCubeArray", "sampler2DMS", "sampler2DMSArray",
		"sampler1DShadow", "sampler2DShadow", "samplerCubeShadow", "sampler2DRectShadow",
		"sampler1DArrayShadow", "sampler2DArrayShadow", "samplerCubeArrayShadow",
		"isampler1D", "isampler2D", "isampler3D", "isamplerCube", "isampler2DRect", "isamplerBuffer",
		"isampler1DArray", "isampler2DArray", "isamplerCubeArray", "isampler2DMS", "isampler2DMSArray",
		"usampler1D", "usampler2D", "usampler3D", "usamplerCube", "usampler2DRect", "usamplerBuffer",
		"usampler1DArray", "usampler2DArray", "usamplerCubeArray", "usampler2DMS", "usampler2DMSArray",
		// Textures (Vulkan)
		"texture1D", "texture2D", "texture3D", "textureCube", "texture2DRect", "textureBuffer",
		"texture1DArray", "texture2DArray", "textureCubeArray", "texture2DMS", "texture2DMSArray",
		"itexture1D", "itexture2D", "itexture3D", "itextureCube", "itexture2DRect", "itextureBuffer",
		"itexture1DArray", "itexture2DArray", "itextureCubeArray", "itexture2DMS", "itexture2DMSArray",
		"utexture1D", "utexture2D", "utexture3D", "utextureCube", "utexture2DRect", "utextureBuffer",
		"utexture1DArray", "utexture2DArray", "utextureCubeArray", "utexture2DMS", "utexture2DMSArray",
	};

	// Pixel/Fragment Shader types
	inline TArray<FString> BuiltinTypesPS = {
		// Subpass input (Vulkan)
		"subpassInput", "subpassInputMS", "isubpassInput", "isubpassInputMS", "usubpassInput", "usubpassInputMS",
	};

	// Compute Shader types
	inline TArray<FString> BuiltinTypesCS = {
		// Images
		"image1D", "image2D", "image3D", "imageCube", "image2DRect", "imageBuffer",
		"image1DArray", "image2DArray", "imageCubeArray", "image2DMS", "image2DMSArray",
		"iimage1D", "iimage2D", "iimage3D", "iimageCube", "iimage2DRect", "iimageBuffer",
		"iimage1DArray", "iimage2DArray", "iimageCubeArray", "iimage2DMS", "iimage2DMSArray",
		"uimage1D", "uimage2D", "uimage3D", "uimageCube", "uimage2DRect", "uimageBuffer",
		"uimage1DArray", "uimage2DArray", "uimageCubeArray", "uimage2DMS", "uimage2DMSArray",
		// Atomic counters
		"atomic_uint",
	};

	inline TArray<FString> GetBuiltinTypes(FW::ShaderType Stage) {
		TArray<FString> Result;
		Result.Append(BuiltinTypesCommon);
		if (Stage == FW::ShaderType::PixelShader)
		{
			Result.Append(BuiltinTypesPS);
		}
		else if (Stage == FW::ShaderType::ComputeShader)
		{
			Result.Append(BuiltinTypesCS);
		}
		return Result;
	};

	// ===================== BuiltinFuncs =====================
	// Common functions (all shader types)
	inline TArray<FString> BuiltinFuncsCommon = {
		// Angle and Trigonometry
		"radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
		// Exponential
		"pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt",
		// Common
		"abs", "sign", "floor", "trunc", "round", "roundEven", "ceil", "fract", "mod", "modf",
		"min", "max", "clamp", "mix", "step", "smoothstep", "isnan", "isinf", "fma",
		"floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat",
		"frexp", "ldexp",
		// Packing and Unpacking
		"packUnorm2x16", "packSnorm2x16", "packUnorm4x8", "packSnorm4x8",
		"unpackUnorm2x16", "unpackSnorm2x16", "unpackUnorm4x8", "unpackSnorm4x8",
		"packHalf2x16", "unpackHalf2x16", "packDouble2x32", "unpackDouble2x32",
		// Geometric
		"length", "distance", "dot", "cross", "normalize", "faceforward", "reflect", "refract",
		// Matrix
		"matrixCompMult", "outerProduct", "transpose", "determinant", "inverse",
		// Vector Relational
		"lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual", "equal", "notEqual", "any", "all", "not",
		// Integer
		"uaddCarry", "usubBorrow", "umulExtended", "imulExtended",
		"bitfieldExtract", "bitfieldInsert", "bitfieldReverse", "bitCount", "findLSB", "findMSB",
		// Texture
		"textureSize", "textureQueryLod", "textureQueryLevels", "textureSamples",
		"texture", "textureProj", "textureLod", "textureOffset", "texelFetch", "texelFetchOffset",
		"textureProjOffset", "textureLodOffset", "textureProjLod", "textureProjLodOffset",
		"textureGrad", "textureGradOffset", "textureProjGrad", "textureProjGradOffset",
		"textureGather", "textureGatherOffset", "textureGatherOffsets",
		// Noise (deprecated but still valid)
		"noise1", "noise2", "noise3", "noise4",
	};

	// Pixel/Fragment Shader functions
	inline TArray<FString> BuiltinFuncsPS = {
		// Derivative
		"dFdx", "dFdy", "dFdxFine", "dFdyFine", "dFdxCoarse", "dFdyCoarse", "fwidth", "fwidthFine", "fwidthCoarse",
		// Interpolation
		"interpolateAtCentroid", "interpolateAtSample", "interpolateAtOffset",
		// Subpass Input (Vulkan)
		"subpassLoad",
	};

	// Compute Shader functions
	inline TArray<FString> BuiltinFuncsCS = {
		// Image
		"imageSize", "imageSamples", "imageLoad", "imageStore",
		"imageAtomicAdd", "imageAtomicMin", "imageAtomicMax", "imageAtomicAnd", "imageAtomicOr", "imageAtomicXor",
		"imageAtomicExchange", "imageAtomicCompSwap",
		// Atomic Counter
		"atomicCounterIncrement", "atomicCounterDecrement", "atomicCounter",
		"atomicCounterAdd", "atomicCounterSubtract", "atomicCounterMin", "atomicCounterMax",
		"atomicCounterAnd", "atomicCounterOr", "atomicCounterXor", "atomicCounterExchange", "atomicCounterCompSwap",
		// Atomic Memory
		"atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr", "atomicXor", "atomicExchange", "atomicCompSwap",
		// Shader Invocation Control
		"barrier", "memoryBarrier", "memoryBarrierAtomicCounter", "memoryBarrierBuffer", "memoryBarrierImage", "memoryBarrierShared", "groupMemoryBarrier",
	};

	inline TArray<FString> GetBuiltinFuncs(FW::ShaderType Stage) {
		TArray<FString> Result;
		Result.Append(BuiltinFuncsCommon);
		if (Stage == FW::ShaderType::PixelShader)
		{
			Result.Append(BuiltinFuncsPS);
		}
		else if (Stage == FW::ShaderType::ComputeShader)
		{
			Result.Append(BuiltinFuncsCS);
		}
		return Result;
	};
}


namespace FW
{
	inline TArray<ShaderDiagnosticInfo> ParseDiagnosticInfoFromGlslang(FStringView GlslDiagnosticInfo)
	{
		TArray<ShaderDiagnosticInfo> Ret;
		int32 LineInfoFirstPos = GlslDiagnosticInfo.Find(TEXT("glsl:"));
		while (LineInfoFirstPos != INDEX_NONE)
		{
			ShaderDiagnosticInfo DiagnosticInfo;

			int32 LineInfoLastPos = GlslDiagnosticInfo.Find(TEXT("\n"), LineInfoFirstPos);
			FStringView LineStringView{ GlslDiagnosticInfo.GetData() + LineInfoFirstPos, LineInfoLastPos - LineInfoFirstPos };

			int32 LineInfoFirstColonPos = 4;
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

			LineInfoFirstPos = GlslDiagnosticInfo.Find(TEXT("glsl:"), LineInfoLastPos);
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
				Type = UTF8_TO_TCHAR(Node->getType().getCompleteString().c_str());
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
				Type = UTF8_TO_TCHAR(Node->getType().getCompleteString().c_str());
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

		bool operator==(const GlslSymbol& Other) const { return File == Other.File && Location == Other.Location; };
		friend uint32 GetTypeHash(const GlslSymbol& Key)
		{
			uint32 Hash = ::GetTypeHash(Key.File);
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

		void visitSymbol(glslang::TIntermSymbol* Node) override
		{
			FString NodeName = UTF8_TO_TCHAR(Node->getName().c_str());
			if (getParentNode()->getAsAggregate() && getParentNode()->getAsAggregate()->getOp() == glslang::EOpParameters)
			{
				bool bIsStruct = Node->getType().isStruct();
				FString TypeName = bIsStruct ? UTF8_TO_TCHAR(Node->getType().getTypeName().c_str()) : FString();

				Context.SymbolTable.AddUnique({
					.Id = LexToString(Node->getId()),
					.Kind = ShaderTokenType::Param,
					.Name = NodeName,
					.File = Node->getLoc().getFilenameStr(),
					.Location = {Node->getLoc().line, Node->getLoc().column},
					.Scope = ScopeStack.Top(),
					.TypeName = TypeName,
					.bIsStruct = bIsStruct
				});
				Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
					.Name = NodeName,
					.File = Node->getLoc().getFilenameStr(),
					.Location = {Node->getLoc().line, Node->getLoc().column}
				});

				if (bIsStruct)
				{
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
			else
			{
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
					FString MemberTypeName = bMemberIsStruct ? UTF8_TO_TCHAR(Member.type->getTypeName().c_str()) : FString();

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
				FString TypeName = bIsStruct ? UTF8_TO_TCHAR(DeclSymbol->getType().getTypeName().c_str()) : FString();

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

				FString Name = UTF8_TO_TCHAR(Node->getName().c_str());
				FString FuncName, _;
				Name.Split(TEXT("("), &FuncName , &_);
				Context.Funcs.Add(ShaderFunc{
					.Name = FuncName, 
					.FullName = Name,
					.Start = StartLoc,
					.End = EndLoc
				});
				Context.SymbolTable.AddUnique({
					.Id = Name,
					.Kind = ShaderTokenType::Func,
					.Name = FuncName,
					.File = Node->getLoc().getFilenameStr(),
					.Location = {Node->getLoc().line, Node->getLoc().column},
					.Scope = ScopeStack.Top(),
				});
				Context.SymbolRefs.AddUnique(Context.SymbolTable.Last(), GlslSymbolRef{
					.Name = FuncName,
					.File = Node->getLoc().getFilenameStr(),
					.Location = {Node->getLoc().line, Node->getLoc().column}
				});
				ScopeStack.Push(FuncScope);
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

	private:
		GlslContext& Context;
		TArray<ShaderScope> ScopeStack;
	};

	class FRAMEWORK_API GlslTU : public ShaderTU
	{
	public:
		GlslTU(TRefCountPtr<GpuShader> InShader)
			: ShaderTU(InShader->GetSourceText())
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
			ParsedShader = GlslCompiler.ParseGlslToGlslangShader(TCHAR_TO_UTF8(*SourceText), MapShadercKind(InShader->GetShaderType()), "Temp.glsl", Options);

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
			return ParseDiagnosticInfoFromGlslang(Msg);
		}

		ShaderTokenType GetTokenType(ShaderTokenType InType, uint32 Row, uint32 Col, uint32 Size) override
		{
			if (InType == ShaderTokenType::Identifier)
			{
				FString TokenStr = GetStr(Row, Col, Size);
				for (const auto& [Def, Ref] : Context.SymbolRefs)
				{
					if (Ref.File == "Temp.glsl" && Row == (uint32)Ref.Location.X && Col >= (uint32)Ref.Location.Y && Col <= (uint32)Ref.Location.Y + Ref.Name.Len())
					{
						return Def.Kind;
					}
				}

				if (GLSL::GetBuiltinTypes(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::BuildtinType;
				}
				else if (GLSL::GetBuiltinFuncs(Stage).Contains(TokenStr))
				{
					return ShaderTokenType::BuildtinFunc;
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
				if (Ref.File == "Temp.glsl" && Row == (uint32)Ref.Location.X && Col >= (uint32)Ref.Location.Y && Col <= (uint32)Ref.Location.Y + Ref.Name.Len())
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
