// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

PRAGMA_DISABLE_SHADOW_VARIABLE_WARNINGS

#include <type_traits>

// Include the intrinsic functions header
#if ((PLATFORM_WINDOWS || PLATFORM_HOLOLENS) && PLATFORM_64BITS)
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif

#include "Math/Float16.h"

/*=============================================================================
 *	Helpers:
 *============================================================================*/

#ifdef _MSC_VER

// MSVC NEON headers typedef float32x4_t and int32x4_t both to __n128
// This wrapper type allows VectorRegister4Float and VectorRegister4Int to be
// discriminated for template specialization (e.g. FConstantHandler)
//
// This comes at the cost of having to define constructors for some
// anonymous unions, because VectorRegister4Float/VectorRegister4Int are no
// longer trivially constructible. The optimizer should eliminate the
// redundant zero initialization in these cases for non-MSVC (e.g. V()
// is called now where it wasn't before)
template<typename T, typename BASE_TYPE>
struct alignas(alignof(T)) VectorRegisterWrapper
{
	FORCEINLINE VectorRegisterWrapper() {}
	FORCEINLINE constexpr VectorRegisterWrapper(T vec) : m_vec(vec) {}

	FORCEINLINE operator T&() { return m_vec; }
	FORCEINLINE operator const T&() const { return m_vec; }

	FORCEINLINE BASE_TYPE operator[](int Index) const;

	T m_vec;
};

template<>
FORCEINLINE float VectorRegisterWrapper<float32x4_t, float>::operator[](int Index) const
{
	return m_vec.n128_f32[Index];
}

template<>
FORCEINLINE double VectorRegisterWrapper<float64x2_t, double>::operator[](int Index) const
{
	return m_vec.n128_f64[Index];
}

template<>
FORCEINLINE int VectorRegisterWrapper<int32x4_t, int>::operator[](int Index) const
{
	return m_vec.n128_i32[Index];
}

template<>
FORCEINLINE int64 VectorRegisterWrapper<int64x2_t, int64>::operator[](int Index) const
{
	return m_vec.n128_i64[Index];
}

/** 16-byte vector register type */
typedef VectorRegisterWrapper<float32x4_t, float> VectorRegister4Float;
typedef VectorRegisterWrapper<float64x2_t, double> VectorRegister2Double;
typedef VectorRegisterWrapper<int32x4_t, int> VectorRegister4Int;
typedef VectorRegisterWrapper<int64x2_t, int64> VectorRegister2Int64;

FORCEINLINE constexpr VectorRegister4Int MakeVectorRegisterIntConstant(int32 X, int32 Y, int32 Z, int32 W)
{
    int32x4_t Out = {};
	Out.n128_i32[0] = X;
    Out.n128_i32[1] = Y;
    Out.n128_i32[2] = Z;
    Out.n128_i32[3] = W;
    return Out;
}

FORCEINLINE constexpr VectorRegister4Float MakeVectorRegisterFloatConstant(float X, float Y, float Z, float W)
{
    float32x4_t Out = {};
    Out.n128_f32[0] = X;
    Out.n128_f32[1] = Y;
    Out.n128_f32[2] = Z;
    Out.n128_f32[3] = W;
	return Out;
}

FORCEINLINE constexpr VectorRegister2Double MakeVectorRegister2DoubleConstant(double X, double Y)
{
    float64x2_t Out = {};
    Out.n128_f64[0] = X;
    Out.n128_f64[1] = Y;
	return Out;
}

#else

/** 16-byte vector register type */
typedef float32x4_t GCC_ALIGN(16) VectorRegister4Float;
typedef float64x2_t GCC_ALIGN(16) VectorRegister2Double;
typedef int32x4_t  GCC_ALIGN(16) VectorRegister4Int;
typedef int64x2_t GCC_ALIGN(16) VectorRegister2Int64;

FORCEINLINE constexpr VectorRegister4Int MakeVectorRegisterIntConstant(int32 X, int32 Y, int32 Z, int32 W)
{
    return VectorRegister4Int { X, Y, Z, W };
}

FORCEINLINE constexpr VectorRegister4Float MakeVectorRegisterFloatConstant(float X, float Y, float Z, float W)
{
    return VectorRegister4Float { X, Y, Z, W };
}

FORCEINLINE constexpr VectorRegister2Double MakeVectorRegister2DoubleConstant(double X, double Y)
{
    return VectorRegister2Double { X, Y };
}

#endif

#define DECLARE_VECTOR_REGISTER(X, Y, Z, W) MakeVectorRegister( X, Y, Z, W )

struct alignas(16) VectorRegister4Double
{
	struct
	{
		VectorRegister2Double XY;
		VectorRegister2Double ZW;
	};

	FORCEINLINE VectorRegister4Double() = default;
	FORCEINLINE VectorRegister4Double(const VectorRegister2Double& xy, const VectorRegister2Double& zw) : XY(xy), ZW(zw) {}
	FORCEINLINE constexpr VectorRegister4Double(VectorRegister2Double xy, VectorRegister2Double zw, VectorRegisterConstInit) : XY(xy), ZW(zw) {}

	FORCEINLINE VectorRegister4Double(const VectorRegister4Float& From)
	{
		XY = vcvt_f64_f32(*(float32x2_t*)&From);
		ZW = vcvt_high_f64_f32(From);
	}

	VectorRegister4Double(const VectorRegister2Double& From) = delete;

	FORCEINLINE VectorRegister4Double& operator=(const VectorRegister4Float& From)
	{
		*this = VectorRegister4Double(From);
		return *this;
	}
};

typedef VectorRegister4Double VectorRegister;
#define VectorZeroVectorRegister() VectorZeroDouble()
#define VectorOneVectorRegister() VectorOneDouble()

// Forward declarations
VectorRegister4Float VectorLoadAligned(const float* Ptr);
VectorRegister4Double VectorLoadAligned(const double* Ptr);
void VectorStoreAligned(const VectorRegister4Float& Vec, float* Ptr);
void VectorStoreAligned(const VectorRegister4Double& Vec, double* Dst);


// Helper for conveniently aligning a float array for extraction from VectorRegister4Float
struct alignas(alignof(VectorRegister4Float)) AlignedFloat4
{
	float V[4];

	FORCEINLINE AlignedFloat4(const VectorRegister4Float& Vec)
	{
		VectorStoreAligned(Vec, V);
	}

	FORCEINLINE float operator[](int32 Index) const { return V[Index]; }
	FORCEINLINE float& operator[](int32 Index) { return V[Index]; }

	FORCEINLINE VectorRegister4Float ToVectorRegister() const
	{
		return VectorLoadAligned(V);
	}
};


// Helper for conveniently aligning a double array for extraction from VectorRegister4Double
struct alignas(alignof(VectorRegister4Double)) AlignedDouble4
{
	double V[4];

	FORCEINLINE AlignedDouble4(const VectorRegister4Double& Vec)
	{
		VectorStoreAligned(Vec, V);
	}

	FORCEINLINE double operator[](int32 Index) const { return V[Index]; }
	FORCEINLINE double& operator[](int32 Index) { return V[Index]; }

	FORCEINLINE VectorRegister4Double ToVectorRegister() const
	{
		return VectorLoadAligned(V);
	}
};

typedef AlignedDouble4 AlignedRegister4;
// Aliases
typedef VectorRegister4Int VectorRegister4i;
typedef VectorRegister4Float VectorRegister4f;
typedef VectorRegister4Double VectorRegister4d;
typedef VectorRegister2Double VectorRegister2d;

/**
 * Returns a bitwise equivalent vector based on 4 uint32s.
 *
 * @param X		1st uint32 component
 * @param Y		2nd uint32 component
 * @param Z		3rd uint32 component
 * @param W		4th uint32 component
 * @return		Bitwise equivalent vector with 4 floats
 */
