#include "shaderc/shaderc.hpp"

namespace GLSL
{
	TArray<FString> KeyWords = {
		// Control flow
		"discard", "struct", "if", "else", "switch", "case", "default", "break", "return", "for", "while", "do", "continue",
		// Qualifiers
		"const", "in", "out", "inout", "uniform", "buffer", "shared", "coherent", "volatile", "restrict", "readonly", "writeonly",
		"layout", "location", "binding", "set", "push_constant", "input_attachment_index",
		"flat", "noperspective", "smooth", "centroid", "sample", "patch",
		"highp", "mediump", "lowp", "precision",
		"invariant", "precise",
		// Built-in variables
		"gl_Position", "gl_PointSize", "gl_ClipDistance", "gl_CullDistance",
		"gl_VertexIndex", "gl_InstanceIndex", "gl_VertexID", "gl_InstanceID",
		"gl_PrimitiveID", "gl_InvocationID", "gl_Layer", "gl_ViewportIndex",
		"gl_FragCoord", "gl_FrontFacing", "gl_PointCoord", "gl_SampleID", "gl_SamplePosition", "gl_SampleMaskIn", "gl_SampleMask",
		"gl_FragDepth", "gl_HelperInvocation",
		"gl_WorkGroupSize", "gl_WorkGroupID", "gl_LocalInvocationID", "gl_GlobalInvocationID", "gl_LocalInvocationIndex", "gl_NumWorkGroups",
		"gl_TessLevelOuter", "gl_TessLevelInner", "gl_TessCoord", "gl_PatchVerticesIn",
		"gl_in", "gl_out",
		// Misc
		"true", "false",
	};

	TArray<FString> BuiltinTypes = {
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
		// Images
		"image1D", "image2D", "image3D", "imageCube", "image2DRect", "imageBuffer",
		"image1DArray", "image2DArray", "imageCubeArray", "image2DMS", "image2DMSArray",
		"iimage1D", "iimage2D", "iimage3D", "iimageCube", "iimage2DRect", "iimageBuffer",
		"iimage1DArray", "iimage2DArray", "iimageCubeArray", "iimage2DMS", "iimage2DMSArray",
		"uimage1D", "uimage2D", "uimage3D", "uimageCube", "uimage2DRect", "uimageBuffer",
		"uimage1DArray", "uimage2DArray", "uimageCubeArray", "uimage2DMS", "uimage2DMSArray",
		// Textures (Vulkan)
		"texture1D", "texture2D", "texture3D", "textureCube", "texture2DRect", "textureBuffer",
		"texture1DArray", "texture2DArray", "textureCubeArray", "texture2DMS", "texture2DMSArray",
		"itexture1D", "itexture2D", "itexture3D", "itextureCube", "itexture2DRect", "itextureBuffer",
		"itexture1DArray", "itexture2DArray", "itextureCubeArray", "itexture2DMS", "itexture2DMSArray",
		"utexture1D", "utexture2D", "utexture3D", "utextureCube", "utexture2DRect", "utextureBuffer",
		"utexture1DArray", "utexture2DArray", "utextureCubeArray", "utexture2DMS", "utexture2DMSArray",
		// Subpass input (Vulkan)
		"subpassInput", "subpassInputMS", "isubpassInput", "isubpassInputMS", "usubpassInput", "usubpassInputMS",
		// Atomic counters
		"atomic_uint",
	};

	TArray<FString> BuiltinFuncs = {
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
		// Derivative
		"dFdx", "dFdy", "dFdxFine", "dFdyFine", "dFdxCoarse", "dFdyCoarse", "fwidth", "fwidthFine", "fwidthCoarse",
		// Interpolation
		"interpolateAtCentroid", "interpolateAtSample", "interpolateAtOffset",
		// Noise (deprecated but still valid)
		"noise1", "noise2", "noise3", "noise4",
		// Geometry Shader
		"EmitStreamVertex", "EndStreamPrimitive", "EmitVertex", "EndPrimitive",
		// Shader Invocation Control
		"barrier", "memoryBarrier", "memoryBarrierAtomicCounter", "memoryBarrierBuffer", "memoryBarrierImage", "memoryBarrierShared", "groupMemoryBarrier",
		// Subpass Input (Vulkan)
		"subpassLoad",
	};
}


namespace FW
{
	class FRAMEWORK_API GlslTU : public ShaderTU
	{
	public:
		GlslTU(const FString& InShaderSource, const TArray<FString>& IncludeDirs)
			: ShaderTU(InShaderSource)
		{

		}

		TArray<ShaderDiagnosticInfo> GetDiagnostic() override
		{
			return {};
		}

		ShaderTokenType GetTokenType(ShaderTokenType InType, uint32 Row, uint32 Col, uint32 Size) override
		{
			return {};
		}

		TArray<ShaderCandidateInfo> GetCodeComplete(uint32 Row, uint32 Col) override
		{
			return {};
		}

		TArray<ShaderOccurrence> GetOccurrences(uint32 Row, uint32 Col) override
		{
			return {};
		}

	private:
	};
}