#include "CommonHeader.h"
#include "ShaderCodeTokenizer.h"

namespace SH
{
	const char* Operators[] = {
		":", "+=", "++", "+", "--", "-=", "-", "(",
		"", "[", "]", ".", "->", "!=", "!",
		"&=", "~", "&", "*=", "*", "->", "/=",
		"/", "%=", "%", "<<=", "<<", "<=",
		"<", ">>=",	">>", ">=", ">", "==", "&&",
		"&", "^=", "^", "|=", "||", "|", "?",
		"=", ",",
	};

	const char* KeyWords[] = {
		"register", "packoffset", "static", "const",
		"break", "continue", "discard", "do", "for", "if", 
		"else", "switch", "while", "case", "default", "return", "true", "false",
		"SV_Coverage", "SV_Depth", "SV_DispatchThreadID", "SV_DomainLocation", "SV_GroupID", "SV_GroupIndex", "SV_GroupThreadID", "SV_GSInstanceID",
		"SV_InsideTessFactor", "SV_IsFrontFace", "SV_OutputControlPointID", "SV_POSITION", "SV_Position", "SV_RenderTargetArrayIndex",
		"SV_SampleIndex", "SV_TessFactor", "SV_ViewportArrayIndex", "SV_InstanceID", "SV_PrimitiveID", "SV_VertexID", "SV_TargetID",
		"SV_TARGET", "SV_Target", "SV_Target0", "SV_Target1", "SV_Target2", "SV_Target3", "SV_Target4", "SV_Target5", "SV_Target6", "SV_Target7",
	};

	const char* BuiltinTypes[] = {
		"bool", "bool1", "bool2", "bool3", "bool4", "bool1x1", "bool1x2", "bool1x3", "bool1x4",
		"bool2x1", "bool2x2", "bool2x3", "bool2x4", "bool3x1", "bool3x2", "bool3x3", "bool3x4",
		"bool4x1", "bool4x2", "bool4x3", "bool4x4",
		"int", "int1", "int2", "int3", "int4", "int1x1", "int1x2", "int1x3", "int1x4",
		"int2x1", "int2x2", "int2x3", "int2x4", "int3x1", "int3x2", "int3x3", "int3x4",
		"int4x1", "int4x2", "int4x3", "int4x4",
		"uint", "uint1", "uint2", "uint3", "uint4", "uint1x1", "uint1x2", "uint1x3", "uint1x4",
		"uint2x1", "uint2x2", "uint2x3", "uint2x4", "uint3x1", "uint3x2", "uint3x3", "uint3x4",
		"uint4x1", "uint4x2", "uint4x3", "uint4x4",
		"UINT", "UINT2", "UINT3", "UINT4",
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
		"snorm", "unorm", "string", "void", "cbuffer", "struct",
		"Buffer", "AppendStructuredBfufer", "ByteAddressBuffer", "ConsumeStructuredBuffer", "StructuredBuffer",
		"RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D",
		"InputPatch", "OutputPatch",
		"linear", "centroid", "nointerpolation", "noperspective", "sample",
		"sampler", "sampler1D", "sampler2D", "sampler3D", "samplerCUBE", "SamplerComparisonState", "SamplerState", "sampler_state",
		"AddressU", "AddressV", "AddressW", "BorderColor", "Filter", "MaxAnisotropy", "MaxLOD", "MinLOD", "MipLODBias", "ComparisonFunc", "ComparisonFilter",
		"texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS", "Texture2DMSArray", "Texture3D", "TextureCube",
	};

	const char* BuiltinFuncs[] = {
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
		"texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc",
	};

	TArray<HlslHighLightTokenizer::TokenizedLine> HlslHighLightTokenizer::Tokenize(const FString& HlslCodeString)
	{
		TArray<HlslHighLightTokenizer::TokenizedLine> TokenizedLines;
		TArray<FTextRange> LineRanges;
		FTextRange::CalculateLineRangesFromString(HlslCodeString, LineRanges);

		for (const FTextRange& LineRange : LineRanges)
		{
			HlslHighLightTokenizer::TokenizedLine TokenizedLine;
			TokenizedLine.LineRange = LineRange;
			
			//DFA
		}

		return TokenizedLines;
	}

}