FORCEINLINE VectorRegister4Float MakeVectorRegister( uint32 X, uint32 Y, uint32 Z, uint32 W )
{
	union U {
		VectorRegister4Float V; uint32 F[4];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.F[0] = X;
	Tmp.F[1] = Y;
	Tmp.F[2] = Z;
	Tmp.F[3] = W;
	return Tmp.V;
}

FORCEINLINE VectorRegister4Float MakeVectorRegisterFloat(uint32 X, uint32 Y, uint32 Z, uint32 W)
{
	return MakeVectorRegister(X, Y, Z, W);
}

// Nicer alias
FORCEINLINE VectorRegister4Float MakeVectorRegisterFloatMask(uint32 X, uint32 Y, uint32 Z, uint32 W)
{
	return MakeVectorRegisterFloat(X, Y, Z, W);
}


/**
 * Returns a vector based on 4 floats.
 *
 * @param X		1st float component
 * @param Y		2nd float component
 * @param Z		3rd float component
 * @param W		4th float component
 * @return		Vector of the 4 floats
 */
FORCEINLINE VectorRegister4Float MakeVectorRegister( float X, float Y, float Z, float W )
{
	union U {
		VectorRegister4Float V; float F[4];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.F[0] = X;
	Tmp.F[1] = Y;
	Tmp.F[2] = Z;
	Tmp.F[3] = W;
	return Tmp.V;
}

FORCEINLINE VectorRegister4Float MakeVectorRegisterFloat(float X, float Y, float Z, float W)
{
	return MakeVectorRegister(X, Y, Z, W);
}

/**
 * Returns a vector based on 4 doubles.
 *
 * @param X		1st double component
 * @param Y		2nd double component
 * @param Z		3rd double component
 * @param W		4th double component
 * @return		Vector of the 4 doubles
 */
FORCEINLINE VectorRegister4Double MakeVectorRegister(double X, double Y, double Z, double W)
{
	union U
	{
		VectorRegister4Double V; double D[4];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.D[0] = X;
	Tmp.D[1] = Y;
	Tmp.D[2] = Z;
	Tmp.D[3] = W;
	return Tmp.V;
}

FORCEINLINE VectorRegister4Double MakeVectorRegisterDouble(double X, double Y, double Z, double W)
{
	return MakeVectorRegister(X, Y, Z, W);
}

FORCEINLINE VectorRegister4Double MakeVectorRegisterDouble(const VectorRegister2Double& XY, const VectorRegister2Double& ZW)
{
	return VectorRegister4Double(XY, ZW);
}

FORCEINLINE VectorRegister4Double MakeVectorRegisterDouble(uint64 X, uint64 Y, uint64 Z, uint64 W)
{
	union U
	{
		VectorRegister4Double V; uint64_t D[4];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.D[0] = X;
	Tmp.D[1] = Y;
	Tmp.D[2] = Z;
	Tmp.D[3] = W;
	return Tmp.V;
}

// Nicer alias
FORCEINLINE VectorRegister4Double MakeVectorRegisterDoubleMask(uint64 X, uint64 Y, uint64 Z, uint64 W)
{
	return MakeVectorRegisterDouble(X, Y, Z, W);
}

FORCEINLINE VectorRegister2Double MakeVectorRegister2Double(double X, double Y)
{
	union U
	{
		VectorRegister2Double V; double D[2];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.D[0] = X;
	Tmp.D[1] = Y;
	return Tmp.V;
}

FORCEINLINE VectorRegister2Double MakeVectorRegister2Double(uint64 X, uint64 Y)
{
	union U
	{
		VectorRegister2Double V; uint64_t D[2];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.D[0] = X;
	Tmp.D[1] = Y;
	return Tmp.V;
}

/**
* Returns a vector based on 4 int32.
*
* @param X		1st int32 component
* @param Y		2nd int32 component
* @param Z		3rd int32 component
* @param W		4th int32 component
* @return		Vector of the 4 int32
*/
FORCEINLINE VectorRegister4Int MakeVectorRegisterInt(int32 X, int32 Y, int32 Z, int32 W)
{
	union U {
		VectorRegister4Int V; int32 I[4];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.I[0] = X;
	Tmp.I[1] = Y;
	Tmp.I[2] = Z;
	Tmp.I[3] = W;
	return Tmp.V;
}

FORCEINLINE VectorRegister2Int64 MakeVectorRegisterInt64(int64 X, int64 Y)
{
	union U
	{
		VectorRegister2Int64 V; int64 I[2];
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.I[0] = X;
	Tmp.I[1] = Y;
	return Tmp.V;
}

// Make double register from float register
FORCEINLINE VectorRegister4Double MakeVectorRegisterDouble(const VectorRegister4Float& From)
{
	return VectorRegister4Double(From);
}

// Lossy conversion: double->float vector
FORCEINLINE VectorRegister4Float MakeVectorRegisterFloatFromDouble(const VectorRegister4Double& Vec)
{
	return vcvt_high_f32_f64(vcvt_f32_f64(Vec.XY), Vec.ZW);
}

/*
#define VectorPermute(Vec1, Vec2, Mask) my_perm(Vec1, Vec2, Mask)

/ ** Reads NumBytesMinusOne+1 bytes from the address pointed to by Ptr, always reading the aligned 16 bytes containing the start of Ptr, but only reading the next 16 bytes if the data straddles the boundary * /
FORCEINLINE VectorRegister4Float VectorLoadNPlusOneUnalignedBytes(const void* Ptr, int NumBytesMinusOne)
{
	return VectorPermute( my_ld (0, (float*)Ptr), my_ld(NumBytesMinusOne, (float*)Ptr), my_lvsl(0, (float*)Ptr) );
}
*/


/*=============================================================================
 *	Constants:
 *============================================================================*/

#include "Math/UnrealMathVectorConstants.h"


/*=============================================================================
 *	Intrinsics:
 *============================================================================*/

/**
 * Returns a vector with all zeros.
 *
 * @return		VectorRegister4Float(0.0f, 0.0f, 0.0f, 0.0f)
 */
FORCEINLINE VectorRegister4Float VectorZeroFloat()
{	
	return vdupq_n_f32( 0.0f );
}

FORCEINLINE VectorRegister4Double VectorZeroDouble()
{
	VectorRegister2Double Zero = vdupq_n_f64(0.0);
	return VectorRegister4Double(Zero, Zero);
}


/**
 * Returns a vector with all ones.
 *
 * @return		VectorRegister4Float(1.0f, 1.0f, 1.0f, 1.0f)
 */
FORCEINLINE VectorRegister4Float VectorOneFloat()
{
	return vdupq_n_f32( 1.0f );
}

FORCEINLINE VectorRegister4Double VectorOneDouble()
{
	VectorRegister4Double Result;
	Result.XY = vdupq_n_f64(1.0f);
	Result.ZW = Result.XY;
	return Result;
}

/**
 * Loads 4 floats from unaligned memory.
 *
 * @param Ptr	Unaligned memory pointer to the 4 floats
 * @return		VectorRegister4Float(Ptr[0], Ptr[1], Ptr[2], Ptr[3])
 */
FORCEINLINE VectorRegister4Float VectorLoad(const float* Ptr)
{
	return vld1q_f32( (float32_t*)Ptr );
}

FORCEINLINE VectorRegister4Double VectorLoad(const double* Ptr)
{
	float64x2x2_t Vec = vld1q_f64_x2(Ptr);
	VectorRegister4Double Result = *(VectorRegister4Double*)&Vec;
	return Result;
}

/**
 * Loads 2 floats from unaligned memory into X and Y and duplicates them in Z and W.
 *
 * @param Ptr	Unaligned memory pointer to the floats
 * @return		VectorRegister4Float(Ptr[0], Ptr[1], Ptr[0], Ptr[1])
 */
FORCEINLINE VectorRegister4Float VectorLoadFloat2(const float* Ptr)
{
	return MakeVectorRegister(Ptr[0], Ptr[1], Ptr[0], Ptr[1]);
}

/**
 * Loads 3 floats from unaligned memory and leaves W undefined.
 *
 * @param Ptr	Unaligned memory pointer to the 3 floats
 * @return		VectorRegister4Float(Ptr[0], Ptr[1], Ptr[2], 0.0f)
 */
FORCEINLINE VectorRegister4Double VectorLoadFloat3(const double* Ptr)
{
	union U
	{
		VectorRegister4Double V; double D[4];
		inline U() : V() {}
	} Tmp;

	Tmp.V.XY = vld1q_f64(Ptr);
	Tmp.D[2] = Ptr[2];
	Tmp.D[3] = 0.0;
	return Tmp.V;
}

/**
 * Loads 3 FLOATs from unaligned memory and sets W=1.
 *
 * @param Ptr	Unaligned memory pointer to the 3 FLOATs
 * @return		VectorRegister4Float(Ptr[0], Ptr[1], Ptr[2], 1.0f)
 */
FORCEINLINE VectorRegister4Double VectorLoadFloat3_W1(const double* Ptr)
{
	return MakeVectorRegisterDouble(Ptr[0], Ptr[1], Ptr[2], 1.0f);
}

/**
 * Sets a single component of a vector. Must be a define since ElementIndex needs to be a constant integer
 */
template <int ElementIndex>
FORCEINLINE VectorRegister4Float VectorSetComponentImpl(const VectorRegister4Float& Vec, float Scalar)
{
	return vsetq_lane_f32(Scalar, Vec, ElementIndex);
}

template <int ElementIndex>
FORCEINLINE VectorRegister2Double VectorSetComponentImpl(const VectorRegister2Double& Vec, double Scalar)
{
	return vsetq_lane_f64(Scalar, Vec, ElementIndex);
}

template<int ElementIndex, typename std::enable_if< (ElementIndex > 1), bool >::type = true >
FORCEINLINE VectorRegister4Double VectorSetComponentImpl(const VectorRegister4Double& Vec, double Scalar)
{
	VectorRegister4Double Result;
	Result.XY = Vec.XY;
	Result.ZW = VectorSetComponentImpl<ElementIndex - 2>(Vec.ZW, Scalar);
	return Result;
}

template<int ElementIndex, typename std::enable_if < (ElementIndex <= 1), bool >::type = true >
FORCEINLINE VectorRegister4Double VectorSetComponentImpl(const VectorRegister4Double& Vec, double Scalar)
{
	VectorRegister4Double Result;
	Result.XY = VectorSetComponentImpl<ElementIndex>(Vec.XY, Scalar);
	Result.ZW = Vec.ZW;
	return Result;
}

#define VectorSetComponent( Vec, ElementIndex, Scalar ) VectorSetComponentImpl<ElementIndex>(Vec, Scalar)


/**
 * Loads 4 floats from aligned memory.
 *
 * @param Ptr	Aligned memory pointer to the 4 floats
 * @return		VectorRegister4Float(Ptr[0], Ptr[1], Ptr[2], Ptr[3])
 */
FORCEINLINE VectorRegister4Float VectorLoadAligned( const float* Ptr )
{
	return VectorLoad(Ptr);
}

FORCEINLINE VectorRegister4Double VectorLoadAligned(const double* Ptr)
{
	return VectorLoad(Ptr);
}

/**
 * Loads 1 float from unaligned memory and replicates it to all 4 elements.
 *
 * @param Ptr	Unaligned memory pointer to the float
 * @return		VectorRegister4Float(Ptr[0], Ptr[0], Ptr[0], Ptr[0])
 */
FORCEINLINE VectorRegister4Float VectorLoadFloat1( const float *Ptr )
{
	return vdupq_n_f32(Ptr[0]);
}

FORCEINLINE VectorRegister4Double VectorLoadDouble1(const double* Ptr)
{
	VectorRegister4Double Result;
	Result.XY = vdupq_n_f64(Ptr[0]);
	Result.ZW = Result.XY;
	return Result;
}

/**
 * Loads 4 unaligned floats - 2 from the first pointer, 2 from the second, and packs
 * them in to 1 vector.
 *
 * @param Ptr1	Unaligned memory pointer to the first 2 floats
 * @param Ptr2	Unaligned memory pointer to the second 2 floats
 * @return		VectorRegister4Float(Ptr1[0], Ptr1[1], Ptr2[0], Ptr2[1])
 */
FORCEINLINE VectorRegister4Float VectorLoadTwoPairsFloat(const float* Ptr1, const float* Ptr2)
{
	float32x2_t Lo = vld1_f32(Ptr1);
	float32x2_t Hi = vld1_f32(Ptr2);
	return vcombine_f32(Lo, Hi);
}

FORCEINLINE VectorRegister4Double VectorLoadTwoPairsFloat(const double* Ptr1, const double* Ptr2)
{
	VectorRegister4Double Res;
	Res.XY = vld1q_f64(Ptr1);
	Res.ZW = vld1q_f64(Ptr2);
	return Res;
}

/**
* Propagates passed in float to all registers.
*
* @param X		float component
* @return		VectorRegister4Float(X, X, X, X)
*/
FORCEINLINE VectorRegister4Float VectorSetFloat1(float X)
{
	return vdupq_n_f32(X);
}

FORCEINLINE VectorRegister4Double VectorSetFloat1(double X)
{
	VectorRegister4Double Result;
	Result.XY = vdupq_n_f64(X);
	Result.ZW = Result.XY;
	return Result;
}

/**
 * Stores a vector to aligned memory.
 *
 * @param Vec	Vector to store
 * @param Ptr	Aligned memory pointer
 */
FORCEINLINE void VectorStoreAligned(const VectorRegister4Float& Vec, float* Ptr)
{
	vst1q_f32(Ptr, Vec);
}

FORCEINLINE void VectorStoreAligned(const VectorRegister4Double& Vec, double* Ptr)
{
	vst1q_f64_x2(Ptr, *(float64x2x2_t*)&Vec);
}

//TODO: LWC VectorVM.cpp calls it on a line 3294, case EVectorVMOp::outputdata_half: Context.WriteExecFunction(CopyConstantToOutput<float, FFloat16, 2>); break;
FORCEINLINE void VectorStoreAligned(VectorRegister4Float Vec, FFloat16* Ptr)
{
	AlignedFloat4 Floats(Vec);
	for (int i = 0; i < 4; ++i)
	{
		Ptr[i] = Floats[i];
	}
}

/**
* Same as VectorStoreAligned for Neon. 
*
* @param Vec	Vector to store
* @param Ptr	Aligned memory pointer
*/
#define VectorStoreAlignedStreamed( Vec, Ptr )	VectorStoreAligned( Vec, Ptr )

/**
 * Stores a vector to memory (aligned or unaligned).
 *
 * @param Vec	Vector to store
 * @param Ptr	Memory pointer
 */
FORCEINLINE void VectorStore(const VectorRegister4Float& Vec, float* Ptr)
{
	vst1q_f32(Ptr, Vec);
}

FORCEINLINE void VectorStore(const VectorRegister4Double& Vec, double* Ptr)
{
	vst1q_f64_x2(Ptr, *(float64x2x2_t*)&Vec);
}

/**
 * Stores the XYZ components of a vector to unaligned memory.
 *
 * @param Vec	Vector to store XYZ
 * @param Ptr	Unaligned memory pointer
 */
FORCEINLINE void VectorStoreFloat3( const VectorRegister4Float& Vec, float* Ptr )
{
	vst1_f32(Ptr, *(float32x2_t*)&Vec);
	vst1q_lane_f32(((float32_t*)Ptr) + 2, Vec, 2);
}

/**
 * Stores the XYZ components of a double vector pair to unaligned memory.
 *
 * @param Vec	Vector to store XYZ
 * @param Ptr	Unaligned memory pointer
 */
FORCEINLINE void VectorStoreFloat3(const VectorRegister4Double& Vec, double* Ptr)
{
	vst1q_f64(Ptr, Vec.XY);
	vst1q_lane_f64(((float64_t*)Ptr) + 2, Vec.ZW, 0);
}


/**
 * Stores the X component of a vector to unaligned memory.
 *
 * @param Vec	Vector to store X
 * @param Ptr	Unaligned memory pointer
 */
FORCEINLINE void VectorStoreFloat1(VectorRegister4Float Vec, float* Ptr )
{
	vst1q_lane_f32( Ptr, Vec, 0 );
}

FORCEINLINE void VectorStoreFloat1(const VectorRegister4Double& Vec, double* Ptr)
{
	vst1q_lane_f64(Ptr, Vec.XY, 0);
}

/**
 * Replicates one element into all four elements and returns the new vector. Must be a #define for ELementIndex
 * to be a constant integer
 *
 * @param Vec			Source vector
 * @param ElementIndex	Index (0-3) of the element to replicate
 * @return				VectorRegister4Float( Vec[ElementIndex], Vec[ElementIndex], Vec[ElementIndex], Vec[ElementIndex] )
 */
template <int ElementIndex>
FORCEINLINE VectorRegister4Float VectorReplicateImpl(const VectorRegister4Float& Vec)
{
	return vdupq_n_f32(vgetq_lane_f32(Vec, ElementIndex));
}

template <int ElementIndex>
FORCEINLINE VectorRegister2Double VectorReplicateImpl(const VectorRegister2Double& Vec)
{
	return vdupq_n_f64(vgetq_lane_f64(Vec, ElementIndex));
}

template <int ElementIndex, typename std::enable_if < (ElementIndex <= 1), bool >::type = true >
FORCEINLINE VectorRegister4Double VectorReplicateImpl(const VectorRegister4Double& Vec)
{
	VectorRegister4Double Result;
	Result.XY = VectorReplicateImpl<ElementIndex>(Vec.XY);
	Result.ZW = Result.XY;
	return Result;
}

template <int ElementIndex, typename std::enable_if < (ElementIndex > 1), bool >::type = true >
FORCEINLINE VectorRegister4Double VectorReplicateImpl(const VectorRegister4Double& Vec)
{
	VectorRegister4Double Result;
	Result.ZW = VectorReplicateImpl<ElementIndex - 2>(Vec.ZW);
	Result.XY = Result.ZW;
	return Result;
}

#define VectorReplicate( Vec, ElementIndex ) VectorReplicateImpl<ElementIndex>(Vec)


/**
 * Returns the absolute value (component-wise).
 *
 * @param Vec			Source vector
 * @return				VectorRegister4Float( abs(Vec.x), abs(Vec.y), abs(Vec.z), abs(Vec.w) )
 */
FORCEINLINE VectorRegister4Float VectorAbs(VectorRegister4Float Vec )
{
	return vabsq_f32( Vec );
}

FORCEINLINE VectorRegister4Double VectorAbs(VectorRegister4Double Vec)
{
	VectorRegister4Double Result;
	Result.XY = vabsq_f64(Vec.XY);
	Result.ZW = vabsq_f64(Vec.ZW);
	return Result;
}

/**
 * Returns the negated value (component-wise).
 *
 * @param Vec			Source vector
 * @return				VectorRegister4Float( -Vec.x, -Vec.y, -Vec.z, -Vec.w )
 */
FORCEINLINE VectorRegister4Float VectorNegate( VectorRegister4Float Vec )
{
	return vnegq_f32( Vec );
}

FORCEINLINE VectorRegister4Double VectorNegate(VectorRegister4Double Vec)
{
	VectorRegister4Double Result;
	Result.XY = vnegq_f64(Vec.XY);
	Result.ZW = vnegq_f64(Vec.ZW);
	return Result;
}

/**
 * Adds two vectors (component-wise) and returns the result.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( Vec1.x+Vec2.x, Vec1.y+Vec2.y, Vec1.z+Vec2.z, Vec1.w+Vec2.w )
 */
FORCEINLINE VectorRegister4Float VectorAdd( VectorRegister4Float Vec1, VectorRegister4Float Vec2 )
{
	return vaddq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorAdd(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister4Double Result;
	Result.XY = vaddq_f64(Vec1.XY, Vec2.XY);
	Result.ZW = vaddq_f64(Vec1.ZW, Vec2.ZW);
	return Result;
}


/**
 * Subtracts a vector from another (component-wise) and returns the result.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( Vec1.x-Vec2.x, Vec1.y-Vec2.y, Vec1.z-Vec2.z, Vec1.w-Vec2.w )
 */
FORCEINLINE VectorRegister4Float VectorSubtract( VectorRegister4Float Vec1, VectorRegister4Float Vec2 )
{
	return vsubq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorSubtract(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister4Double Res;
	Res.XY = vsubq_f64(Vec1.XY, Vec2.XY);
	Res.ZW = vsubq_f64(Vec1.ZW, Vec2.ZW);
	return Res;
}


/**
 * Multiplies two vectors (component-wise) and returns the result.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( Vec1.x*Vec2.x, Vec1.y*Vec2.y, Vec1.z*Vec2.z, Vec1.w*Vec2.w )
 */
FORCEINLINE VectorRegister4Float VectorMultiply( VectorRegister4Float Vec1, VectorRegister4Float Vec2 ) 
{
	return vmulq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister2Double VectorMultiply(VectorRegister2Double Vec1, VectorRegister2Double Vec2)
{
	return vmulq_f64(Vec1, Vec2);
}

FORCEINLINE VectorRegister4Double VectorMultiply(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister4Double Result;
	Result.XY = vmulq_f64(Vec1.XY, Vec2.XY);
	Result.ZW = vmulq_f64(Vec1.ZW, Vec2.ZW);
	return Result;
}


/**
* Divides two vectors (component-wise) and returns the result.
*
* @param Vec1	1st vector
* @param Vec2	2nd vector
* @return		VectorRegister4Float( Vec1.x/Vec2.x, Vec1.y/Vec2.y, Vec1.z/Vec2.z, Vec1.w/Vec2.w )
*/
FORCEINLINE VectorRegister4Float VectorDivide(VectorRegister4Float Vec1, VectorRegister4Float Vec2)
{
	return vdivq_f32(Vec1, Vec2);
}

FORCEINLINE VectorRegister4Double VectorDivide(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister4Double Res;
	Res.XY = vdivq_f64(Vec1.XY, Vec2.XY);
	Res.ZW = vdivq_f64(Vec1.ZW, Vec2.ZW);
	return Res;
}


/**
 * Multiplies two vectors (component-wise), adds in the third vector and returns the result.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @param Acc	3rd vector
 * @return		VectorRegister4Float( Vec1.x*Vec2.x + Acc.x, Vec1.y*Vec2.y + Acc.y, Vec1.z*Vec2.z + Acc.z, Vec1.w*Vec2.w + Acc.w )
 */
FORCEINLINE VectorRegister4Float VectorMultiplyAdd( VectorRegister4Float Vec1, VectorRegister4Float Vec2, VectorRegister4Float Acc )
{
	return vfmaq_f32(Acc, Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorMultiplyAdd(VectorRegister4Double Vec1, VectorRegister4Double Vec2, VectorRegister4Double Acc)
{
	VectorRegister4Double Result;
	Result.XY = vfmaq_f64(Acc.XY, Vec1.XY, Vec2.XY);
	Result.ZW = vfmaq_f64(Acc.ZW, Vec1.ZW, Vec2.ZW);
	return Result;
}

/**
 * Multiplies two vectors (component-wise) and subtracts the result from the third vector.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @param Sub	3rd vector
 * @return		VectorRegister4Float( Sub.x - Vec1.x*Vec2.x, Sub.y - Vec1.y*Vec2.y, Sub.z - Vec1.z*Vec2.z, Sub.w - Vec1.w*Vec2.w )
 */
FORCEINLINE VectorRegister4Float VectorNegateMultiplyAdd(VectorRegister4Float Vec1, VectorRegister4Float Vec2, VectorRegister4Float Sub)
{
	return vfmsq_f32(Sub, Vec1, Vec2);
}

FORCEINLINE VectorRegister4Double VectorNegateMultiplyAdd(VectorRegister4Double Vec1, VectorRegister4Double Vec2, VectorRegister4Double Sub)
{
	VectorRegister4Double Result;
	Result.XY = vfmsq_f64(Sub.XY, Vec1.XY, Vec2.XY);
	Result.ZW = vfmsq_f64(Sub.ZW, Vec1.ZW, Vec2.ZW);
	return Result;
}


/**
 * Calculates the dot3 product of two vectors and returns a vector with the result in all 4 components.
 * Only really efficient on Xbox 360.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		d = dot3(Vec1.xyz, Vec2.xyz), VectorRegister4Float( d, d, d, d )
 */
FORCEINLINE VectorRegister4Float VectorDot3( const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	VectorRegister4Float Temp = VectorMultiply( Vec1, Vec2 );
	Temp = vsetq_lane_f32( 0.0f, Temp, 3 );
	float32x2_t sum = vpadd_f32( vget_low_f32( Temp ), vget_high_f32( Temp ) );
	sum = vpadd_f32( sum, sum );
	return vdupq_lane_f32( sum, 0 );
}

FORCEINLINE VectorRegister4Double VectorDot3(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister2Double A, B;
	A = vmulq_f64(Vec1.XY, Vec2.XY);
	B = vfmaq_f64(A, Vec1.ZW, Vec2.ZW);
	float64x1_t Sum = vadd_f64(vget_low_f64(B), vget_high_f64(A));
	VectorRegister4Double Temp;
	Temp.XY = vdupq_lane_f64(Sum, 0);
	Temp.ZW = Temp.XY;
	return Temp;
}

FORCEINLINE float VectorDot3Scalar(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2)
{
	return vgetq_lane_f32(VectorDot3(Vec1, Vec2), 0);
}

FORCEINLINE double VectorDot3Scalar(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister2Double A, B;
	A = vmulq_f64(Vec1.XY, Vec2.XY);
	B = vfmaq_f64(A, Vec1.ZW, Vec2.ZW);
	float64x1_t Sum = vadd_f64(vget_low_f64(B), vget_high_f64(A));
	return *(double*)&Sum;
}



/**
 * Calculates the dot4 product of two vectors and returns a vector with the result in all 4 components.
 * Only really efficient on Xbox 360.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		d = dot4(Vec1.xyzw, Vec2.xyzw), VectorRegister4Float( d, d, d, d )
 */
FORCEINLINE VectorRegister4Float VectorDot4(VectorRegister4Float Vec1, VectorRegister4Float Vec2)
{
	VectorRegister4Float Temp = VectorMultiply(Vec1, Vec2);
	float32x2_t sum = vpadd_f32(vget_low_f32(Temp), vget_high_f32(Temp));
	sum = vpadd_f32(sum, sum);
	return vdupq_lane_f32(sum, 0);
}

FORCEINLINE VectorRegister4Double VectorDot4(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister2Double A, B;
	A = vmulq_f64(Vec1.XY, Vec2.XY);
	B = vfmaq_f64(A, Vec1.ZW, Vec2.ZW);
	A = vextq_f64(B, B, 1);
	VectorRegister4Double Temp;
	Temp.XY = vaddq_f64(A, B);
	Temp.ZW = Temp.XY;
	return Temp;
}

/**
 * Creates a four-part mask based on component-wise == compares of the input vectors
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( Vec1.x == Vec2.x ? 0xFFFFFFFF : 0, same for yzw )
 */

FORCEINLINE VectorRegister4Float VectorCompareEQ( const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return (VectorRegister4Float)vceqq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorCompareEQ(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = (VectorRegister2Double)vceqq_f64(Vec1.XY, Vec2.XY);
	Result.ZW = (VectorRegister2Double)vceqq_f64(Vec1.ZW, Vec2.ZW);
	return Result;
}



/**
 * Creates a four-part mask based on component-wise != compares of the input vectors
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( Vec1.x != Vec2.x ? 0xFFFFFFFF : 0, same for yzw )
 */

FORCEINLINE VectorRegister4Float VectorCompareNE( const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return (VectorRegister4Float)vmvnq_u32( vceqq_f32( Vec1, Vec2 ) );
}

FORCEINLINE VectorRegister4Double VectorCompareNE(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = (VectorRegister2Double)vmvnq_u32(vceqq_f64(Vec1.XY, Vec2.XY));
	Result.ZW = (VectorRegister2Double)vmvnq_u32(vceqq_f64(Vec1.ZW, Vec2.ZW));
	return Result;
}

/**
 * Creates a four-part mask based on component-wise > compares of the input vectors
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( Vec1.x > Vec2.x ? 0xFFFFFFFF : 0, same for yzw )
 */

FORCEINLINE VectorRegister4Float VectorCompareGT( const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return (VectorRegister4Float)vcgtq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorCompareGT(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = (VectorRegister2Double)vcgtq_f64(Vec1.XY, Vec2.XY);
	Result.ZW = (VectorRegister2Double)vcgtq_f64(Vec1.ZW, Vec2.ZW);
	return Result;
}

/**
 * Creates a four-part mask based on component-wise >= compares of the input vectors
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( Vec1.x >= Vec2.x ? 0xFFFFFFFF : 0, same for yzw )
 */

FORCEINLINE VectorRegister4Float VectorCompareGE( const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return (VectorRegister4Float)vcgeq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorCompareGE(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = (VectorRegister2Double)vcgeq_f64(Vec1.XY, Vec2.XY);
	Result.ZW = (VectorRegister2Double)vcgeq_f64(Vec1.ZW, Vec2.ZW);
	return Result;
}

/**
* Creates a four-part mask based on component-wise < compares of the input vectors
*
* @param Vec1	1st vector
* @param Vec2	2nd vector
* @return		VectorRegister4Float( Vec1.x < Vec2.x ? 0xFFFFFFFF : 0, same for yzw )
*/
FORCEINLINE VectorRegister4Float VectorCompareLT(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2)
{
	return (VectorRegister4Float)vcltq_f32(Vec1, Vec2);
}

FORCEINLINE VectorRegister4Double VectorCompareLT(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Res;
	Res.XY = (VectorRegister2Double)vcltq_f64(Vec1.XY, Vec2.XY);
	Res.ZW = (VectorRegister2Double)vcltq_f64(Vec1.ZW, Vec2.ZW);
	return Res;
}

/**
* Creates a four-part mask based on component-wise <= compares of the input vectors
*
* @param Vec1	1st vector
* @param Vec2	2nd vector
* @return		VectorRegister4Float( Vec1.x <= Vec2.x ? 0xFFFFFFFF : 0, same for yzw )
*/
FORCEINLINE VectorRegister4Float VectorCompareLE(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2)
{
	return (VectorRegister4Float)vcleq_f32(Vec1, Vec2);
}

FORCEINLINE VectorRegister4Double VectorCompareLE(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Res;
	Res.XY = (VectorRegister2Double)vcleq_f64(Vec1.XY, Vec2.XY);
	Res.ZW = (VectorRegister2Double)vcleq_f64(Vec1.ZW, Vec2.ZW);
	return Res;
}

/**
 * Does a bitwise vector selection based on a mask (e.g., created from VectorCompareXX)
 *
 * @param Mask  Mask (when 1: use the corresponding bit from Vec1 otherwise from Vec2)
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( for each bit i: Mask[i] ? Vec1[i] : Vec2[i] )
 *
 */

FORCEINLINE VectorRegister4Float VectorSelect(const VectorRegister4Float& Mask, const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return vbslq_f32((VectorRegister4Int)Mask, Vec1, Vec2);
}

FORCEINLINE VectorRegister4Double VectorSelect(const VectorRegister4Double& Mask, const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = vbslq_f64((VectorRegister2Int64)Mask.XY, Vec1.XY, Vec2.XY);
	Result.ZW = vbslq_f64((VectorRegister2Int64)Mask.ZW, Vec1.ZW, Vec2.ZW);
	return Result;
}

/**
 * Combines two vectors using bitwise OR (treating each vector as a 128 bit field)
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( for each bit i: Vec1[i] | Vec2[i] )
 */
FORCEINLINE VectorRegister4Float VectorBitwiseOr(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return (VectorRegister4Float)vorrq_u32( (VectorRegister4Int)Vec1, (VectorRegister4Int)Vec2 );
}

FORCEINLINE VectorRegister4Double VectorBitwiseOr(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = (VectorRegister2Double)vorrq_u64((VectorRegister2Int64)Vec1.XY, (VectorRegister2Int64)Vec2.XY);
	Result.ZW = (VectorRegister2Double)vorrq_u64((VectorRegister2Int64)Vec1.ZW, (VectorRegister2Int64)Vec2.ZW);
	return Result;
}

/**
 * Combines two vectors using bitwise AND (treating each vector as a 128 bit field)
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( for each bit i: Vec1[i] & Vec2[i] )
 */
FORCEINLINE VectorRegister4Float VectorBitwiseAnd(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return (VectorRegister4Float)vandq_u32( (VectorRegister4Int)Vec1, (VectorRegister4Int)Vec2 );
}

FORCEINLINE VectorRegister4Double VectorBitwiseAnd(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = (VectorRegister2Double)vandq_u64((VectorRegister2Int64)Vec1.XY, (VectorRegister2Int64)Vec2.XY);
	Result.ZW = (VectorRegister2Double)vandq_u64((VectorRegister2Int64)Vec1.ZW, (VectorRegister2Int64)Vec2.ZW);
	return Result;
}

/**
 * Combines two vectors using bitwise XOR (treating each vector as a 128 bit field)
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( for each bit i: Vec1[i] ^ Vec2[i] )
 */
FORCEINLINE VectorRegister4Float VectorBitwiseXor(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	return (VectorRegister4Float)veorq_u32( (VectorRegister4Int)Vec1, (VectorRegister4Int)Vec2 );
}

FORCEINLINE VectorRegister4Double VectorBitwiseXor(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = (VectorRegister2Double)veorq_u64((VectorRegister2Int64)Vec1.XY, (VectorRegister2Int64)Vec2.XY);
	Result.ZW = (VectorRegister2Double)veorq_u64((VectorRegister2Int64)Vec1.ZW, (VectorRegister2Int64)Vec2.ZW);
	return Result;
}


/**
 * Swizzles the 4 components of a vector and returns the result.
 *
 * @param Vec		Source vector
 * @param X			Index for which component to use for X (literal 0-3)
 * @param Y			Index for which component to use for Y (literal 0-3)
 * @param Z			Index for which component to use for Z (literal 0-3)
 * @param W			Index for which component to use for W (literal 0-3)
 * @return			The swizzled vector
 */
#ifndef __clang__
FORCEINLINE VectorRegister4Float VectorSwizzle
(
	VectorRegister4Float V,
	uint32 E0,
	uint32 E1,
	uint32 E2,
	uint32 E3
)
{
	check((E0 < 4) && (E1 < 4) && (E2 < 4) && (E3 < 4));
	static constexpr uint32_t ControlElement[4] =
	{
		0x03020100, // XM_SWIZZLE_X
		0x07060504, // XM_SWIZZLE_Y
		0x0B0A0908, // XM_SWIZZLE_Z
		0x0F0E0D0C, // XM_SWIZZLE_W
	};

	uint8x8x2_t tbl;
	tbl.val[0] = vget_low_f32(V);
	tbl.val[1] = vget_high_f32(V);

	uint32x2_t idx = vcreate_u32(static_cast<uint64>(ControlElement[E0]) | (static_cast<uint64>(ControlElement[E1]) << 32));
	const uint8x8_t rL = vtbl2_u8(tbl, idx);

	idx = vcreate_u32(static_cast<uint64>(ControlElement[E2]) | (static_cast<uint64>(ControlElement[E3]) << 32));
	const uint8x8_t rH = vtbl2_u8(tbl, idx);

	return vcombine_f32(rL, rH);
}

FORCEINLINE VectorRegister4Double VectorSwizzle
(
	VectorRegister4Double V,
	uint32 E0,
	uint32 E1,
	uint32 E2,
	uint32 E3
)
{
	check((E0 < 4) && (E1 < 4) && (E2 < 4) && (E3 < 4));
	static constexpr uint64_t ControlElement[4] =
	{
		0x0706050403020100ULL, // XM_SWIZZLE_X
		0x0F0E0D0C0B0A0908ULL, // XM_SWIZZLE_Y
		0x1716151413121110ULL, // XM_SWIZZLE_Z
		0x1F1E1D1C1B1A1918ULL, // XM_SWIZZLE_W
	};

	uint8x16x2_t tbl;
	tbl.val[0] = V.XY;
	tbl.val[1] = V.ZW;

	VectorRegister4Double Result;
	uint32x4_t idx = vcombine_u64(vcreate_u64(ControlElement[E0]), vcreate_u64(ControlElement[E1]));
	Result.XY = vqtbl2q_u8(tbl, idx);

	idx = vcombine_u64(vcreate_u64(ControlElement[E2]), vcreate_u64(ControlElement[E3]));
	Result.ZW = vqtbl2q_u8(tbl, idx);

	return Result;
}
#else
template <int X, int Y, int Z, int W>
FORCEINLINE VectorRegister4Float VectorSwizzleImpl(VectorRegister4Float Vec)
{
	return __builtin_shufflevector(Vec, Vec, X, Y, Z, W);
}

template <int X, int Y, typename std::enable_if < (X <= 1) && (Y <= 1), bool >::type = true>
FORCEINLINE VectorRegister2Double VectorSwizzleImpl2(VectorRegister4Double Vec)
{
	return __builtin_shufflevector(Vec.XY, Vec.XY, X, Y);
}

template <int X, int Y, typename std::enable_if < (X <= 1) && (Y > 1), bool >::type = true>
FORCEINLINE VectorRegister2Double VectorSwizzleImpl2(VectorRegister4Double Vec)
{
	return __builtin_shufflevector(Vec.XY, Vec.ZW, X, Y);
}

template <int X, int Y, typename std::enable_if < (X > 1) && (Y <= 1), bool >::type = true>
FORCEINLINE VectorRegister2Double VectorSwizzleImpl2(VectorRegister4Double Vec)
{
	return __builtin_shufflevector(Vec.ZW, Vec.XY, X - 2, Y + 2);
}

template <int X, int Y, typename std::enable_if < (X > 1) && (Y > 1), bool >::type = true>
FORCEINLINE VectorRegister2Double VectorSwizzleImpl2(VectorRegister4Double Vec)
{
	return __builtin_shufflevector(Vec.ZW, Vec.ZW, X - 2, Y);
}

template <int X, int Y, int Z, int W>
FORCEINLINE VectorRegister4Double VectorSwizzleImpl(VectorRegister4Double Vec)
{
	VectorRegister4Double Result;
	Result.XY = VectorSwizzleImpl2<X, Y>(Vec);
	Result.ZW = VectorSwizzleImpl2<Z, W>(Vec);
	return Result;
}

#define VectorSwizzle( Vec, X, Y, Z, W ) VectorSwizzleImpl<X, Y, Z, W>(Vec)
#endif // __clang__ 


/**
* Creates a vector through selecting two components from each vector via a shuffle mask.
*
* @param Vec1		Source vector1
* @param Vec2		Source vector2
* @param X			Index for which component of Vector1 to use for X (literal 0-3)
* @param Y			Index for which component of Vector1 to use for Y (literal 0-3)
* @param Z			Index for which component of Vector2 to use for Z (literal 0-3)
* @param W			Index for which component of Vector2 to use for W (literal 0-3)
* @return			The swizzled vector
*/
#ifndef __clang__
FORCEINLINE VectorRegister4Float VectorShuffle
(
	VectorRegister4Float V1,
	VectorRegister4Float V2,
	uint32 PermuteX,
	uint32 PermuteY,
	uint32 PermuteZ,
	uint32 PermuteW
)
{
	check(PermuteX <= 3 && PermuteY <= 3 && PermuteZ <= 3 && PermuteW <= 3);

	static constexpr uint32 ControlElement[8] =
	{
		0x03020100, // XM_PERMUTE_0X
		0x07060504, // XM_PERMUTE_0Y
		0x0B0A0908, // XM_PERMUTE_0Z
		0x0F0E0D0C, // XM_PERMUTE_0W
		0x13121110, // XM_PERMUTE_1X
		0x17161514, // XM_PERMUTE_1Y
		0x1B1A1918, // XM_PERMUTE_1Z
		0x1F1E1D1C, // XM_PERMUTE_1W
	};

	uint8x8x4_t tbl;
	tbl.val[0] = vget_low_f32(V1);
	tbl.val[1] = vget_high_f32(V1);
	tbl.val[2] = vget_low_f32(V2);
	tbl.val[3] = vget_high_f32(V2);

	uint32x2_t idx = vcreate_u32(static_cast<uint64>(ControlElement[PermuteX]) | (static_cast<uint64>(ControlElement[PermuteY]) << 32));
	const uint8x8_t rL = vtbl4_u8(tbl, idx);

	idx = vcreate_u32(static_cast<uint64>(ControlElement[PermuteZ + 4]) | (static_cast<uint64>(ControlElement[PermuteW + 4]) << 32));
	const uint8x8_t rH = vtbl4_u8(tbl, idx);

	return vcombine_f32(rL, rH);
}

FORCEINLINE VectorRegister4Double VectorShuffle
(
	VectorRegister4Double V1,
	VectorRegister4Double V2,
	uint32 PermuteX,
	uint32 PermuteY,
	uint32 PermuteZ,
	uint32 PermuteW
)
{
	check(PermuteX <= 3 && PermuteY <= 3 && PermuteZ <= 3 && PermuteW <= 3);

	static constexpr uint64 ControlElement[8] =
	{
		0x0706050403020100ULL, // XM_PERMUTE_0X
		0x0F0E0D0C0B0A0908ULL, // XM_PERMUTE_0Y
		0x1716151413121110ULL, // XM_PERMUTE_0Z
		0x1F1E1D1C1B1A1918ULL, // XM_PERMUTE_0W

		0x2726252423222120ULL, // XM_PERMUTE_1X
		0x2F2E2D2C2B2A2928ULL, // XM_PERMUTE_1Y
		0x3736353433323130ULL, // XM_PERMUTE_1Z
		0x3F3E3D3C3B3A3938ULL, // XM_PERMUTE_1W
	};

	uint8x16x4_t tbl;
	tbl.val[0] = V1.XY;
	tbl.val[1] = V1.ZW;
	tbl.val[2] = V2.XY;
	tbl.val[3] = V2.ZW;

	VectorRegister4Double Result;
	uint32x4_t idx = vcombine_u64(vcreate_u64(ControlElement[PermuteX]), vcreate_u64(ControlElement[PermuteY]));
	Result.XY = vqtbl4q_u8(tbl, idx);

	idx = vcombine_u64(vcreate_u64(ControlElement[PermuteZ + 4]), vcreate_u64(ControlElement[PermuteW + 4]));
	Result.ZW = vqtbl4q_u8(tbl, idx);

	return Result;
}
#else

template <int X, int Y, int Z, int W>
FORCEINLINE VectorRegister4Float VectorShuffleImpl(VectorRegister4Float Vec1, VectorRegister4Float Vec2)
{
	return __builtin_shufflevector(Vec1, Vec2, X, Y, Z + 4, W + 4);
}

template <int X, int Y, int Z, int W>
FORCEINLINE VectorRegister4Double VectorShuffleImpl(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister4Double Result;
	Result.XY = VectorSwizzleImpl2<X, Y>(Vec1);
	Result.ZW = VectorSwizzleImpl2<Z, W>(Vec2);
	return Result;
}

#define VectorShuffle( Vec1, Vec2, X, Y, Z, W )	VectorShuffleImpl<X, Y, Z, W>(Vec1, Vec2)
#endif // __clang__ 

/**
 * Returns an integer bit-mask (0x00 - 0x0f) based on the sign-bit for each component in a vector.
 *
 * @param VecMask		Vector
 * @return				Bit 0 = sign(VecMask.x), Bit 1 = sign(VecMask.y), Bit 2 = sign(VecMask.z), Bit 3 = sign(VecMask.w)
 */
FORCEINLINE uint32 VectorMaskBits(VectorRegister4Float VecMask)
{
	uint32x4_t mmA = vtstq_u32(vreinterpretq_u32_f32(VecMask), GlobalVectorConstants::SignBit()); // mask with 1s every bit for vector element if it's sign is negative
	uint32x4_t mmB = vandq_u32(mmA, MakeVectorRegisterInt(0x1, 0x2, 0x4, 0x8)); // pick only one bit on it's corresponding position
	uint32x2_t mmC = vorr_u32(vget_low_u32(mmB), vget_high_u32(mmB));           // now combine the result
	return vget_lane_u32(mmC, 0) | vget_lane_u32(mmC, 1);                       // reduce the result from 2 elements to one
}

FORCEINLINE uint32 VectorMaskBits(VectorRegister4Double VecMask)
{
	uint64x2_t mmA = vtstq_u64(vreinterpretq_u64_f64(VecMask.XY), GlobalVectorConstants::DoubleSignBit().XY); // mask with 1s every bit for vector element if it's sign is negative
	uint64x2_t mmA1 = vtstq_u64(vreinterpretq_u64_f64(VecMask.ZW), GlobalVectorConstants::DoubleSignBit().XY);
	uint64x2_t mmB = vandq_u64(mmA, MakeVectorRegisterInt64(0x1, 0x2)); // pick only one bit on it's corresponding position
	uint64x2_t mmB1 = vandq_u64(mmA1, MakeVectorRegisterInt64(0x4, 0x8));
	uint64x2_t mmC = vorrq_u64(mmB, mmB1);								// now combine the result
	return (uint32)(vgetq_lane_u64(mmC, 0) | vgetq_lane_u64(mmC, 1));     // reduce the result from 2 elements to one
}

/**
* Creates a vector by combining two high components from each vector
*
* @param Vec1		Source vector1
* @param Vec2		Source vector2
* @return			The combined vector
*/
FORCEINLINE VectorRegister4Float VectorCombineHigh(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2)
{
	return vcombine_f32(vget_high_f32(Vec1), vget_high_f32(Vec2));
}

FORCEINLINE VectorRegister4Double VectorCombineHigh(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = Vec1.ZW;
	Result.ZW = Vec2.ZW;
	return Result;
}

/**
* Creates a vector by combining two low components from each vector
*
* @param Vec1		Source vector1
* @param Vec2		Source vector2
* @return			The combined vector
*/
FORCEINLINE VectorRegister4Float VectorCombineLow(const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2)
{
	return vcombine_f32(vget_low_f32(Vec1), vget_low_f32(Vec2));
}

FORCEINLINE VectorRegister4Double VectorCombineLow(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double Result;
	Result.XY = Vec1.XY;
	Result.ZW = Vec2.XY;
	return Result;
}

/**
 * Deinterleaves the components of the two given vectors such that the even components
 * are in one vector and the odds in another.
 *
 * @param Lo	[Even0, Odd0, Even1, Odd1]
 * @param Hi	[Even2, Odd2, Even3, Odd3]
 * @param OutEvens [Even0, Even1, Even2, Even3]
 * @param OutOdds [Odd0, Odd1, Odd2, Odd3]
*/
FORCEINLINE void VectorDeinterleave(VectorRegister4Float& OutEvens, VectorRegister4Float& OutOdds, const VectorRegister4Float& Lo, const VectorRegister4Float& Hi)
{
	float32x4x2_t deinterleaved = vuzpq_f32(Lo, Hi);
	OutEvens = deinterleaved.val[0];
	OutOdds = deinterleaved.val[1];
}

FORCEINLINE void VectorDeinterleave(VectorRegister4Double& RESTRICT OutEvens, VectorRegister4Double& RESTRICT OutOdds, const VectorRegister4Double& Lo, const VectorRegister4Double& Hi)
{
	OutEvens = VectorShuffle(Lo, Hi, 0, 2, 0, 2);
	OutOdds = VectorShuffle(Lo, Hi, 1, 3, 1, 3);
}

/**
 * Calculates the cross product of two vectors (XYZ components). W of the input should be 0, and will remain 0.
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		cross(Vec1.xyz, Vec2.xyz). W of the input should be 0, and will remain 0.
 */
FORCEINLINE VectorRegister4Float VectorCross( const VectorRegister4Float& Vec1, const VectorRegister4Float& Vec2 )
{
	VectorRegister4Float C = VectorMultiply(Vec1, VectorSwizzle(Vec2, 1, 2, 0, 3));
	C = VectorNegateMultiplyAdd(VectorSwizzle(Vec1, 1, 2, 0, 3), Vec2, C);
	C = VectorSwizzle(C, 1, 2, 0, 3);
	return C;
}

FORCEINLINE VectorRegister4Double VectorCross(const VectorRegister4Double& Vec1, const VectorRegister4Double& Vec2)
{
	VectorRegister4Double C = VectorMultiply(Vec1, VectorSwizzle(Vec2, 1, 2, 0, 3));
	C = VectorNegateMultiplyAdd(VectorSwizzle(Vec1, 1, 2, 0, 3), Vec2, C);
	C = VectorSwizzle(C, 1, 2, 0, 3);
	return C;
}

/**
 * Calculates x raised to the power of y (component-wise).
 *
 * @param Base		Base vector
 * @param Exponent	Exponent vector
 * @return			VectorRegister4Float( Base.x^Exponent.x, Base.y^Exponent.y, Base.z^Exponent.z, Base.w^Exponent.w )
 */
FORCEINLINE VectorRegister4Float VectorPow( const VectorRegister4Float& Base, const VectorRegister4Float& Exponent )
{
	//@TODO: Optimize this
	union U { 
		VectorRegister4Float V; float F[4]; 
		FORCEINLINE U() : V() {}
	} B, E;
	B.V = Base;
	E.V = Exponent;
	return MakeVectorRegister( powf(B.F[0], E.F[0]), powf(B.F[1], E.F[1]), powf(B.F[2], E.F[2]), powf(B.F[3], E.F[3]) );
}

FORCEINLINE VectorRegister4Double VectorPow(const VectorRegister4Double& Base, const VectorRegister4Double& Exponent)
{
	//@TODO: Optimize this
	AlignedDouble4 Values(Base);
	AlignedDouble4 Exponents(Exponent);

	Values[0] = FMath::Pow(Values[0], Exponents[0]);
	Values[1] = FMath::Pow(Values[1], Exponents[1]);
	Values[2] = FMath::Pow(Values[2], Exponents[2]);
	Values[3] = FMath::Pow(Values[3], Exponents[3]);
	return Values.ToVectorRegister();
}

/**
* Returns an estimate of 1/sqrt(c) for each component of the vector
*
* @param Vector		Vector 
* @return			VectorRegister4Float(1/sqrt(t), 1/sqrt(t), 1/sqrt(t), 1/sqrt(t))
*/
FORCEINLINE VectorRegister4Float VectorReciprocalSqrt(const VectorRegister4Float& Vec)
{
	return vrsqrteq_f32(Vec);
}

FORCEINLINE VectorRegister4Double VectorReciprocalSqrt(const VectorRegister4Double& Vec)
{
	VectorRegister4Double Result;
	Result.XY = vrsqrteq_f64(Vec.XY);
	Result.ZW = vrsqrteq_f64(Vec.ZW);
	return Result;
}

/**
 * Computes an estimate of the reciprocal of a vector (component-wise) and returns the result.
 *
 * @param Vec	1st vector
 * @return		VectorRegister4Float( (Estimate) 1.0f / Vec.x, (Estimate) 1.0f / Vec.y, (Estimate) 1.0f / Vec.z, (Estimate) 1.0f / Vec.w )
 */
FORCEINLINE VectorRegister4Float VectorReciprocal(const VectorRegister4Float& Vec)
{
	return vrecpeq_f32(Vec);
}

FORCEINLINE VectorRegister4Double VectorReciprocal(const VectorRegister4Double& Vec)
{
	VectorRegister4Double Result;
	Result.XY = vrecpeq_f64(Vec.XY);
	Result.ZW = vrecpeq_f64(Vec.ZW);
	return Result;
}


/**
* Return Reciprocal Length of the vector
*
* @param Vector		Vector 
* @return			VectorRegister4Float(rlen, rlen, rlen, rlen) when rlen = 1/sqrt(dot4(V))
*/
#define VectorReciprocalLen(Vector)		VectorReciprocalSqrt( VectorDot4( Vector, Vector ) )


/**
* Return the reciprocal of the square root of each component
*
* @param Vector		Vector
* @return			VectorRegister4Float(1/sqrt(Vec.X), 1/sqrt(Vec.Y), 1/sqrt(Vec.Z), 1/sqrt(Vec.W))
*/
FORCEINLINE VectorRegister4Float VectorReciprocalSqrtAccurate(const VectorRegister4Float& Vec)
{
	// Perform a single pass of Newton-Raphson iteration on the hardware estimate
	// This is a builtin instruction (VRSQRTS)

	// Initial estimate
	VectorRegister4Float RecipSqrt = VectorReciprocalSqrt(Vec);

	// Two refinement
	RecipSqrt = VectorMultiply(vrsqrtsq_f32(Vec, VectorMultiply(RecipSqrt, RecipSqrt)), RecipSqrt);
	return VectorMultiply(vrsqrtsq_f32(Vec, VectorMultiply(RecipSqrt, RecipSqrt)), RecipSqrt);
}

FORCEINLINE VectorRegister4Double VectorReciprocalSqrtAccurate(const VectorRegister4Double& Vec)
{
	// Perform a single pass of Newton-Raphson iteration on the hardware estimate
	// This is a builtin instruction (VRSQRTS)

	// Initial estimate
	VectorRegister4Double RecipSqrt = VectorReciprocalSqrt(Vec);

	// Two refinement
	VectorRegister4Double Tmp;
	Tmp.XY = vrsqrtsq_f64(Vec.XY, VectorMultiply(RecipSqrt.XY, RecipSqrt.XY));
	Tmp.ZW = vrsqrtsq_f64(Vec.ZW, VectorMultiply(RecipSqrt.ZW, RecipSqrt.ZW));
	RecipSqrt = VectorMultiply(Tmp, RecipSqrt);

	Tmp.XY = vrsqrtsq_f64(Vec.XY, VectorMultiply(RecipSqrt.XY, RecipSqrt.XY));
	Tmp.ZW = vrsqrtsq_f64(Vec.ZW, VectorMultiply(RecipSqrt.ZW, RecipSqrt.ZW));
	return VectorMultiply(Tmp, RecipSqrt);
}


/**
 * Computes the reciprocal of a vector (component-wise) and returns the result.
 *
 * @param Vec	1st vector
 * @return		VectorRegister4Float( 1.0f / Vec.x, 1.0f / Vec.y, 1.0f / Vec.z, 1.0f / Vec.w )
 */
FORCEINLINE VectorRegister4Float VectorReciprocalAccurate(const VectorRegister4Float& Vec)
{
	// Perform two passes of Newton-Raphson iteration on the hardware estimate
	// The built-in instruction (VRECPS) is not as accurate

	// Initial estimate
	VectorRegister4Float Reciprocal = VectorReciprocal(Vec);

	// First iteration
	VectorRegister4Float Squared = VectorMultiply(Reciprocal, Reciprocal);
	VectorRegister4Float Double = VectorAdd(Reciprocal, Reciprocal);
	Reciprocal = VectorNegateMultiplyAdd(Vec, Squared, Double);

	// Second iteration
	Squared = VectorMultiply(Reciprocal, Reciprocal);
	Double = VectorAdd(Reciprocal, Reciprocal);
	return VectorNegateMultiplyAdd(Vec, Squared, Double);
}

FORCEINLINE VectorRegister4Double VectorReciprocalAccurate(const VectorRegister4Double& Vec)
{
	return VectorDivide(GlobalVectorConstants::DoubleOne, Vec);
}

/**
* Loads XYZ and sets W=0
*
* @param Vector	VectorRegister4Float
* @return		VectorRegister4Float(X, Y, Z, 0.0f)
*/
FORCEINLINE VectorRegister4Float VectorSet_W0(const VectorRegister4Float& Vec)
{
	return VectorSetComponent(Vec, 3, 0.0f);
}

FORCEINLINE VectorRegister4Double VectorSet_W0(const VectorRegister4Double& Vec)
{
	return VectorSetComponent(Vec, 3, 0.0);
}


/**
* Loads XYZ and sets W=1.
*
* @param Vector	VectorRegister4Float
* @return		VectorRegister4Float(X, Y, Z, 1.0f)
*/
FORCEINLINE VectorRegister4Float VectorSet_W1(const VectorRegister4Float& Vec)
{
	return VectorSetComponent(Vec, 3, 1.0f);
}

FORCEINLINE VectorRegister4Double VectorSet_W1(const VectorRegister4Double& Vec)
{
	return VectorSetComponent(Vec, 3, 1.0);
}



/**
* Returns a component from a vector.
*
* @param Vec				Vector register
* @param ComponentIndex	Which component to get, X=0, Y=1, Z=2, W=3
* @return					The component as a float
*/
template <uint32 ElementIndex>
FORCEINLINE float VectorGetComponentImpl(VectorRegister4Float Vec)
{
	return vgetq_lane_f32(Vec, ElementIndex);
}

template <int ElementIndex>
FORCEINLINE double VectorGetComponentImpl(VectorRegister2Double Vec)
{
	return vgetq_lane_f64(Vec, ElementIndex);
}

template<int ElementIndex, typename std::enable_if< (ElementIndex > 1), bool >::type = true >
FORCEINLINE double VectorGetComponentImpl(const VectorRegister4Double& Vec)
{
	return VectorGetComponentImpl<ElementIndex - 2>(Vec.ZW);
}

template<int ElementIndex, typename std::enable_if < (ElementIndex <= 1), bool >::type = true >
FORCEINLINE double VectorGetComponentImpl(const VectorRegister4Double& Vec)
{
	return VectorGetComponentImpl<ElementIndex>(Vec.XY);
}

#define VectorGetComponent(Vec, ElementIndex) VectorGetComponentImpl<ElementIndex>(Vec)

FORCEINLINE float VectorGetComponentDynamic(VectorRegister4Float Vec, uint32 ElementIndex)
{
	AlignedFloat4 Floats(Vec);
	return Floats[ElementIndex];
}

FORCEINLINE double VectorGetComponentDynamic(VectorRegister4Double Vec, uint32 ElementIndex)
{
	AlignedDouble4 Doubles(Vec);
	return Doubles[ElementIndex];
}

/**
 * Multiplies two 4x4 matrices.
 *
 * @param Result	Pointer to where the result should be stored
 * @param Matrix1	Pointer to the first matrix
 * @param Matrix2	Pointer to the second matrix
 */
FORCEINLINE void VectorMatrixMultiply( FMatrix44f* Result, const FMatrix44f* Matrix1, const FMatrix44f* Matrix2 )
{
	float32x4x4_t A = vld1q_f32_x4((const float*)Matrix1);
	float32x4x4_t B = vld1q_f32_x4((const float*)Matrix2);
	float32x4x4_t R;

	// First row of result (Matrix1[0] * Matrix2).
	R.val[0] = vmulq_lane_f32(B.val[0], vget_low_f32(A.val[0]), 0);
	R.val[0] = vfmaq_lane_f32(R.val[0], B.val[1], vget_low_f32(A.val[0]), 1);
	R.val[0] = vfmaq_lane_f32(R.val[0], B.val[2], vget_high_f32(A.val[0]), 0);
	R.val[0] = vfmaq_lane_f32(R.val[0], B.val[3], vget_high_f32(A.val[0]), 1);

	// Second row of result (Matrix1[1] * Matrix2).
	R.val[1] = vmulq_lane_f32(B.val[0], vget_low_f32(A.val[1]), 0);
	R.val[1] = vfmaq_lane_f32(R.val[1], B.val[1], vget_low_f32(A.val[1]), 1);
	R.val[1] = vfmaq_lane_f32(R.val[1], B.val[2], vget_high_f32(A.val[1]), 0);
	R.val[1] = vfmaq_lane_f32(R.val[1], B.val[3], vget_high_f32(A.val[1]), 1);

	// Third row of result (Matrix1[2] * Matrix2).
	R.val[2] = vmulq_lane_f32(B.val[0], vget_low_f32(A.val[2]), 0);
	R.val[2] = vfmaq_lane_f32(R.val[2], B.val[1], vget_low_f32(A.val[2]), 1);
	R.val[2] = vfmaq_lane_f32(R.val[2], B.val[2], vget_high_f32(A.val[2]), 0);
	R.val[2] = vfmaq_lane_f32(R.val[2], B.val[3], vget_high_f32(A.val[2]), 1);

	// Fourth row of result (Matrix1[3] * Matrix2).
	R.val[3] = vmulq_lane_f32(B.val[0], vget_low_f32(A.val[3]), 0);
	R.val[3] = vfmaq_lane_f32(R.val[3], B.val[1], vget_low_f32(A.val[3]), 1);
	R.val[3] = vfmaq_lane_f32(R.val[3], B.val[2], vget_high_f32(A.val[3]), 0);
	R.val[3] = vfmaq_lane_f32(R.val[3], B.val[3], vget_high_f32(A.val[3]), 1);

	vst1q_f32_x4((float*)Result, R);
}

FORCEINLINE void VectorMatrixMultiply(FMatrix44d* Result, const FMatrix44d* Matrix1, const FMatrix44d* Matrix2)
{
	float64x2x4_t A = vld1q_f64_x4((const double*)Matrix1);
	float64x2x4_t B1 = vld1q_f64_x4((const double*)Matrix2);
	float64x2x4_t B2 = vld1q_f64_x4((const double*)Matrix2 + 8);
	float64_t* V = (float64_t*)&A;
	float64x2x4_t R;

	// First row of result (Matrix1[0] * Matrix2).
	R.val[0] = vmulq_n_f64(B1.val[0], V[0]);
	R.val[0] = vfmaq_n_f64(R.val[0], B1.val[2], V[1]);
	R.val[0] = vfmaq_n_f64(R.val[0], B2.val[0], V[2]);
	R.val[0] = vfmaq_n_f64(R.val[0], B2.val[2], V[3]);

	R.val[1] = vmulq_n_f64(B1.val[1], V[0]);
	R.val[1] = vfmaq_n_f64(R.val[1], B1.val[3], V[1]);
	R.val[1] = vfmaq_n_f64(R.val[1], B2.val[1], V[2]);
	R.val[1] = vfmaq_n_f64(R.val[1], B2.val[3], V[3]);

	// Second row of result (Matrix1[1] * Matrix2).
	R.val[2] = vmulq_n_f64(B1.val[0], V[4]);
	R.val[2] = vfmaq_n_f64(R.val[2], B1.val[2], V[5]);
	R.val[2] = vfmaq_n_f64(R.val[2], B2.val[0], V[6]);
	R.val[2] = vfmaq_n_f64(R.val[2], B2.val[2], V[7]);

	R.val[3] = vmulq_n_f64(B1.val[1], V[4]);
	R.val[3] = vfmaq_n_f64(R.val[3], B1.val[3], V[5]);
	R.val[3] = vfmaq_n_f64(R.val[3], B2.val[1], V[6]);
	R.val[3] = vfmaq_n_f64(R.val[3], B2.val[3], V[7]);

	vst1q_f64_x4((double*)Result, R);
	A = vld1q_f64_x4((const double*)Matrix1 + 8);
	V = (float64_t*)&A;

	// Third row of result (Matrix1[2] * Matrix2).
	R.val[0] = vmulq_n_f64(B1.val[0], V[0]);
	R.val[0] = vfmaq_n_f64(R.val[0], B1.val[2], V[1]);
	R.val[0] = vfmaq_n_f64(R.val[0], B2.val[0], V[2]);
	R.val[0] = vfmaq_n_f64(R.val[0], B2.val[2], V[3]);

	R.val[1] = vmulq_n_f64(B1.val[1], V[0]);
	R.val[1] = vfmaq_n_f64(R.val[1], B1.val[3], V[1]);
	R.val[1] = vfmaq_n_f64(R.val[1], B2.val[1], V[2]);
	R.val[1] = vfmaq_n_f64(R.val[1], B2.val[3], V[3]);

	// Fourth row of result (Matrix1[3] * Matrix2).
	R.val[2] = vmulq_n_f64(B1.val[0], V[4]);
	R.val[2] = vfmaq_n_f64(R.val[2], B1.val[2], V[5]);
	R.val[2] = vfmaq_n_f64(R.val[2], B2.val[0], V[6]);
	R.val[2] = vfmaq_n_f64(R.val[2], B2.val[2], V[7]);

	R.val[3] = vmulq_n_f64(B1.val[1], V[4]);
	R.val[3] = vfmaq_n_f64(R.val[3], B1.val[3], V[5]);
	R.val[3] = vfmaq_n_f64(R.val[3], B2.val[1], V[6]);
	R.val[3] = vfmaq_n_f64(R.val[3], B2.val[3], V[7]);

	vst1q_f64_x4((double*)Result + 8, R);
}

/**
 * Calculate the inverse of an FMatrix.
 *
 * @param DstMatrix		FMatrix pointer to where the result should be stored
 * @param SrcMatrix		FMatrix pointer to the Matrix to be inversed
 */
// OPTIMIZE ME: stolen from UnMathFpu.h
FORCEINLINE void VectorMatrixInverse(FMatrix44f* DstMatrix, const FMatrix44f* SrcMatrix )
{
	typedef float Float4x4[4][4];
	const Float4x4& M = *((const Float4x4*) SrcMatrix);
	Float4x4 Result;
	float Det[4];
	Float4x4 Tmp;
	
	Tmp[0][0]       = M[2][2] * M[3][3] - M[2][3] * M[3][2];
	Tmp[0][1]       = M[1][2] * M[3][3] - M[1][3] * M[3][2];
	Tmp[0][2]       = M[1][2] * M[2][3] - M[1][3] * M[2][2];
	
	Tmp[1][0]       = M[2][2] * M[3][3] - M[2][3] * M[3][2];
	Tmp[1][1]       = M[0][2] * M[3][3] - M[0][3] * M[3][2];
	Tmp[1][2]       = M[0][2] * M[2][3] - M[0][3] * M[2][2];
	
	Tmp[2][0]       = M[1][2] * M[3][3] - M[1][3] * M[3][2];
	Tmp[2][1]       = M[0][2] * M[3][3] - M[0][3] * M[3][2];
	Tmp[2][2]       = M[0][2] * M[1][3] - M[0][3] * M[1][2];
	
	Tmp[3][0]       = M[1][2] * M[2][3] - M[1][3] * M[2][2];
	Tmp[3][1]       = M[0][2] * M[2][3] - M[0][3] * M[2][2];
	Tmp[3][2]       = M[0][2] * M[1][3] - M[0][3] * M[1][2];
	
	Det[0]          = M[1][1]*Tmp[0][0] - M[2][1]*Tmp[0][1] + M[3][1]*Tmp[0][2];
	Det[1]          = M[0][1]*Tmp[1][0] - M[2][1]*Tmp[1][1] + M[3][1]*Tmp[1][2];
	Det[2]          = M[0][1]*Tmp[2][0] - M[1][1]*Tmp[2][1] + M[3][1]*Tmp[2][2];
	Det[3]          = M[0][1]*Tmp[3][0] - M[1][1]*Tmp[3][1] + M[2][1]*Tmp[3][2];
	
	float Determinant = M[0][0]*Det[0] - M[1][0]*Det[1] + M[2][0]*Det[2] - M[3][0]*Det[3];
	const float     RDet = 1.0f / Determinant;
	
	Result[0][0] =  RDet * Det[0];
	Result[0][1] = -RDet * Det[1];
	Result[0][2] =  RDet * Det[2];
	Result[0][3] = -RDet * Det[3];
	Result[1][0] = -RDet * (M[1][0]*Tmp[0][0] - M[2][0]*Tmp[0][1] + M[3][0]*Tmp[0][2]);
	Result[1][1] =  RDet * (M[0][0]*Tmp[1][0] - M[2][0]*Tmp[1][1] + M[3][0]*Tmp[1][2]);
	Result[1][2] = -RDet * (M[0][0]*Tmp[2][0] - M[1][0]*Tmp[2][1] + M[3][0]*Tmp[2][2]);
	Result[1][3] =  RDet * (M[0][0]*Tmp[3][0] - M[1][0]*Tmp[3][1] + M[2][0]*Tmp[3][2]);
	Result[2][0] =  RDet * (
							M[1][0] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) -
							M[2][0] * (M[1][1] * M[3][3] - M[1][3] * M[3][1]) +
							M[3][0] * (M[1][1] * M[2][3] - M[1][3] * M[2][1])
							);
	Result[2][1] = -RDet * (
							M[0][0] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) -
							M[2][0] * (M[0][1] * M[3][3] - M[0][3] * M[3][1]) +
							M[3][0] * (M[0][1] * M[2][3] - M[0][3] * M[2][1])
							);
	Result[2][2] =  RDet * (
							M[0][0] * (M[1][1] * M[3][3] - M[1][3] * M[3][1]) -
							M[1][0] * (M[0][1] * M[3][3] - M[0][3] * M[3][1]) +
							M[3][0] * (M[0][1] * M[1][3] - M[0][3] * M[1][1])
							);
	Result[2][3] = -RDet * (
							M[0][0] * (M[1][1] * M[2][3] - M[1][3] * M[2][1]) -
							M[1][0] * (M[0][1] * M[2][3] - M[0][3] * M[2][1]) +
							M[2][0] * (M[0][1] * M[1][3] - M[0][3] * M[1][1])
							);
	Result[3][0] = -RDet * (
							M[1][0] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]) -
							M[2][0] * (M[1][1] * M[3][2] - M[1][2] * M[3][1]) +
							M[3][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1])
							);
	Result[3][1] =  RDet * (
							M[0][0] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]) -
							M[2][0] * (M[0][1] * M[3][2] - M[0][2] * M[3][1]) +
							M[3][0] * (M[0][1] * M[2][2] - M[0][2] * M[2][1])
							);
	Result[3][2] = -RDet * (
							M[0][0] * (M[1][1] * M[3][2] - M[1][2] * M[3][1]) -
							M[1][0] * (M[0][1] * M[3][2] - M[0][2] * M[3][1]) +
							M[3][0] * (M[0][1] * M[1][2] - M[0][2] * M[1][1])
							);
	Result[3][3] =  RDet * (
							M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) -
							M[1][0] * (M[0][1] * M[2][2] - M[0][2] * M[2][1]) +
							M[2][0] * (M[0][1] * M[1][2] - M[0][2] * M[1][1])
							);
	
	memcpy( DstMatrix, &Result, sizeof(Result) );
}

FORCEINLINE void VectorMatrixInverse(FMatrix44d* DstMatrix, const FMatrix44d* SrcMatrix)
{
	typedef double Double4x4[4][4];
	const Double4x4& M = *((const Double4x4*)SrcMatrix);
	Double4x4 Result;
	double Det[4];
	Double4x4 Tmp;

	Tmp[0][0] = M[2][2] * M[3][3] - M[2][3] * M[3][2];
	Tmp[0][1] = M[1][2] * M[3][3] - M[1][3] * M[3][2];
	Tmp[0][2] = M[1][2] * M[2][3] - M[1][3] * M[2][2];

	Tmp[1][0] = M[2][2] * M[3][3] - M[2][3] * M[3][2];
	Tmp[1][1] = M[0][2] * M[3][3] - M[0][3] * M[3][2];
	Tmp[1][2] = M[0][2] * M[2][3] - M[0][3] * M[2][2];

	Tmp[2][0] = M[1][2] * M[3][3] - M[1][3] * M[3][2];
	Tmp[2][1] = M[0][2] * M[3][3] - M[0][3] * M[3][2];
	Tmp[2][2] = M[0][2] * M[1][3] - M[0][3] * M[1][2];

	Tmp[3][0] = M[1][2] * M[2][3] - M[1][3] * M[2][2];
	Tmp[3][1] = M[0][2] * M[2][3] - M[0][3] * M[2][2];
	Tmp[3][2] = M[0][2] * M[1][3] - M[0][3] * M[1][2];

	Det[0] = M[1][1] * Tmp[0][0] - M[2][1] * Tmp[0][1] + M[3][1] * Tmp[0][2];
	Det[1] = M[0][1] * Tmp[1][0] - M[2][1] * Tmp[1][1] + M[3][1] * Tmp[1][2];
	Det[2] = M[0][1] * Tmp[2][0] - M[1][1] * Tmp[2][1] + M[3][1] * Tmp[2][2];
	Det[3] = M[0][1] * Tmp[3][0] - M[1][1] * Tmp[3][1] + M[2][1] * Tmp[3][2];

	double Determinant = M[0][0] * Det[0] - M[1][0] * Det[1] + M[2][0] * Det[2] - M[3][0] * Det[3];
	const double RDet = 1.0 / Determinant;

	Result[0][0] = RDet * Det[0];
	Result[0][1] = -RDet * Det[1];
	Result[0][2] = RDet * Det[2];
	Result[0][3] = -RDet * Det[3];
	Result[1][0] = -RDet * (M[1][0] * Tmp[0][0] - M[2][0] * Tmp[0][1] + M[3][0] * Tmp[0][2]);
	Result[1][1] = RDet * (M[0][0] * Tmp[1][0] - M[2][0] * Tmp[1][1] + M[3][0] * Tmp[1][2]);
	Result[1][2] = -RDet * (M[0][0] * Tmp[2][0] - M[1][0] * Tmp[2][1] + M[3][0] * Tmp[2][2]);
	Result[1][3] = RDet * (M[0][0] * Tmp[3][0] - M[1][0] * Tmp[3][1] + M[2][0] * Tmp[3][2]);
	Result[2][0] = RDet * (
		M[1][0] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) -
		M[2][0] * (M[1][1] * M[3][3] - M[1][3] * M[3][1]) +
		M[3][0] * (M[1][1] * M[2][3] - M[1][3] * M[2][1])
		);
	Result[2][1] = -RDet * (
		M[0][0] * (M[2][1] * M[3][3] - M[2][3] * M[3][1]) -
		M[2][0] * (M[0][1] * M[3][3] - M[0][3] * M[3][1]) +
		M[3][0] * (M[0][1] * M[2][3] - M[0][3] * M[2][1])
		);
	Result[2][2] = RDet * (
		M[0][0] * (M[1][1] * M[3][3] - M[1][3] * M[3][1]) -
		M[1][0] * (M[0][1] * M[3][3] - M[0][3] * M[3][1]) +
		M[3][0] * (M[0][1] * M[1][3] - M[0][3] * M[1][1])
		);
	Result[2][3] = -RDet * (
		M[0][0] * (M[1][1] * M[2][3] - M[1][3] * M[2][1]) -
		M[1][0] * (M[0][1] * M[2][3] - M[0][3] * M[2][1]) +
		M[2][0] * (M[0][1] * M[1][3] - M[0][3] * M[1][1])
		);
	Result[3][0] = -RDet * (
		M[1][0] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]) -
		M[2][0] * (M[1][1] * M[3][2] - M[1][2] * M[3][1]) +
		M[3][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1])
		);
	Result[3][1] = RDet * (
		M[0][0] * (M[2][1] * M[3][2] - M[2][2] * M[3][1]) -
		M[2][0] * (M[0][1] * M[3][2] - M[0][2] * M[3][1]) +
		M[3][0] * (M[0][1] * M[2][2] - M[0][2] * M[2][1])
		);
	Result[3][2] = -RDet * (
		M[0][0] * (M[1][1] * M[3][2] - M[1][2] * M[3][1]) -
		M[1][0] * (M[0][1] * M[3][2] - M[0][2] * M[3][1]) +
		M[3][0] * (M[0][1] * M[1][2] - M[0][2] * M[1][1])
		);
	Result[3][3] = RDet * (
		M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) -
		M[1][0] * (M[0][1] * M[2][2] - M[0][2] * M[2][1]) +
		M[2][0] * (M[0][1] * M[1][2] - M[0][2] * M[1][1])
		);

	memcpy(DstMatrix, &Result, sizeof(Result));
}

/**
 * Calculate Homogeneous transform.
 *
 * @param VecP			VectorRegister4Float 
 * @param MatrixM		FMatrix pointer to the Matrix to apply transform
 * @return VectorRegister4Float = VecP*MatrixM
 */
FORCEINLINE VectorRegister4Float VectorTransformVector(const VectorRegister4Float& VecP, const FMatrix44f* MatrixM )
{
	float32x4x4_t M = vld1q_f32_x4((const float*)MatrixM);
	VectorRegister4Float Result;

	Result = vmulq_n_f32(M.val[0], VecP[0]);
	Result = vfmaq_n_f32(Result, M.val[1], VecP[1]);
	Result = vfmaq_n_f32(Result, M.val[2], VecP[2]);
	Result = vfmaq_n_f32(Result, M.val[3], VecP[3]);

	return Result;
}

FORCEINLINE VectorRegister4Float VectorTransformVector(const VectorRegister4Float& VecP, const FMatrix44d* MatrixM)
{
	float64x2x4_t M1 = vld1q_f64_x4((const double*)MatrixM);
	float64x2x4_t M2 = vld1q_f64_x4(((const double*)MatrixM) + 8);
	VectorRegister4Double Result;
	VectorRegister4Double Vec(VecP);

	Result.XY = vmulq_n_f64(M1.val[0], Vec.XY[0]);
	Result.XY = vfmaq_n_f64(Result.XY, M1.val[2], Vec.XY[1]);
	Result.XY = vfmaq_n_f64(Result.XY, M2.val[0], Vec.ZW[0]);
	Result.XY = vfmaq_n_f64(Result.XY, M2.val[2], Vec.ZW[1]);

	Result.ZW = vmulq_n_f64(M1.val[1], Vec.XY[0]);
	Result.ZW = vfmaq_n_f64(Result.ZW, M1.val[3], Vec.XY[1]);
	Result.ZW = vfmaq_n_f64(Result.ZW, M2.val[1], Vec.ZW[0]);
	Result.ZW = vfmaq_n_f64(Result.ZW, M2.val[3], Vec.ZW[1]);

	return MakeVectorRegisterFloatFromDouble(Result);
}

FORCEINLINE VectorRegister4Double VectorTransformVector(const VectorRegister4Double& VecP, const FMatrix44d* MatrixM)
{
	float64x2x4_t M1 = vld1q_f64_x4((const double*)MatrixM);
	float64x2x4_t M2 = vld1q_f64_x4(((const double*)MatrixM) + 8);
	VectorRegister4Double Result;

	//TODO: this can be rewritten to avoid using M2 var, saves some registers
	Result.XY = vmulq_n_f64(M1.val[0], VecP.XY[0]);
	Result.XY = vfmaq_n_f64(Result.XY, M1.val[2], VecP.XY[1]);
	Result.XY = vfmaq_n_f64(Result.XY, M2.val[0], VecP.ZW[0]);
	Result.XY = vfmaq_n_f64(Result.XY, M2.val[2], VecP.ZW[1]);

	Result.ZW = vmulq_n_f64(M1.val[1], VecP.XY[0]);
	Result.ZW = vfmaq_n_f64(Result.ZW, M1.val[3], VecP.XY[1]);
	Result.ZW = vfmaq_n_f64(Result.ZW, M2.val[1], VecP.ZW[0]);
	Result.ZW = vfmaq_n_f64(Result.ZW, M2.val[3], VecP.ZW[1]);

	return Result;
}

/**
 * Returns the minimum values of two vectors (component-wise).
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( min(Vec1.x,Vec2.x), min(Vec1.y,Vec2.y), min(Vec1.z,Vec2.z), min(Vec1.w,Vec2.w) )
 */
FORCEINLINE VectorRegister4Float VectorMin( VectorRegister4Float Vec1, VectorRegister4Float Vec2 )
{
	return vminq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorMin(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister4Double Result;
	Result.XY = vminq_f64(Vec1.XY, Vec2.XY);
	Result.ZW = vminq_f64(Vec1.ZW, Vec2.ZW);
	return Result;
}

/**
 * Returns the maximum values of two vectors (component-wise).
 *
 * @param Vec1	1st vector
 * @param Vec2	2nd vector
 * @return		VectorRegister4Float( max(Vec1.x,Vec2.x), max(Vec1.y,Vec2.y), max(Vec1.z,Vec2.z), max(Vec1.w,Vec2.w) )
 */
FORCEINLINE VectorRegister4Float VectorMax( VectorRegister4Float Vec1, VectorRegister4Float Vec2 )
{
	return vmaxq_f32( Vec1, Vec2 );
}

FORCEINLINE VectorRegister4Double VectorMax(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	VectorRegister4Double Result;
	Result.XY = vmaxq_f64(Vec1.XY, Vec2.XY);
	Result.ZW = vmaxq_f64(Vec1.ZW, Vec2.ZW);
	return Result;
}

/**
 * Merges the XYZ components of one vector with the W component of another vector and returns the result.
 *
 * @param VecXYZ	Source vector for XYZ_
 * @param VecW		Source register for ___W (note: the fourth component is used, not the first)
 * @return			VectorRegister4Float(VecXYZ.x, VecXYZ.y, VecXYZ.z, VecW.w)
 */
FORCEINLINE VectorRegister4Float VectorMergeVecXYZ_VecW(const VectorRegister4Float& VecXYZ, const VectorRegister4Float& VecW)
{
	return vsetq_lane_f32(vgetq_lane_f32(VecW, 3), VecXYZ, 3);
}

FORCEINLINE VectorRegister4Double VectorMergeVecXYZ_VecW(const VectorRegister4Double& VecXYZ, const VectorRegister4Double& VecW)
{
	VectorRegister4Double Res;
	Res.XY = VecXYZ.XY;
	Res.ZW = vsetq_lane_f64(vgetq_lane_f64(VecW.ZW, 1), VecXYZ.ZW, 1);
	return Res;
}

/**
 * Loads 4 uint8s from unaligned memory and converts them into 4 floats.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
 *
 * @param Ptr			Unaligned memory pointer to the 4 uint8s.
 * @return				VectorRegister4Float( float(Ptr[0]), float(Ptr[1]), float(Ptr[2]), float(Ptr[3]) )
 */
FORCEINLINE VectorRegister4Float VectorLoadByte4( const void* Ptr )
{
	// OPTIMIZE ME!
	const uint8 *P = (const uint8 *)Ptr;
	return MakeVectorRegister( (float)P[0], (float)P[1], (float)P[2], (float)P[3] );
}

/**
* Loads 4 int8s from unaligned memory and converts them into 4 floats.
* IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
*
* @param Ptr			Unaligned memory pointer to the 4 uint8s.
* @return				VectorRegister4Float( float(Ptr[0]), float(Ptr[1]), float(Ptr[2]), float(Ptr[3]) )
*/
FORCEINLINE VectorRegister4Float VectorLoadSignedByte4(const void* Ptr)
{
	// OPTIMIZE ME!
	const int8 *P = (const int8 *)Ptr;
	return MakeVectorRegister((float)P[0], (float)P[1], (float)P[2], (float)P[3]);
}

/**
 * Loads 4 uint8s from unaligned memory and converts them into 4 floats in reversed order.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
 *
 * @param Ptr			Unaligned memory pointer to the 4 uint8s.
 * @return				VectorRegister4Float( float(Ptr[3]), float(Ptr[2]), float(Ptr[1]), float(Ptr[0]) )
 */
FORCEINLINE VectorRegister4Float VectorLoadByte4Reverse( const uint8* Ptr )
{
	// OPTIMIZE ME!
	const uint8 *P = (const uint8 *)Ptr;
	return MakeVectorRegister( (float)P[3], (float)P[2], (float)P[1], (float)P[0] );
}

/**
 * Converts the 4 floats in the vector to 4 uint8s, clamped to [0,255], and stores to unaligned memory.
 * IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
 *
 * @param Vec			Vector containing 4 floats
 * @param Ptr			Unaligned memory pointer to store the 4 uint8s.
 */
FORCEINLINE void VectorStoreByte4( VectorRegister4Float Vec, void* Ptr )
{
	uint16x8_t u16x8 = (uint16x8_t)vcvtq_u32_f32(VectorMin(Vec, GlobalVectorConstants::Float255));
	uint8x8_t u8x8 = (uint8x8_t)vget_low_u16( vuzpq_u16( u16x8, u16x8 ).val[0] );
	u8x8 = vuzp_u8( u8x8, u8x8 ).val[0];
	uint32_t buf[2];
	vst1_u8( (uint8_t *)buf, u8x8 );
	*(uint32_t *)Ptr = buf[0]; 
}

/**
* Converts the 4 floats in the vector to 4 int8s, clamped to [-127, 127], and stores to unaligned memory.
* IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar floats after you've used this intrinsic!
*
* @param Vec			Vector containing 4 floats
* @param Ptr			Unaligned memory pointer to store the 4 uint8s.
*/
FORCEINLINE void VectorStoreSignedByte4(VectorRegister4Float Vec, void* Ptr)
{
	int16x8_t s16x8 = (int16x8_t)vcvtq_s32_f32(VectorMax(VectorMin(Vec, GlobalVectorConstants::Float127), GlobalVectorConstants::FloatNeg127));
	int8x8_t s8x8 = (int8x8_t)vget_low_s16(vuzpq_s16(s16x8, s16x8).val[0]);
	s8x8 = vuzp_s8(s8x8, s8x8).val[0];
	int32_t buf[2];
	vst1_s8((int8_t *)buf, s8x8);
	*(int32_t *)Ptr = buf[0];
}

/**
 * Converts the 4 floats in the vector to 4 fp16 and stores based off bool to [un]aligned memory.
 *
 * @param Vec			Vector containing 4 floats
 * @param Ptr			Memory pointer to store the 4 fp16's.
 */
template <bool bAligned>
FORCEINLINE void VectorStoreHalf4(VectorRegister4Float Vec, void* RESTRICT Ptr)
{
	float16x4_t f16x4 = vcvt_f16_f32(Vec);

	if (bAligned)
	{
		vst1_u8( (uint8_t *)Ptr, f16x4 );
	}
	else
	{
		alignas(16) uint16_t Buf[4];
		vst1_u8( (uint8_t *)Buf, f16x4 );
		for (int i = 0; i < 4; ++i)
		{
			((uint16_t*)Ptr)[i] = Buf[i];
		}
	}
}

/**
* Loads packed RGB10A2(4 bytes) from unaligned memory and converts them into 4 FLOATs.
* IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
*
* @param Ptr			Unaligned memory pointer to the RGB10A2(4 bytes).
* @return				VectorRegister4Float with 4 FLOATs loaded from Ptr.
*/
FORCEINLINE VectorRegister4Float VectorLoadURGB10A2N(void* Ptr)
{
	alignas(16) float V[4];
	const uint32 E = *(uint32*)Ptr;
	V[0] = float((E >> 00) & 0x3FF);
	V[1] = float((E >> 10) & 0x3FF);
	V[2] = float((E >> 20) & 0x3FF);
	V[3] = float((E >> 30) & 0x3);

	VectorRegister4Float Div = MakeVectorRegister(1.0f / 1023.0f, 1.0f / 1023.0f, 1.0f / 1023.0f, 1.0f / 3.0f);
	return VectorMultiply(MakeVectorRegister(V[0], V[1], V[2], V[3]), Div);
}

/**
* Converts the 4 FLOATs in the vector RGB10A2, clamped to [0, 1023] and [0, 3], and stores to unaligned memory.
* IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
*
* @param Vec			Vector containing 4 FLOATs
* @param Ptr			Unaligned memory pointer to store the packed RGB10A2(4 bytes).
*/
FORCEINLINE void VectorStoreURGB10A2N(const VectorRegister4Float& Vec, void* Ptr)
{
	union U { 
		VectorRegister4Float V; float F[4]; 
		FORCEINLINE U() : V() {}
	} Tmp;
	Tmp.V = VectorMax(Vec, VectorZeroFloat());
	Tmp.V = VectorMin(Tmp.V, VectorOneFloat());
	Tmp.V = VectorMultiply(Tmp.V, MakeVectorRegister(1023.0f, 1023.0f, 1023.0f, 3.0f));

	uint32* Out = (uint32*)Ptr;
	*Out = (uint32(Tmp.F[0]) & 0x3FF) << 00 |
		(uint32(Tmp.F[1]) & 0x3FF) << 10 |
		(uint32(Tmp.F[2]) & 0x3FF) << 20 |
		(uint32(Tmp.F[3]) & 0x003) << 30;
}

/**
 * Returns non-zero if any element in Vec1 is greater than the corresponding element in Vec2, otherwise 0.
 *
 * @param Vec1			1st source vector
 * @param Vec2			2nd source vector
 * @return				Non-zero integer if (Vec1.x > Vec2.x) || (Vec1.y > Vec2.y) || (Vec1.z > Vec2.z) || (Vec1.w > Vec2.w)
 */
FORCEINLINE int32 VectorAnyGreaterThan( VectorRegister4Float Vec1, VectorRegister4Float Vec2 )
{
	uint16x8_t u16x8 = (uint16x8_t)vcgtq_f32( Vec1, Vec2 );
	uint8x8_t u8x8 = (uint8x8_t)vget_low_u16( vuzpq_u16( u16x8, u16x8 ).val[0] );
	u8x8 = vuzp_u8( u8x8, u8x8 ).val[0];
	uint32_t buf[2];
	vst1_u8( (uint8_t *)buf, u8x8 );
	return (int32)buf[0]; // each byte of output corresponds to a component comparison
}

FORCEINLINE int32 VectorAnyGreaterThan(VectorRegister4Double Vec1, VectorRegister4Double Vec2)
{
	uint16x8_t u16x8_1 = (uint16x8_t)vcgtq_f64(Vec1.XY, Vec2.XY);
	uint16x8_t u16x8_2 = (uint16x8_t)vcgtq_f64(Vec1.ZW, Vec2.ZW);
	uint16x8x2_t tmp = vuzpq_u16(u16x8_1, u16x8_2);
	uint8x8_t u8x8 = (uint8x8_t)vget_low_u16(vuzpq_u16(tmp.val[0], tmp.val[0]).val[0]);
	u8x8 = vuzp_u8(u8x8, u8x8).val[0];
	uint32_t buf[2];
	vst1_u8((uint8_t*)buf, u8x8);
	return (int32)buf[0]; // each byte of output corresponds to a component comparison
}

/**
 * Resets the floating point registers so that they can be used again.
 * Some intrinsics use these for MMX purposes (e.g. VectorLoadByte4 and VectorStoreByte4).
 */
#define VectorResetFloatRegisters()

/**
 * Returns the control register.
 *
 * @return			The uint32 control register
 */
#define VectorGetControlRegister()		0

/**
 * Sets the control register.
 *
 * @param ControlStatus		The uint32 control status value to set
 */
#define	VectorSetControlRegister(ControlStatus)

/**
 * Control status bit to round all floating point math results towards zero.
 */
#define VECTOR_ROUND_TOWARD_ZERO		0


/**
* Multiplies two quaternions; the order matters.
*
* Order matters when composing quaternions: C = VectorQuaternionMultiply2(A, B) will yield a quaternion C = A * B
* that logically first applies B then A to any subsequent transformation (right first, then left).
*
* @param Quat1	Pointer to the first quaternion
* @param Quat2	Pointer to the second quaternion
* @return Quat1 * Quat2
*/
FORCEINLINE VectorRegister4Float VectorQuaternionMultiply2( const VectorRegister4Float& Quat1, const VectorRegister4Float& Quat2 )
{
	VectorRegister4Float Result = VectorMultiply(VectorReplicate(Quat1, 3), Quat2);
	Result = VectorMultiplyAdd( VectorMultiply(VectorReplicate(Quat1, 0), VectorSwizzle(Quat2, 3,2,1,0)), GlobalVectorConstants::QMULTI_SIGN_MASK0, Result);
	Result = VectorMultiplyAdd( VectorMultiply(VectorReplicate(Quat1, 1), VectorSwizzle(Quat2, 2,3,0,1)), GlobalVectorConstants::QMULTI_SIGN_MASK1, Result);
	Result = VectorMultiplyAdd( VectorMultiply(VectorReplicate(Quat1, 2), VectorSwizzle(Quat2, 1,0,3,2)), GlobalVectorConstants::QMULTI_SIGN_MASK2, Result);

	return Result;
}

FORCEINLINE VectorRegister4Double VectorQuaternionMultiply2(const VectorRegister4Double& Quat1, const VectorRegister4Double& Quat2)
{
	VectorRegister4Double Result = VectorMultiply(VectorReplicate(Quat1, 3), Quat2);
	Result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(Quat1, 0), VectorSwizzle(Quat2, 3, 2, 1, 0)), GlobalVectorConstants::DOUBLE_QMULTI_SIGN_MASK0, Result);
	Result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(Quat1, 1), VectorSwizzle(Quat2, 2, 3, 0, 1)), GlobalVectorConstants::DOUBLE_QMULTI_SIGN_MASK1, Result);
	Result = VectorMultiplyAdd(VectorMultiply(VectorReplicate(Quat1, 2), VectorSwizzle(Quat2, 1, 0, 3, 2)), GlobalVectorConstants::DOUBLE_QMULTI_SIGN_MASK2, Result);

	return Result;
}

/**
* Multiplies two quaternions; the order matters.
*
* When composing quaternions: VectorQuaternionMultiply(C, A, B) will yield a quaternion C = A * B
* that logically first applies B then A to any subsequent transformation (right first, then left).
*
* @param Result	Pointer to where the result Quat1 * Quat2 should be stored
* @param Quat1	Pointer to the first quaternion (must not be the destination)
* @param Quat2	Pointer to the second quaternion (must not be the destination)
*/
FORCEINLINE void VectorQuaternionMultiply(VectorRegister4Float* RESTRICT Result, const VectorRegister4Float* RESTRICT Quat1, const VectorRegister4Float* RESTRICT Quat2)
{
	*Result = VectorQuaternionMultiply2(*Quat1, *Quat2);
}

FORCEINLINE void VectorQuaternionMultiply(VectorRegister4Double* RESTRICT Result, const VectorRegister4Double* RESTRICT Quat1, const VectorRegister4Double* RESTRICT Quat2)
{
	*Result = VectorQuaternionMultiply2(*Quat1, *Quat2);
}

/**
* Computes the sine and cosine of each component of a Vector.
*
* @param VSinAngles	VectorRegister4Float Pointer to where the Sin result should be stored
* @param VCosAngles	VectorRegister4Float Pointer to where the Cos result should be stored
* @param VAngles VectorRegister4Float Pointer to the input angles 
*/
FORCEINLINE void VectorSinCos(  VectorRegister4Float* RESTRICT VSinAngles, VectorRegister4Float* RESTRICT VCosAngles, const VectorRegister4Float* RESTRICT VAngles )
{	
	// Map to [-pi, pi]
	// X = A - 2pi * round(A/2pi)
	// Note the round(), not truncate(). In this case round() can round halfway cases using round-to-nearest-even OR round-to-nearest.

	// Quotient = round(A/2pi)
	VectorRegister4Float Quotient = VectorMultiply(*VAngles, GlobalVectorConstants::OneOverTwoPi);
	Quotient = vcvtq_f32_s32(vcvtnq_s32_f32(Quotient)); // round to nearest even is the default rounding mode but that's fine here.

	// X = A - 2pi * Quotient
	VectorRegister4Float X = VectorNegateMultiplyAdd(GlobalVectorConstants::TwoPi, Quotient, *VAngles);

	// Map in [-pi/2,pi/2]
	VectorRegister4Float sign = VectorBitwiseAnd(X, GlobalVectorConstants::SignBit());
	VectorRegister4Float c = VectorBitwiseOr(GlobalVectorConstants::Pi, sign);  // pi when x >= 0, -pi when x < 0
	VectorRegister4Float absx = VectorAbs(X);
	VectorRegister4Float rflx = VectorSubtract(c, X);
	VectorRegister4Float comp = VectorCompareGT(absx, GlobalVectorConstants::PiByTwo);
	X = VectorSelect(comp, rflx, X);
	sign = VectorSelect(comp, GlobalVectorConstants::FloatMinusOne, GlobalVectorConstants::FloatOne);

	const VectorRegister4Float XSquared = VectorMultiply(X, X);

	// 11-degree minimax approximation
	//*ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;
	const VectorRegister4Float SinCoeff0 = MakeVectorRegister(1.0f, -0.16666667f, 0.0083333310f, -0.00019840874f);
	const VectorRegister4Float SinCoeff1 = MakeVectorRegister(2.7525562e-06f, -2.3889859e-08f, /*unused*/ 0.f, /*unused*/ 0.f);

	VectorRegister4Float S;
	S = VectorReplicate(SinCoeff1, 1);
	S = VectorMultiplyAdd(XSquared, S, VectorReplicate(SinCoeff1, 0));
	S = VectorMultiplyAdd(XSquared, S, VectorReplicate(SinCoeff0, 3));
	S = VectorMultiplyAdd(XSquared, S, VectorReplicate(SinCoeff0, 2));
	S = VectorMultiplyAdd(XSquared, S, VectorReplicate(SinCoeff0, 1));
	S = VectorMultiplyAdd(XSquared, S, VectorReplicate(SinCoeff0, 0));
	*VSinAngles = VectorMultiply(S, X);

	// 10-degree minimax approximation
	//*ScalarCos = sign * (((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f);
	const VectorRegister4Float CosCoeff0 = MakeVectorRegister(1.0f, -0.5f, 0.041666638f, -0.0013888378f);
	const VectorRegister4Float CosCoeff1 = MakeVectorRegister(2.4760495e-05f, -2.6051615e-07f, /*unused*/ 0.f, /*unused*/ 0.f);

	VectorRegister4Float C;
	C = VectorReplicate(CosCoeff1, 1);
	C = VectorMultiplyAdd(XSquared, C, VectorReplicate(CosCoeff1, 0));
	C = VectorMultiplyAdd(XSquared, C, VectorReplicate(CosCoeff0, 3));
	C = VectorMultiplyAdd(XSquared, C, VectorReplicate(CosCoeff0, 2));
	C = VectorMultiplyAdd(XSquared, C, VectorReplicate(CosCoeff0, 1));
	C = VectorMultiplyAdd(XSquared, C, VectorReplicate(CosCoeff0, 0));
	*VCosAngles = VectorMultiply(C, sign);
}

// Returns true if the vector contains a component that is either NAN or +/-infinite.
inline bool VectorContainsNaNOrInfinite(const VectorRegister4Float& Vec)
{
	// https://en.wikipedia.org/wiki/IEEE_754-1985
	// Infinity is represented with all exponent bits set, with the correct sign bit.
	// NaN is represented with all exponent bits set, plus at least one fraction/significant bit set.
	// This means finite values will not have all exponent bits set, so check against those bits.

	union { float F; uint32 U; } InfUnion;
	InfUnion.U = 0x7F800000;
	const float Inf = InfUnion.F;
	const VectorRegister4Float FloatInfinity = MakeVectorRegister(Inf, Inf, Inf, Inf);

	// Mask off Exponent
	VectorRegister4Float ExpTest = VectorBitwiseAnd(Vec, FloatInfinity);

	// Compare to full exponent & combine resulting flags into lane 0
	const int32x4_t Table = MakeVectorRegisterIntConstant(0x0C080400, 0, 0, 0);

	uint8x16_t res = (uint8x16_t)VectorCompareEQ(ExpTest, FloatInfinity);
	// If we have all zeros, all elements are finite
	return vgetq_lane_u32((uint32x4_t)vqtbx1q_u8(res, res, Table), 0) != 0;
}

inline bool VectorContainsNaNOrInfinite(const VectorRegister4Double& Vec)
{
	// https://en.wikipedia.org/wiki/IEEE_754-1985
	// Infinity is represented with all exponent bits set, with the correct sign bit.
	// NaN is represented with all exponent bits set, plus at least one fraction/significant bit set.
	// This means finite values will not have all exponent bits set, so check against those bits.

	union { double F; uint64 U; } InfUnion;
	InfUnion.U = 0x7FF0000000000000ULL;
	const double Inf = InfUnion.F;
	const VectorRegister4Double DoubleInfinity = MakeVectorRegister(Inf, Inf, Inf, Inf);

	// Mask off Exponent
	VectorRegister4Double ExpTest = VectorBitwiseAnd(Vec, DoubleInfinity);

	// Compare to full exponent & combine resulting flags into lane 0
	const int32x4_t Table = MakeVectorRegisterIntConstant(0x18100800, 0, 0, 0);

	VectorRegister4Double InfTestRes = VectorCompareEQ(ExpTest, DoubleInfinity);

	// If we have all zeros, all elements are finite
	uint8x16_t ZeroVec = vdupq_n_u8(0);
	//TODO: there must be a better instruction to just get the top bits or smth
	return vgetq_lane_u32((uint32x4_t)vqtbx2q_u8(ZeroVec, *(uint8x16x2_t*)&InfTestRes, Table), 0) != 0;
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorExp(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Exp(Val[0]), FMath::Exp(Val[1]), FMath::Exp(Val[2]), FMath::Exp(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorExp(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Exp(Val[0]), FMath::Exp(Val[1]), FMath::Exp(Val[2]), FMath::Exp(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorExp2(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Exp2(Val[0]), FMath::Exp2(Val[1]), FMath::Exp2(Val[2]), FMath::Exp2(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorExp2(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Exp2(Val[0]), FMath::Exp2(Val[1]), FMath::Exp2(Val[2]), FMath::Exp2(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorLog(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Loge(Val[0]), FMath::Loge(Val[1]), FMath::Loge(Val[2]), FMath::Loge(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorLog(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Loge(Val[0]), FMath::Loge(Val[1]), FMath::Loge(Val[2]), FMath::Loge(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorLog2(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Log2(Val[0]), FMath::Log2(Val[1]), FMath::Log2(Val[2]), FMath::Log2(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorLog2(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Log2(Val[0]), FMath::Log2(Val[1]), FMath::Log2(Val[2]), FMath::Log2(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorTan(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Tan(Val[0]), FMath::Tan(Val[1]), FMath::Tan(Val[2]), FMath::Tan(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorTan(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Tan(Val[0]), FMath::Tan(Val[1]), FMath::Tan(Val[2]), FMath::Tan(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorASin(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Asin(Val[0]), FMath::Asin(Val[1]), FMath::Asin(Val[2]), FMath::Asin(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorASin(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Asin(Val[0]), FMath::Asin(Val[1]), FMath::Asin(Val[2]), FMath::Asin(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorACos(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Acos(Val[0]), FMath::Acos(Val[1]), FMath::Acos(Val[2]), FMath::Acos(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorACos(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Acos(Val[0]), FMath::Acos(Val[1]), FMath::Acos(Val[2]), FMath::Acos(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorATan(const VectorRegister4Float& X)
{
	AlignedFloat4 Val(X);
	return MakeVectorRegister(FMath::Atan(Val[0]), FMath::Atan(Val[1]), FMath::Atan(Val[2]), FMath::Atan(Val[3]));
}

FORCEINLINE VectorRegister4Double VectorATan(const VectorRegister4Double& X)
{
	AlignedDouble4 Val(X);
	return MakeVectorRegister(FMath::Atan(Val[0]), FMath::Atan(Val[1]), FMath::Atan(Val[2]), FMath::Atan(Val[3]));
}

//TODO: Vectorize
FORCEINLINE VectorRegister4Float VectorATan2(const VectorRegister4Float& X, const VectorRegister4Float& Y)
{
	AlignedFloat4 ValX(X);
	AlignedFloat4 ValY(Y);

	return MakeVectorRegister(FMath::Atan2(ValX[0], ValY[0]),
							  FMath::Atan2(ValX[1], ValY[1]),
							  FMath::Atan2(ValX[2], ValY[2]),
							  FMath::Atan2(ValX[3], ValY[3]));
}

FORCEINLINE VectorRegister4Double VectorATan2(const VectorRegister4Double& X, const VectorRegister4Double& Y)
{
	AlignedDouble4 ValX(X);
	AlignedDouble4 ValY(Y);

	return MakeVectorRegister(FMath::Atan2(ValX[0], ValY[0]),
							  FMath::Atan2(ValX[1], ValY[1]),
							  FMath::Atan2(ValX[2], ValY[2]),
							  FMath::Atan2(ValX[3], ValY[3]));
}

FORCEINLINE VectorRegister4Float VectorCeil(const VectorRegister4Float& X)
{
	return vrndpq_f32(X);
}

FORCEINLINE VectorRegister4Double VectorCeil(const VectorRegister4Double& X)
{
	VectorRegister4Double Result;
	Result.XY = vrndpq_f64(X.XY);
	Result.ZW = vrndpq_f64(X.ZW);
	return Result;
}

FORCEINLINE VectorRegister4Float VectorFloor(const VectorRegister4Float& X)
{
	return vrndmq_f32(X);
}

FORCEINLINE VectorRegister4Double VectorFloor(const VectorRegister4Double& X)
{
	VectorRegister4Double Result;
	Result.XY = vrndmq_f64(X.XY);
	Result.ZW = vrndmq_f64(X.ZW);
	return Result;
}

FORCEINLINE VectorRegister4Float VectorTruncate(const VectorRegister4Float& X)
{
	return vrndq_f32(X);
}

FORCEINLINE VectorRegister4Double VectorTruncate(const VectorRegister4Double& X)
{
	VectorRegister4Double Result;
	Result.XY = vrndq_f64(X.XY);
	Result.ZW = vrndq_f64(X.ZW);
	return Result;
}

FORCEINLINE VectorRegister4Float VectorMod(const VectorRegister4Float& X, const VectorRegister4Float& Y)
{
	VectorRegister4Float Div = VectorDivide(X, Y);
	// Floats where abs(f) >= 2^23 have no fractional portion, and larger values would overflow VectorTruncate.
	VectorRegister4Float NoFractionMask = VectorCompareGE(VectorAbs(Div), GlobalVectorConstants::FloatNonFractional);
	VectorRegister4Float Temp = VectorSelect(NoFractionMask, Div, VectorTruncate(Div));
	VectorRegister4Float Result = VectorNegateMultiplyAdd(Y, Temp, X);
	// Clamp to [-AbsY, AbsY] because of possible failures for very large numbers (>1e10) due to precision loss.
	VectorRegister4Float AbsY = VectorAbs(Y);
	return vmaxnmq_f32(VectorNegate(AbsY), vminnmq_f32(Result, AbsY));
}

FORCEINLINE VectorRegister4Double VectorMod(const VectorRegister4Double& X, const VectorRegister4Double& Y)
{
	VectorRegister4Double Div = VectorDivide(X, Y);
	// Floats where abs(f) >= 2^52 have no fractional portion, and larger values would overflow VectorTruncate.
	VectorRegister4Double NoFractionMask = VectorCompareGE(VectorAbs(Div), GlobalVectorConstants::DoubleNonFractional);
	VectorRegister4Double Temp = VectorSelect(NoFractionMask, Div, VectorTruncate(Div));
	VectorRegister4Double Result = VectorNegateMultiplyAdd(Y, Temp, X);
	// Clamp to [-AbsY, AbsY] because of possible failures for very large numbers (>1e10) due to precision loss.
	VectorRegister4Double AbsY = VectorAbs(Y);
	VectorRegister4Double NegAbsY = VectorNegate(AbsY);
	
	VectorRegister4Double Min;
	Min.XY = vminnmq_f64(Result.XY, AbsY.XY);
	Min.ZW = vminnmq_f64(Result.ZW, AbsY.ZW);
	
	VectorRegister4Double Max;
	Max.XY = vmaxnmq_f64(NegAbsY.XY, Min.XY);
	Max.ZW = vmaxnmq_f64(NegAbsY.ZW, Min.ZW);
	return Max;
}

FORCEINLINE VectorRegister4Float VectorSign(const VectorRegister4Float& X)
{
	VectorRegister4Float Mask = VectorCompareGE(X, GlobalVectorConstants::FloatZero);
	return VectorSelect(Mask, GlobalVectorConstants::FloatOne, GlobalVectorConstants::FloatMinusOne);
}

FORCEINLINE VectorRegister4Double VectorSign(const VectorRegister4Double& X)
{
	VectorRegister4Double Mask = VectorCompareGE(X, GlobalVectorConstants::DoubleZero);
	return VectorSelect(Mask, GlobalVectorConstants::DoubleOne, GlobalVectorConstants::DoubleMinusOne);
}

FORCEINLINE VectorRegister4Float VectorStep(const VectorRegister4Float& X)
{
	VectorRegister4Float Mask = VectorCompareGE(X, GlobalVectorConstants::FloatZero);
	return VectorSelect(Mask, GlobalVectorConstants::FloatOne, GlobalVectorConstants::FloatZero);
}

FORCEINLINE VectorRegister4Double VectorStep(const VectorRegister4Double& X)
{
	VectorRegister4Double Mask = VectorCompareGE(X, GlobalVectorConstants::DoubleZero);
	return VectorSelect(Mask, GlobalVectorConstants::DoubleOne, GlobalVectorConstants::DoubleZero);
}

namespace VectorSinConstantsNEON
{
	static const float p = 0.225f;
	static const float a = 7.58946609f; // 16 * sqrtf(p)
	static const float b = 1.63384342f; // (1 - p) / sqrtf(p)
	static const VectorRegister4Float A = MakeVectorRegisterConstant(a, a, a, a);
	static const VectorRegister4Float B = MakeVectorRegisterConstant(b, b, b, b);
}

FORCEINLINE VectorRegister4Float VectorSin(const VectorRegister4Float& X)
{
	//Sine approximation using a squared parabola restrained to f(0) = 0, f(PI) = 0, f(PI/2) = 1.
	//based on a good discussion here http://forum.devmaster.net/t/fast-and-accurate-sine-cosine/9648
	//After approx 2.5 million tests comparing to sin(): 
	//Average error of 0.000128
	//Max error of 0.001091
	//
	// Error clarification - the *relative* error rises above 1.2% near
	// 0 and PI (as the result nears 0). This is enough to introduce 
	// harmonic distortion when used as an oscillator - VectorSinCos
	// doesn't cost that much more and is significantly more accurate.
	// (though don't use either for an oscillator if you care about perf)

	VectorRegister4Float Y = VectorMultiply(X, GlobalVectorConstants::OneOverTwoPi);
	Y = VectorSubtract(Y, VectorFloor(VectorAdd(Y, GlobalVectorConstants::FloatOneHalf)));
	Y = VectorMultiply(VectorSinConstantsNEON::A, VectorMultiply(Y, VectorSubtract(GlobalVectorConstants::FloatOneHalf, VectorAbs(Y))));
	return VectorMultiply(Y, VectorAdd(VectorSinConstantsNEON::B, VectorAbs(Y)));
}

FORCEINLINE VectorRegister4Double VectorSin(const VectorRegister4Double& X)
{
	AlignedDouble4 Doubles(X);
	Doubles[0] = FMath::Sin(Doubles[0]);
	Doubles[1] = FMath::Sin(Doubles[1]);
	Doubles[2] = FMath::Sin(Doubles[2]);
	Doubles[3] = FMath::Sin(Doubles[3]);
	return Doubles.ToVectorRegister();
}

FORCEINLINE VectorRegister4Float VectorCos(const VectorRegister4Float& X)
{
	return VectorSin(VectorAdd(X, GlobalVectorConstants::PiByTwo));
}

FORCEINLINE VectorRegister4Double VectorCos(const VectorRegister4Double& X)
{
	AlignedDouble4 Doubles(X);
	Doubles[0] = FMath::Cos(Doubles[0]);
	Doubles[1] = FMath::Cos(Doubles[1]);
	Doubles[2] = FMath::Cos(Doubles[2]);
	Doubles[3] = FMath::Cos(Doubles[3]);
	return Doubles.ToVectorRegister();
}

FORCEINLINE void VectorSinCos(VectorRegister4Double* RESTRICT VSinAngles, VectorRegister4Double* RESTRICT VCosAngles, const VectorRegister4Double* RESTRICT VAngles)
{
	*VSinAngles = VectorSin(*VAngles);
	*VCosAngles = VectorCos(*VAngles);
}

/**
* Loads packed RGBA16(4 bytes) from unaligned memory and converts them into 4 FLOATs.
* IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
*
* @param Ptr			Unaligned memory pointer to the RGBA16(8 bytes).
* @return				VectorRegister4Float with 4 FLOATs loaded from Ptr.
*/
FORCEINLINE VectorRegister4Float VectorLoadURGBA16N(const uint16* E)
{
	alignas(16) float V[4];
	V[0] = float(E[0]);
	V[1] = float(E[1]);
	V[2] = float(E[2]);
	V[3] = float(E[3]);

	VectorRegister4Float Vec = VectorLoad(V);
	VectorRegister4Float Div = vdupq_n_f32(1.0f / 65535.0f);
	return VectorMultiply(Vec, Div);
}

/**
* Loads packed signed RGBA16(4 bytes) from unaligned memory and converts them into 4 FLOATs.
* IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
*
* @param Ptr			Unaligned memory pointer to the RGBA16(8 bytes).
* @return				VectorRegister4Float with 4 FLOATs loaded from Ptr.
*/
FORCEINLINE VectorRegister4Float VectorLoadSRGBA16N(const void* Ptr)
{
	alignas(16) float V[4];
	int16* E = (int16*)Ptr;

	V[0] = float(E[0]);
	V[1] = float(E[1]);
	V[2] = float(E[2]);
	V[3] = float(E[3]);

	VectorRegister4Float Vec = VectorLoad(V);
	VectorRegister4Float Div = vdupq_n_f32(1.0f / 32767.0f);
	return VectorMultiply(Vec, Div);
}

/**
* Converts the 4 FLOATs in the vector RGBA16, clamped to [0, 65535], and stores to unaligned memory.
* IMPORTANT: You need to call VectorResetFloatRegisters() before using scalar FLOATs after you've used this intrinsic!
*
* @param Vec			Vector containing 4 FLOATs
* @param Ptr			Unaligned memory pointer to store the packed RGBA16(8 bytes).
*/
FORCEINLINE void VectorStoreURGBA16N(const VectorRegister4Float& Vec, uint16* Out)
{
	VectorRegister4Float Tmp;
	Tmp = VectorMax(Vec, VectorZeroFloat());
	Tmp = VectorMin(Tmp, VectorOneFloat());
	Tmp = VectorMultiplyAdd(Tmp, vdupq_n_f32(65535.0f), vdupq_n_f32(0.5f));
	Tmp = VectorTruncate(Tmp);

	alignas(16) float F[4];
	VectorStoreAligned(Tmp, F);

	Out[0] = (uint16)F[0];
	Out[1] = (uint16)F[1];
	Out[2] = (uint16)F[2];
	Out[3] = (uint16)F[3];
}

//////////////////////////////////////////////////////////////////////////
//Integer ops

//Bitwise
/** = a & b */
#define VectorIntAnd(A, B)		vandq_s32(A, B)
/** = a | b */
#define VectorIntOr(A, B)		vorrq_s32(A, B)
/** = a ^ b */
#define VectorIntXor(A, B)		veorq_s32(A, B)
/** = (~a) & b to match _mm_andnot_si128 */
#define VectorIntAndNot(A, B)	vandq_s32(vmvnq_s32(A), B)
/** = ~a */
#define VectorIntNot(A)	vmvnq_s32(A)

//Comparison
#define VectorIntCompareEQ(A, B)	vceqq_s32(A,B)
#define VectorIntCompareNEQ(A, B)	VectorIntNot(VectorIntCompareEQ(A,B))
#define VectorIntCompareGT(A, B)	vcgtq_s32(A,B)
#define VectorIntCompareLT(A, B)	vcltq_s32(A,B)
#define VectorIntCompareGE(A, B)	vcgeq_s32(A,B)
#define VectorIntCompareLE(A, B)	vcleq_s32(A,B)


FORCEINLINE VectorRegister4Int VectorIntSelect(const VectorRegister4Int& Mask, const VectorRegister4Int& Vec1, const VectorRegister4Int& Vec2)
{
	return VectorIntXor(Vec2, VectorIntAnd(Mask, VectorIntXor(Vec1, Vec2)));
}

//Arithmetic
#define VectorIntAdd(A, B)	vaddq_s32(A, B)
#define VectorIntSubtract(A, B)	vsubq_s32(A, B)
#define VectorIntMultiply(A, B) vmulq_s32(A, B)
#define VectorIntNegate(A) vnegq_s32(A)
#define VectorIntMin(A, B) vminq_s32(A,B)
#define VectorIntMax(A, B) vmaxq_s32(A,B)
#define VectorIntAbs(A) vabdq_s32(A, GlobalVectorConstants::IntZero)

#define VectorIntSign(A) VectorIntSelect( VectorIntCompareGE(A, GlobalVectorConstants::IntZero), GlobalVectorConstants::IntOne, GlobalVectorConstants::IntMinusOne )

#define VectorIntToFloat(A) vcvtq_f32_s32(A)

FORCEINLINE VectorRegister4Int VectorFloatToInt(const VectorRegister4Float& A)
{
	return vcvtq_s32_f32(A);
}

FORCEINLINE VectorRegister4Int VectorFloatToInt(const VectorRegister4Double& A)
{
	return VectorFloatToInt(MakeVectorRegisterFloatFromDouble(A));
}

//Loads and stores

/**
* Stores a vector to memory (aligned or unaligned).
*
* @param Vec	Vector to store
* @param Ptr	Memory pointer
*/
#define VectorIntStore( Vec, Ptr )			vst1q_s32( (int32*)(Ptr), Vec )

/**
* Loads 4 int32s from unaligned memory.
*
* @param Ptr	Unaligned memory pointer to the 4 int32s
* @return		VectorRegister4Int(Ptr[0], Ptr[1], Ptr[2], Ptr[3])
*/
#define VectorIntLoad( Ptr )				vld1q_s32( (int32*)((void*)(Ptr)) )

/**
* Stores a vector to memory (aligned).
*
* @param Vec	Vector to store
* @param Ptr	Aligned Memory pointer
*/
#define VectorIntStoreAligned( Vec, Ptr )			vst1q_s32( (int32*)(Ptr), Vec )

/**
* Loads 4 int32s from aligned memory.
*
* @param Ptr	Aligned memory pointer to the 4 int32s
* @return		VectorRegister4Int(Ptr[0], Ptr[1], Ptr[2], Ptr[3])
*/
#define VectorIntLoadAligned( Ptr )				vld1q_s32( (int32*)((void*)(Ptr)) )

/**
* Loads 1 int32 from unaligned memory into all components of a vector register.
*
* @param Ptr	Unaligned memory pointer to the 4 int32s
* @return		VectorRegister4Int(*Ptr, *Ptr, *Ptr, *Ptr)
*/
#define VectorIntLoad1( Ptr )	vld1q_dup_s32((int32*)(Ptr))

// To be continued...

PRAGMA_ENABLE_SHADOW_VARIABLE_WARNINGS
