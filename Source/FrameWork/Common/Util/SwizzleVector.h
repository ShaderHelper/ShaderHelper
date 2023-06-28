#pragma once
#include "Auxiliary.h"
namespace FRAMEWORK {
	// ShaderHelper Vector
	// fwd
	template<typename T> struct VectorImpl;
	template<typename T> struct Vector2Impl;
	template<typename T> struct Vector4Impl;

	using Vector = VectorImpl<double>;
	using Vector3f = VectorImpl<float>;
	using Vector3d = VectorImpl<double>;

	using Vector2D = Vector2Impl<double>;
	using Vector2f = Vector2Impl<float>;
	using Vector2d = Vector2Impl<double>;

	using Vector4 = Vector4Impl<double>;
	using Vector4f = Vector4Impl<float>;
	using Vector4d = Vector4Impl<double>;

	template<uint32 ElementNum, typename Base, typename Target, uint32... Indexes>
	struct Swizzle
	{
		template <uint32 OtherElementNum, typename OtherBase, typename OtherTarget, uint32 ... OtherIndexes>
		friend struct Swizzle;
	public:
		operator Target() const {
			return Target(Data[Indexes]...);
		}

		Target& operator=(const Target& rhs) const {
			Data = { rhs[Indexes]... };
			return *this;
		}

		friend Target operator+(const Target& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>::template Sum<Target>(lhs, rhs.Data);
		}
		friend Target operator*(const Target& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>::template Mul<Target>(lhs, rhs.Data);
		}
		friend Target operator-(const Target& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>::template Sub<Target>(lhs, rhs.Data);
		}
		friend Target operator/(const Target& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>::template Div<Target>(lhs, rhs.Data);
		}

		friend Target operator+(const Swizzle& lhs, const Target& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::template Sum<Target>(lhs.Data, rhs);
		}
		friend Target operator*(const Swizzle& lhs, const Target& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::template Mul<Target>(lhs.Data, rhs);
		}
		friend Target operator-(const Swizzle& lhs, const Target& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::template Sub<Target>(lhs.Data, rhs);
		}
		friend Target operator/(const Swizzle& lhs, const Target& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::template Div<Target>(lhs.Data, rhs);
		}

		template<uint32 OtherElementNum, uint32 ... OtherIndexes>
		Target operator+(const Swizzle<OtherElementNum, Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Sum<Target>(Data, rhs.Data);
		}
		template<uint32 OtherElementNum, uint32 ... OtherIndexes>
		Target operator*(const Swizzle<OtherElementNum, Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Mul<Target>(Data, rhs.Data);
		}
		template<uint32 OtherElementNum, uint32 ... OtherIndexes>
		Target operator-(const Swizzle<OtherElementNum, Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Sub<Target>(Data, rhs.Data);
		}
		template<uint32 OtherElementNum, uint32 ... OtherIndexes>
		Target operator/(const Swizzle<OtherElementNum, Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Div<Target>(Data, rhs.Data);
		}

	private:
		Base Data[ElementNum];
	};

	template<typename T>
	struct Vector2Impl
	{
		using UE_Vector = UE::Math::TVector2<T>;
		union
		{
			//Working UB
			T Data[2];
			struct
			{
				T X, Y;
			};
			Swizzle<2, T, Vector2Impl, 0, 0> XX;
			Swizzle<2, T, Vector2Impl, 0, 1> XY;
			Swizzle<2, T, Vector2Impl, 1, 0> YX;
			Swizzle<2, T, Vector2Impl, 1, 1> YY;

			Swizzle<2, T, VectorImpl<T>, 0, 0, 0> XXX;
			Swizzle<2, T, VectorImpl<T>, 0, 0, 1> XXY;
			Swizzle<2, T, VectorImpl<T>, 0, 1, 0> XYX;
			Swizzle<2, T, VectorImpl<T>, 0, 1, 1> XYY;
			Swizzle<2, T, VectorImpl<T>, 1, 0, 0> YXX;
			Swizzle<2, T, VectorImpl<T>, 1, 0, 1> YXY;
			Swizzle<2, T, VectorImpl<T>, 1, 1, 0> YYX;
			Swizzle<2, T, VectorImpl<T>, 1, 1, 1> YYY;

			Swizzle<2, T, Vector4Impl<T>, 0, 0, 0, 0> XXXX;
			Swizzle<2, T, Vector4Impl<T>, 0, 0, 0, 1> XXXY;
			Swizzle<2, T, Vector4Impl<T>, 0, 0, 1, 0> XXYX;
			Swizzle<2, T, Vector4Impl<T>, 0, 0, 1, 1> XXYY;
			Swizzle<2, T, Vector4Impl<T>, 0, 1, 0, 0> XYXX;
			Swizzle<2, T, Vector4Impl<T>, 0, 1, 0, 1> XYXY;
			Swizzle<2, T, Vector4Impl<T>, 0, 1, 1, 0> XYYX;
			Swizzle<2, T, Vector4Impl<T>, 0, 1, 1, 1> XYYY;
			Swizzle<2, T, Vector4Impl<T>, 1, 0, 0, 0> YXXX;
			Swizzle<2, T, Vector4Impl<T>, 1, 0, 0, 1> YXXY;
			Swizzle<2, T, Vector4Impl<T>, 1, 0, 1, 0> YXYX;
			Swizzle<2, T, Vector4Impl<T>, 1, 0, 1, 1> YXYY;
			Swizzle<2, T, Vector4Impl<T>, 1, 1, 0, 0> YYXX;
			Swizzle<2, T, Vector4Impl<T>, 1, 1, 0, 1> YYXY;
			Swizzle<2, T, Vector4Impl<T>, 1, 1, 1, 0> YYYX;
			Swizzle<2, T, Vector4Impl<T>, 1, 1, 1, 1> YYYY;
		};

		Vector2Impl() = default;
		Vector2Impl(const UE_Vector& Vec) : X(Vec.X), Y(Vec.Y) {}
		Vector2Impl(T IntT) : X(IntT), Y(IntT) {}
		Vector2Impl(T IntX, T IntY) : X(IntX), Y(IntY) {}

		Vector2Impl operator*(const Vector2Impl& rhs) const {
			return Vector2Impl(X * rhs.X, Y * rhs.Y);
		}
		Vector2Impl operator+(const Vector2Impl& rhs) const {
			return Vector2Impl(X + rhs.X, Y + rhs.Y);
		}
		Vector2Impl operator-(const Vector2Impl& rhs) const {
			return Vector2Impl(X - rhs.X, Y - rhs.Y);
		}
		Vector2Impl operator/(const Vector2Impl& rhs) const {
			return Vector2Impl(X / rhs.X, Y / rhs.Y);
		}

		Vector2Impl& operator*=(const Vector2Impl& rhs) {
			X *= rhs.X; Y *= rhs.Y;
			return *this;
		}
		Vector2Impl& operator+=(const Vector2Impl& rhs) {
			X += rhs.X; Y += rhs.Y;
			return *this;
		}
		Vector2Impl& operator-=(const Vector2Impl& rhs) {
			X -= rhs.X; Y -= rhs.Y;
			return *this;
		}
		Vector2Impl& operator/=(const Vector2Impl& rhs) {
			X /= rhs.X; Y /= rhs.Y;
			return *this;
		}

		bool operator<(const Vector2Impl& rhs) const {
			return X < rhs.X&& Y < rhs.Y;
		}
		bool operator>(const Vector2Impl& rhs) const {
			return X > rhs.X && Y > rhs.Y;
		}
		bool operator<=(const Vector2Impl& rhs) const {
			return X <= rhs.X && Y <= rhs.Y;
		}
		bool operator>=(const Vector2Impl& rhs) const {
			return X >= rhs.X && Y >= rhs.Y;
		}

		bool Equals(const Vector2Impl& rhs, T eps = std::numeric_limits<T>::epsilon()) const {
			return FMath::Abs(X - rhs.X) < eps && FMath::Abs(Y - rhs.Y) < eps;
		}

		Vector2Impl operator*(T rhs) const {
			return Vector2Impl(X * rhs, Y * rhs);
		}
		Vector2Impl operator+(T rhs) const {
			return Vector2Impl(X + rhs, Y + rhs);
		}
		Vector2Impl operator-(T rhs) const {
			return Vector2Impl(X - rhs, Y - rhs);
		}
		Vector2Impl operator/(T rhs) const {
			return Vector2Impl(X / rhs, Y / rhs);
		}

		const T& operator[](uint32 Index) const {
			//Maybe break strict-aliasing rule?
			//Even if the msvc doesn't implement this "strict aliasing" optimization at present and we don't choose the gcc compiler
			//follow the c++ standard as far as possible.
			//return *(reinterpret_cast<const T*>(this) + Index);
			checkf(Index >= 0 && Index < 2, TEXT("Invalid Index"));
			return Data[Index];
		}

		T& operator[](uint32 Index) {
			checkf(Index >= 0 && Index < 2, TEXT("Invalid Index"));
			return Data[Index];
		}

		operator UE_Vector() {
			return UE_Vector(X, Y);
		}

		FString ToString() const {
			return FString::Printf(TEXT("X=%3.3f Y=%3.3f"), X, Y);
		}

		const T* GetData() const {
			return Data;
		}
	};

	template<typename T>
	struct VectorImpl
	{
		using UE_Vector = UE::Math::TVector<T>;
		union
		{
			T Data[3];
			struct
			{
				T X, Y, Z;
			};
			Swizzle<3, T, Vector2Impl<T>, 0, 0> XX;
			Swizzle<3, T, Vector2Impl<T>, 0, 1> XY;
			Swizzle<3, T, Vector2Impl<T>, 1, 0> YX;
			Swizzle<3, T, Vector2Impl<T>, 1, 1> YY;
			Swizzle<3, T, Vector2Impl<T>, 0, 2> XZ;
			Swizzle<3, T, Vector2Impl<T>, 1, 2> YZ;
			Swizzle<3, T, Vector2Impl<T>, 2, 0> ZX;
			Swizzle<3, T, Vector2Impl<T>, 2, 1> ZY;
			Swizzle<3, T, Vector2Impl<T>, 2, 2> ZZ;

			Swizzle<3, T, VectorImpl, 0, 0, 0> XXX;
			Swizzle<3, T, VectorImpl, 0, 0, 1> XXY;
			Swizzle<3, T, VectorImpl, 0, 0, 2> XXZ;
			Swizzle<3, T, VectorImpl, 0, 1, 0> XYX;
			Swizzle<3, T, VectorImpl, 0, 1, 1> XYY;
			Swizzle<3, T, VectorImpl, 0, 1, 2> XYZ;
			Swizzle<3, T, VectorImpl, 0, 2, 0> XZX;
			Swizzle<3, T, VectorImpl, 0, 2, 1> XZY;
			Swizzle<3, T, VectorImpl, 0, 2, 2> XZZ;
			Swizzle<3, T, VectorImpl, 1, 0, 0> YXX;
			Swizzle<3, T, VectorImpl, 1, 0, 1> YXY;
			Swizzle<3, T, VectorImpl, 1, 0, 2> YXZ;
			Swizzle<3, T, VectorImpl, 1, 1, 0> YYX;
			Swizzle<3, T, VectorImpl, 1, 1, 1> YYY;
			Swizzle<3, T, VectorImpl, 1, 1, 2> YYZ;
			Swizzle<3, T, VectorImpl, 1, 2, 0> YZX;
			Swizzle<3, T, VectorImpl, 1, 2, 1> YZY;
			Swizzle<3, T, VectorImpl, 1, 2, 2> YZZ;
			Swizzle<3, T, VectorImpl, 2, 0, 0> ZXX;
			Swizzle<3, T, VectorImpl, 2, 0, 1> ZXY;
			Swizzle<3, T, VectorImpl, 2, 0, 2> ZXZ;
			Swizzle<3, T, VectorImpl, 2, 1, 0> ZYX;
			Swizzle<3, T, VectorImpl, 2, 1, 1> ZYY;
			Swizzle<3, T, VectorImpl, 2, 1, 2> ZYZ;
			Swizzle<3, T, VectorImpl, 2, 2, 0> ZZX;
			Swizzle<3, T, VectorImpl, 2, 2, 1> ZZY;
			Swizzle<3, T, VectorImpl, 2, 2, 2> ZZZ;

			Swizzle<3, T, Vector4Impl<T>, 0, 0, 0, 0> XXXX;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 0, 1> XXXY;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 0, 2> XXXZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 1, 0> XXYX;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 1, 1> XXYY;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 1, 2> XXYZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 2, 0> XXZX;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 2, 1> XXZY;
			Swizzle<3, T, Vector4Impl<T>, 0, 0, 2, 2> XXZZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 0, 0> XYXX;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 0, 1> XYXY;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 0, 2> XYXZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 1, 0> XYYX;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 1, 1> XYYY;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 1, 2> XYYZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 2, 0> XYZX;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 2, 1> XYZY;
			Swizzle<3, T, Vector4Impl<T>, 0, 1, 2, 2> XYZZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 0, 0> XZXX;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 0, 1> XZXY;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 0, 2> XZXZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 1, 0> XZYX;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 1, 1> XZYY;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 1, 2> XZYZ;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 2, 0> XZZX;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 2, 1> XZZY;
			Swizzle<3, T, Vector4Impl<T>, 0, 2, 2, 2> XZZZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 0, 0> YXXX;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 0, 1> YXXY;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 0, 2> YXXZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 1, 0> YXYX;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 1, 1> YXYY;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 1, 2> YXYZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 2, 0> YXZX;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 2, 1> YXZY;
			Swizzle<3, T, Vector4Impl<T>, 1, 0, 2, 2> YXZZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 0, 0> YYXX;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 0, 1> YYXY;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 0, 2> YYXZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 1, 0> YYYX;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 1, 1> YYYY;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 1, 2> YYYZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 2, 0> YYZX;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 2, 1> YYZY;
			Swizzle<3, T, Vector4Impl<T>, 1, 1, 2, 2> YYZZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 0, 0> YZXX;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 0, 1> YZXY;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 0, 2> YZXZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 1, 0> YZYX;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 1, 1> YZYY;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 1, 2> YZYZ;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 2, 0> YZZX;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 2, 1> YZZY;
			Swizzle<3, T, Vector4Impl<T>, 1, 2, 2, 2> YZZZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 0, 0> ZXXX;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 0, 1> ZXXY;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 0, 2> ZXXZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 1, 0> ZXYX;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 1, 1> ZXYY;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 1, 2> ZXYZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 2, 0> ZXZX;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 2, 1> ZXZY;
			Swizzle<3, T, Vector4Impl<T>, 2, 0, 2, 2> ZXZZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 0, 0> ZYXX;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 0, 1> ZYXY;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 0, 2> ZYXZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 1, 0> ZYYX;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 1, 1> ZYYY;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 1, 2> ZYYZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 2, 0> ZYZX;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 2, 1> ZYZY;
			Swizzle<3, T, Vector4Impl<T>, 2, 1, 2, 2> ZYZZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 0, 0> ZZXX;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 0, 1> ZZXY;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 0, 2> ZZXZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 1, 0> ZZYX;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 1, 1> ZZYY;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 1, 2> ZZYZ;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 2, 0> ZZZX;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 2, 1> ZZZY;
			Swizzle<3, T, Vector4Impl<T>, 2, 2, 2, 2> ZZZZ;
		};

		VectorImpl() = default;
		VectorImpl(const UE_Vector& Vec) : X(Vec.X), Y(Vec.Y), Z(Vec.Z) {}
		VectorImpl(T IntT) : X(IntT), Y(IntT), Z(IntT) {}
		VectorImpl(T IntX, T IntY, T IntZ) : X(IntX), Y(IntY), Z(IntZ) {}

		VectorImpl operator*(const VectorImpl& rhs) const {
			return VectorImpl(X * rhs.X, Y * rhs.Y, Z * rhs.Z);
		}
		VectorImpl operator+(const VectorImpl& rhs) const {
			return VectorImpl(X + rhs.X, Y + rhs.Y, Z + rhs.Z);
		}
		VectorImpl operator-(const VectorImpl& rhs) const {
			return VectorImpl(X - rhs.X, Y - rhs.Y, Z - rhs.Z);
		}
		VectorImpl operator/(const VectorImpl& rhs) const {
			return VectorImpl(X / rhs.X, Y / rhs.Y, Z / rhs.Z);
		}

		VectorImpl& operator*=(const VectorImpl& rhs) {
			X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z;
			return *this;
		}
		VectorImpl& operator+=(const VectorImpl& rhs) {
			X += rhs.X; Y += rhs.Y; Z += rhs.Z;
			return *this;
		}
		VectorImpl& operator-=(const VectorImpl& rhs) {
			X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z;
			return *this;
		}
		VectorImpl& operator/=(const VectorImpl& rhs) {
			X /= rhs.X; Y /= rhs.Y; Z /= rhs.Z;
			return *this;
		}

		bool Equals(const VectorImpl& rhs, T eps = std::numeric_limits<T>::epsilon()) const {
			return FMath::Abs(X - rhs.X) < eps && FMath::Abs(Y - rhs.Y) < eps && FMath::Abs(Z - rhs.Z) < eps;
		}

		bool operator<(const VectorImpl& rhs) const {
			return X < rhs.X&& Y < rhs.Y&& Z < rhs.Z;
		}
		bool operator>(const VectorImpl& rhs) const {
			return X > rhs.X && Y > rhs.Y && Z > rhs.Z;
		}
		bool operator<=(const VectorImpl& rhs) const {
			return X <= rhs.X && Y <= rhs.Y && Z <= rhs.Z;
		}
		bool operator>=(const VectorImpl& rhs) const {
			return X >= rhs.X && Y >= rhs.Y && Z >= rhs.Z;
		}

		VectorImpl operator*(T rhs) const {
			return VectorImpl(X * rhs, Y * rhs, Z * rhs);
		}
		VectorImpl operator+(T rhs) const {
			return VectorImpl(X + rhs, Y + rhs, Z + rhs);
		}
		VectorImpl operator-(T rhs) const {
			return VectorImpl(X - rhs, Y - rhs, Z - rhs);
		}
		VectorImpl operator/(T rhs) const {
			return VectorImpl(X / rhs, Y / rhs, Z / rhs);
		}

		const T& operator[](uint32 Index) const {
			checkf(Index >= 0 && Index < 3, TEXT("Invalid Index"));
			return Data[Index];
		}

		T& operator[](uint32 Index) {
			checkf(Index >= 0 && Index < 3, TEXT("Invalid Index"));
			return Data[Index];
		}

		operator UE_Vector() {
			return UE_Vector(X, Y, Z);
		}

		FString ToString() const {
			return FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f"), X, Y, Z);
		}

		const T* GetData() const {
			return Data;
		}
	};

	template<typename T>
	struct alignas(16) Vector4Impl
	{
		using UE_Vector = UE::Math::TVector4<T>;
		union
		{
			T Data[4];
			struct
			{
				T X, Y, Z, W;
			};
			Swizzle<4, T, Vector2Impl<T>, 0, 0> XX;
			Swizzle<4, T, Vector2Impl<T>, 0, 1> XY;
			Swizzle<4, T, Vector2Impl<T>, 0, 2> XZ;
			Swizzle<4, T, Vector2Impl<T>, 0, 3> XW;
			Swizzle<4, T, Vector2Impl<T>, 1, 0> YX;
			Swizzle<4, T, Vector2Impl<T>, 1, 1> YY;
			Swizzle<4, T, Vector2Impl<T>, 1, 2> YZ;
			Swizzle<4, T, Vector2Impl<T>, 1, 3> YW;
			Swizzle<4, T, Vector2Impl<T>, 2, 0> ZX;
			Swizzle<4, T, Vector2Impl<T>, 2, 1> ZY;
			Swizzle<4, T, Vector2Impl<T>, 2, 2> ZZ;
			Swizzle<4, T, Vector2Impl<T>, 2, 3> ZW;
			Swizzle<4, T, Vector2Impl<T>, 3, 0> WX;
			Swizzle<4, T, Vector2Impl<T>, 3, 1> WY;
			Swizzle<4, T, Vector2Impl<T>, 3, 2> WZ;
			Swizzle<4, T, Vector2Impl<T>, 3, 3> WW;

			Swizzle<4, T, VectorImpl<T>, 0, 0, 0> XXX;
			Swizzle<4, T, VectorImpl<T>, 0, 0, 1> XXY;
			Swizzle<4, T, VectorImpl<T>, 0, 0, 2> XXZ;
			Swizzle<4, T, VectorImpl<T>, 0, 0, 3> XXW;
			Swizzle<4, T, VectorImpl<T>, 0, 1, 0> XYX;
			Swizzle<4, T, VectorImpl<T>, 0, 1, 1> XYY;
			Swizzle<4, T, VectorImpl<T>, 0, 1, 2> XYZ;
			Swizzle<4, T, VectorImpl<T>, 0, 1, 3> XYW;
			Swizzle<4, T, VectorImpl<T>, 0, 2, 0> XZX;
			Swizzle<4, T, VectorImpl<T>, 0, 2, 1> XZY;
			Swizzle<4, T, VectorImpl<T>, 0, 2, 2> XZZ;
			Swizzle<4, T, VectorImpl<T>, 0, 2, 3> XZW;
			Swizzle<4, T, VectorImpl<T>, 0, 3, 0> XWX;
			Swizzle<4, T, VectorImpl<T>, 0, 3, 1> XWY;
			Swizzle<4, T, VectorImpl<T>, 0, 3, 2> XWZ;
			Swizzle<4, T, VectorImpl<T>, 0, 3, 3> XWW;
			Swizzle<4, T, VectorImpl<T>, 1, 0, 0> YXX;
			Swizzle<4, T, VectorImpl<T>, 1, 0, 1> YXY;
			Swizzle<4, T, VectorImpl<T>, 1, 0, 2> YXZ;
			Swizzle<4, T, VectorImpl<T>, 1, 0, 3> YXW;
			Swizzle<4, T, VectorImpl<T>, 1, 1, 0> YYX;
			Swizzle<4, T, VectorImpl<T>, 1, 1, 1> YYY;
			Swizzle<4, T, VectorImpl<T>, 1, 1, 2> YYZ;
			Swizzle<4, T, VectorImpl<T>, 1, 1, 3> YYW;
			Swizzle<4, T, VectorImpl<T>, 1, 2, 0> YZX;
			Swizzle<4, T, VectorImpl<T>, 1, 2, 1> YZY;
			Swizzle<4, T, VectorImpl<T>, 1, 2, 2> YZZ;
			Swizzle<4, T, VectorImpl<T>, 1, 2, 3> YZW;
			Swizzle<4, T, VectorImpl<T>, 1, 3, 0> YWX;
			Swizzle<4, T, VectorImpl<T>, 1, 3, 1> YWY;
			Swizzle<4, T, VectorImpl<T>, 1, 3, 2> YWZ;
			Swizzle<4, T, VectorImpl<T>, 1, 3, 3> YWW;
			Swizzle<4, T, VectorImpl<T>, 2, 0, 0> ZXX;
			Swizzle<4, T, VectorImpl<T>, 2, 0, 1> ZXY;
			Swizzle<4, T, VectorImpl<T>, 2, 0, 2> ZXZ;
			Swizzle<4, T, VectorImpl<T>, 2, 0, 3> ZXW;
			Swizzle<4, T, VectorImpl<T>, 2, 1, 0> ZYX;
			Swizzle<4, T, VectorImpl<T>, 2, 1, 1> ZYY;
			Swizzle<4, T, VectorImpl<T>, 2, 1, 2> ZYZ;
			Swizzle<4, T, VectorImpl<T>, 2, 1, 3> ZYW;
			Swizzle<4, T, VectorImpl<T>, 2, 2, 0> ZZX;
			Swizzle<4, T, VectorImpl<T>, 2, 2, 1> ZZY;
			Swizzle<4, T, VectorImpl<T>, 2, 2, 2> ZZZ;
			Swizzle<4, T, VectorImpl<T>, 2, 2, 3> ZZW;
			Swizzle<4, T, VectorImpl<T>, 2, 3, 0> ZWX;
			Swizzle<4, T, VectorImpl<T>, 2, 3, 1> ZWY;
			Swizzle<4, T, VectorImpl<T>, 2, 3, 2> ZWZ;
			Swizzle<4, T, VectorImpl<T>, 2, 3, 3> ZWW;
			Swizzle<4, T, VectorImpl<T>, 3, 0, 0> WXX;
			Swizzle<4, T, VectorImpl<T>, 3, 0, 1> WXY;
			Swizzle<4, T, VectorImpl<T>, 3, 0, 2> WXZ;
			Swizzle<4, T, VectorImpl<T>, 3, 0, 3> WXW;
			Swizzle<4, T, VectorImpl<T>, 3, 1, 0> WYX;
			Swizzle<4, T, VectorImpl<T>, 3, 1, 1> WYY;
			Swizzle<4, T, VectorImpl<T>, 3, 1, 2> WYZ;
			Swizzle<4, T, VectorImpl<T>, 3, 1, 3> WYW;
			Swizzle<4, T, VectorImpl<T>, 3, 2, 0> WZX;
			Swizzle<4, T, VectorImpl<T>, 3, 2, 1> WZY;
			Swizzle<4, T, VectorImpl<T>, 3, 2, 2> WZZ;
			Swizzle<4, T, VectorImpl<T>, 3, 2, 3> WZW;
			Swizzle<4, T, VectorImpl<T>, 3, 3, 0> WWX;
			Swizzle<4, T, VectorImpl<T>, 3, 3, 1> WWY;
			Swizzle<4, T, VectorImpl<T>, 3, 3, 2> WWZ;
			Swizzle<4, T, VectorImpl<T>, 3, 3, 3> WWW;

			Swizzle<4, T, Vector4Impl, 0, 0, 0, 0> XXXX;
			Swizzle<4, T, Vector4Impl, 0, 0, 0, 1> XXXY;
			Swizzle<4, T, Vector4Impl, 0, 0, 0, 2> XXXZ;
			Swizzle<4, T, Vector4Impl, 0, 0, 0, 3> XXXW;
			Swizzle<4, T, Vector4Impl, 0, 0, 1, 0> XXYX;
			Swizzle<4, T, Vector4Impl, 0, 0, 1, 1> XXYY;
			Swizzle<4, T, Vector4Impl, 0, 0, 1, 2> XXYZ;
			Swizzle<4, T, Vector4Impl, 0, 0, 1, 3> XXYW;
			Swizzle<4, T, Vector4Impl, 0, 0, 2, 0> XXZX;
			Swizzle<4, T, Vector4Impl, 0, 0, 2, 1> XXZY;
			Swizzle<4, T, Vector4Impl, 0, 0, 2, 2> XXZZ;
			Swizzle<4, T, Vector4Impl, 0, 0, 2, 3> XXZW;
			Swizzle<4, T, Vector4Impl, 0, 0, 3, 0> XXWX;
			Swizzle<4, T, Vector4Impl, 0, 0, 3, 1> XXWY;
			Swizzle<4, T, Vector4Impl, 0, 0, 3, 2> XXWZ;
			Swizzle<4, T, Vector4Impl, 0, 0, 3, 3> XXWW;
			Swizzle<4, T, Vector4Impl, 0, 1, 0, 0> XYXX;
			Swizzle<4, T, Vector4Impl, 0, 1, 0, 1> XYXY;
			Swizzle<4, T, Vector4Impl, 0, 1, 0, 2> XYXZ;
			Swizzle<4, T, Vector4Impl, 0, 1, 0, 3> XYXW;
			Swizzle<4, T, Vector4Impl, 0, 1, 1, 0> XYYX;
			Swizzle<4, T, Vector4Impl, 0, 1, 1, 1> XYYY;
			Swizzle<4, T, Vector4Impl, 0, 1, 1, 2> XYYZ;
			Swizzle<4, T, Vector4Impl, 0, 1, 1, 3> XYYW;
			Swizzle<4, T, Vector4Impl, 0, 1, 2, 0> XYZX;
			Swizzle<4, T, Vector4Impl, 0, 1, 2, 1> XYZY;
			Swizzle<4, T, Vector4Impl, 0, 1, 2, 2> XYZZ;
			Swizzle<4, T, Vector4Impl, 0, 1, 2, 3> XYZW;
			Swizzle<4, T, Vector4Impl, 0, 1, 3, 0> XYWX;
			Swizzle<4, T, Vector4Impl, 0, 1, 3, 1> XYWY;
			Swizzle<4, T, Vector4Impl, 0, 1, 3, 2> XYWZ;
			Swizzle<4, T, Vector4Impl, 0, 1, 3, 3> XYWW;
			Swizzle<4, T, Vector4Impl, 0, 2, 0, 0> XZXX;
			Swizzle<4, T, Vector4Impl, 0, 2, 0, 1> XZXY;
			Swizzle<4, T, Vector4Impl, 0, 2, 0, 2> XZXZ;
			Swizzle<4, T, Vector4Impl, 0, 2, 0, 3> XZXW;
			Swizzle<4, T, Vector4Impl, 0, 2, 1, 0> XZYX;
			Swizzle<4, T, Vector4Impl, 0, 2, 1, 1> XZYY;
			Swizzle<4, T, Vector4Impl, 0, 2, 1, 2> XZYZ;
			Swizzle<4, T, Vector4Impl, 0, 2, 1, 3> XZYW;
			Swizzle<4, T, Vector4Impl, 0, 2, 2, 0> XZZX;
			Swizzle<4, T, Vector4Impl, 0, 2, 2, 1> XZZY;
			Swizzle<4, T, Vector4Impl, 0, 2, 2, 2> XZZZ;
			Swizzle<4, T, Vector4Impl, 0, 2, 2, 3> XZZW;
			Swizzle<4, T, Vector4Impl, 0, 2, 3, 0> XZWX;
			Swizzle<4, T, Vector4Impl, 0, 2, 3, 1> XZWY;
			Swizzle<4, T, Vector4Impl, 0, 2, 3, 2> XZWZ;
			Swizzle<4, T, Vector4Impl, 0, 2, 3, 3> XZWW;
			Swizzle<4, T, Vector4Impl, 0, 3, 0, 0> XWXX;
			Swizzle<4, T, Vector4Impl, 0, 3, 0, 1> XWXY;
			Swizzle<4, T, Vector4Impl, 0, 3, 0, 2> XWXZ;
			Swizzle<4, T, Vector4Impl, 0, 3, 0, 3> XWXW;
			Swizzle<4, T, Vector4Impl, 0, 3, 1, 0> XWYX;
			Swizzle<4, T, Vector4Impl, 0, 3, 1, 1> XWYY;
			Swizzle<4, T, Vector4Impl, 0, 3, 1, 2> XWYZ;
			Swizzle<4, T, Vector4Impl, 0, 3, 1, 3> XWYW;
			Swizzle<4, T, Vector4Impl, 0, 3, 2, 0> XWZX;
			Swizzle<4, T, Vector4Impl, 0, 3, 2, 1> XWZY;
			Swizzle<4, T, Vector4Impl, 0, 3, 2, 2> XWZZ;
			Swizzle<4, T, Vector4Impl, 0, 3, 2, 3> XWZW;
			Swizzle<4, T, Vector4Impl, 0, 3, 3, 0> XWWX;
			Swizzle<4, T, Vector4Impl, 0, 3, 3, 1> XWWY;
			Swizzle<4, T, Vector4Impl, 0, 3, 3, 2> XWWZ;
			Swizzle<4, T, Vector4Impl, 0, 3, 3, 3> XWWW;
			Swizzle<4, T, Vector4Impl, 1, 0, 0, 0> YXXX;
			Swizzle<4, T, Vector4Impl, 1, 0, 0, 1> YXXY;
			Swizzle<4, T, Vector4Impl, 1, 0, 0, 2> YXXZ;
			Swizzle<4, T, Vector4Impl, 1, 0, 0, 3> YXXW;
			Swizzle<4, T, Vector4Impl, 1, 0, 1, 0> YXYX;
			Swizzle<4, T, Vector4Impl, 1, 0, 1, 1> YXYY;
			Swizzle<4, T, Vector4Impl, 1, 0, 1, 2> YXYZ;
			Swizzle<4, T, Vector4Impl, 1, 0, 1, 3> YXYW;
			Swizzle<4, T, Vector4Impl, 1, 0, 2, 0> YXZX;
			Swizzle<4, T, Vector4Impl, 1, 0, 2, 1> YXZY;
			Swizzle<4, T, Vector4Impl, 1, 0, 2, 2> YXZZ;
			Swizzle<4, T, Vector4Impl, 1, 0, 2, 3> YXZW;
			Swizzle<4, T, Vector4Impl, 1, 0, 3, 0> YXWX;
			Swizzle<4, T, Vector4Impl, 1, 0, 3, 1> YXWY;
			Swizzle<4, T, Vector4Impl, 1, 0, 3, 2> YXWZ;
			Swizzle<4, T, Vector4Impl, 1, 0, 3, 3> YXWW;
			Swizzle<4, T, Vector4Impl, 1, 1, 0, 0> YYXX;
			Swizzle<4, T, Vector4Impl, 1, 1, 0, 1> YYXY;
			Swizzle<4, T, Vector4Impl, 1, 1, 0, 2> YYXZ;
			Swizzle<4, T, Vector4Impl, 1, 1, 0, 3> YYXW;
			Swizzle<4, T, Vector4Impl, 1, 1, 1, 0> YYYX;
			Swizzle<4, T, Vector4Impl, 1, 1, 1, 1> YYYY;
			Swizzle<4, T, Vector4Impl, 1, 1, 1, 2> YYYZ;
			Swizzle<4, T, Vector4Impl, 1, 1, 1, 3> YYYW;
			Swizzle<4, T, Vector4Impl, 1, 1, 2, 0> YYZX;
			Swizzle<4, T, Vector4Impl, 1, 1, 2, 1> YYZY;
			Swizzle<4, T, Vector4Impl, 1, 1, 2, 2> YYZZ;
			Swizzle<4, T, Vector4Impl, 1, 1, 2, 3> YYZW;
			Swizzle<4, T, Vector4Impl, 1, 1, 3, 0> YYWX;
			Swizzle<4, T, Vector4Impl, 1, 1, 3, 1> YYWY;
			Swizzle<4, T, Vector4Impl, 1, 1, 3, 2> YYWZ;
			Swizzle<4, T, Vector4Impl, 1, 1, 3, 3> YYWW;
			Swizzle<4, T, Vector4Impl, 1, 2, 0, 0> YZXX;
			Swizzle<4, T, Vector4Impl, 1, 2, 0, 1> YZXY;
			Swizzle<4, T, Vector4Impl, 1, 2, 0, 2> YZXZ;
			Swizzle<4, T, Vector4Impl, 1, 2, 0, 3> YZXW;
			Swizzle<4, T, Vector4Impl, 1, 2, 1, 0> YZYX;
			Swizzle<4, T, Vector4Impl, 1, 2, 1, 1> YZYY;
			Swizzle<4, T, Vector4Impl, 1, 2, 1, 2> YZYZ;
			Swizzle<4, T, Vector4Impl, 1, 2, 1, 3> YZYW;
			Swizzle<4, T, Vector4Impl, 1, 2, 2, 0> YZZX;
			Swizzle<4, T, Vector4Impl, 1, 2, 2, 1> YZZY;
			Swizzle<4, T, Vector4Impl, 1, 2, 2, 2> YZZZ;
			Swizzle<4, T, Vector4Impl, 1, 2, 2, 3> YZZW;
			Swizzle<4, T, Vector4Impl, 1, 2, 3, 0> YZWX;
			Swizzle<4, T, Vector4Impl, 1, 2, 3, 1> YZWY;
			Swizzle<4, T, Vector4Impl, 1, 2, 3, 2> YZWZ;
			Swizzle<4, T, Vector4Impl, 1, 2, 3, 3> YZWW;
			Swizzle<4, T, Vector4Impl, 1, 3, 0, 0> YWXX;
			Swizzle<4, T, Vector4Impl, 1, 3, 0, 1> YWXY;
			Swizzle<4, T, Vector4Impl, 1, 3, 0, 2> YWXZ;
			Swizzle<4, T, Vector4Impl, 1, 3, 0, 3> YWXW;
			Swizzle<4, T, Vector4Impl, 1, 3, 1, 0> YWYX;
			Swizzle<4, T, Vector4Impl, 1, 3, 1, 1> YWYY;
			Swizzle<4, T, Vector4Impl, 1, 3, 1, 2> YWYZ;
			Swizzle<4, T, Vector4Impl, 1, 3, 1, 3> YWYW;
			Swizzle<4, T, Vector4Impl, 1, 3, 2, 0> YWZX;
			Swizzle<4, T, Vector4Impl, 1, 3, 2, 1> YWZY;
			Swizzle<4, T, Vector4Impl, 1, 3, 2, 2> YWZZ;
			Swizzle<4, T, Vector4Impl, 1, 3, 2, 3> YWZW;
			Swizzle<4, T, Vector4Impl, 1, 3, 3, 0> YWWX;
			Swizzle<4, T, Vector4Impl, 1, 3, 3, 1> YWWY;
			Swizzle<4, T, Vector4Impl, 1, 3, 3, 2> YWWZ;
			Swizzle<4, T, Vector4Impl, 1, 3, 3, 3> YWWW;
			Swizzle<4, T, Vector4Impl, 2, 0, 0, 0> ZXXX;
			Swizzle<4, T, Vector4Impl, 2, 0, 0, 1> ZXXY;
			Swizzle<4, T, Vector4Impl, 2, 0, 0, 2> ZXXZ;
			Swizzle<4, T, Vector4Impl, 2, 0, 0, 3> ZXXW;
			Swizzle<4, T, Vector4Impl, 2, 0, 1, 0> ZXYX;
			Swizzle<4, T, Vector4Impl, 2, 0, 1, 1> ZXYY;
			Swizzle<4, T, Vector4Impl, 2, 0, 1, 2> ZXYZ;
			Swizzle<4, T, Vector4Impl, 2, 0, 1, 3> ZXYW;
			Swizzle<4, T, Vector4Impl, 2, 0, 2, 0> ZXZX;
			Swizzle<4, T, Vector4Impl, 2, 0, 2, 1> ZXZY;
			Swizzle<4, T, Vector4Impl, 2, 0, 2, 2> ZXZZ;
			Swizzle<4, T, Vector4Impl, 2, 0, 2, 3> ZXZW;
			Swizzle<4, T, Vector4Impl, 2, 0, 3, 0> ZXWX;
			Swizzle<4, T, Vector4Impl, 2, 0, 3, 1> ZXWY;
			Swizzle<4, T, Vector4Impl, 2, 0, 3, 2> ZXWZ;
			Swizzle<4, T, Vector4Impl, 2, 0, 3, 3> ZXWW;
			Swizzle<4, T, Vector4Impl, 2, 1, 0, 0> ZYXX;
			Swizzle<4, T, Vector4Impl, 2, 1, 0, 1> ZYXY;
			Swizzle<4, T, Vector4Impl, 2, 1, 0, 2> ZYXZ;
			Swizzle<4, T, Vector4Impl, 2, 1, 0, 3> ZYXW;
			Swizzle<4, T, Vector4Impl, 2, 1, 1, 0> ZYYX;
			Swizzle<4, T, Vector4Impl, 2, 1, 1, 1> ZYYY;
			Swizzle<4, T, Vector4Impl, 2, 1, 1, 2> ZYYZ;
			Swizzle<4, T, Vector4Impl, 2, 1, 1, 3> ZYYW;
			Swizzle<4, T, Vector4Impl, 2, 1, 2, 0> ZYZX;
			Swizzle<4, T, Vector4Impl, 2, 1, 2, 1> ZYZY;
			Swizzle<4, T, Vector4Impl, 2, 1, 2, 2> ZYZZ;
			Swizzle<4, T, Vector4Impl, 2, 1, 2, 3> ZYZW;
			Swizzle<4, T, Vector4Impl, 2, 1, 3, 0> ZYWX;
			Swizzle<4, T, Vector4Impl, 2, 1, 3, 1> ZYWY;
			Swizzle<4, T, Vector4Impl, 2, 1, 3, 2> ZYWZ;
			Swizzle<4, T, Vector4Impl, 2, 1, 3, 3> ZYWW;
			Swizzle<4, T, Vector4Impl, 2, 2, 0, 0> ZZXX;
			Swizzle<4, T, Vector4Impl, 2, 2, 0, 1> ZZXY;
			Swizzle<4, T, Vector4Impl, 2, 2, 0, 2> ZZXZ;
			Swizzle<4, T, Vector4Impl, 2, 2, 0, 3> ZZXW;
			Swizzle<4, T, Vector4Impl, 2, 2, 1, 0> ZZYX;
			Swizzle<4, T, Vector4Impl, 2, 2, 1, 1> ZZYY;
			Swizzle<4, T, Vector4Impl, 2, 2, 1, 2> ZZYZ;
			Swizzle<4, T, Vector4Impl, 2, 2, 1, 3> ZZYW;
			Swizzle<4, T, Vector4Impl, 2, 2, 2, 0> ZZZX;
			Swizzle<4, T, Vector4Impl, 2, 2, 2, 1> ZZZY;
			Swizzle<4, T, Vector4Impl, 2, 2, 2, 2> ZZZZ;
			Swizzle<4, T, Vector4Impl, 2, 2, 2, 3> ZZZW;
			Swizzle<4, T, Vector4Impl, 2, 2, 3, 0> ZZWX;
			Swizzle<4, T, Vector4Impl, 2, 2, 3, 1> ZZWY;
			Swizzle<4, T, Vector4Impl, 2, 2, 3, 2> ZZWZ;
			Swizzle<4, T, Vector4Impl, 2, 2, 3, 3> ZZWW;
			Swizzle<4, T, Vector4Impl, 2, 3, 0, 0> ZWXX;
			Swizzle<4, T, Vector4Impl, 2, 3, 0, 1> ZWXY;
			Swizzle<4, T, Vector4Impl, 2, 3, 0, 2> ZWXZ;
			Swizzle<4, T, Vector4Impl, 2, 3, 0, 3> ZWXW;
			Swizzle<4, T, Vector4Impl, 2, 3, 1, 0> ZWYX;
			Swizzle<4, T, Vector4Impl, 2, 3, 1, 1> ZWYY;
			Swizzle<4, T, Vector4Impl, 2, 3, 1, 2> ZWYZ;
			Swizzle<4, T, Vector4Impl, 2, 3, 1, 3> ZWYW;
			Swizzle<4, T, Vector4Impl, 2, 3, 2, 0> ZWZX;
			Swizzle<4, T, Vector4Impl, 2, 3, 2, 1> ZWZY;
			Swizzle<4, T, Vector4Impl, 2, 3, 2, 2> ZWZZ;
			Swizzle<4, T, Vector4Impl, 2, 3, 2, 3> ZWZW;
			Swizzle<4, T, Vector4Impl, 2, 3, 3, 0> ZWWX;
			Swizzle<4, T, Vector4Impl, 2, 3, 3, 1> ZWWY;
			Swizzle<4, T, Vector4Impl, 2, 3, 3, 2> ZWWZ;
			Swizzle<4, T, Vector4Impl, 2, 3, 3, 3> ZWWW;
			Swizzle<4, T, Vector4Impl, 3, 0, 0, 0> WXXX;
			Swizzle<4, T, Vector4Impl, 3, 0, 0, 1> WXXY;
			Swizzle<4, T, Vector4Impl, 3, 0, 0, 2> WXXZ;
			Swizzle<4, T, Vector4Impl, 3, 0, 0, 3> WXXW;
			Swizzle<4, T, Vector4Impl, 3, 0, 1, 0> WXYX;
			Swizzle<4, T, Vector4Impl, 3, 0, 1, 1> WXYY;
			Swizzle<4, T, Vector4Impl, 3, 0, 1, 2> WXYZ;
			Swizzle<4, T, Vector4Impl, 3, 0, 1, 3> WXYW;
			Swizzle<4, T, Vector4Impl, 3, 0, 2, 0> WXZX;
			Swizzle<4, T, Vector4Impl, 3, 0, 2, 1> WXZY;
			Swizzle<4, T, Vector4Impl, 3, 0, 2, 2> WXZZ;
			Swizzle<4, T, Vector4Impl, 3, 0, 2, 3> WXZW;
			Swizzle<4, T, Vector4Impl, 3, 0, 3, 0> WXWX;
			Swizzle<4, T, Vector4Impl, 3, 0, 3, 1> WXWY;
			Swizzle<4, T, Vector4Impl, 3, 0, 3, 2> WXWZ;
			Swizzle<4, T, Vector4Impl, 3, 0, 3, 3> WXWW;
			Swizzle<4, T, Vector4Impl, 3, 1, 0, 0> WYXX;
			Swizzle<4, T, Vector4Impl, 3, 1, 0, 1> WYXY;
			Swizzle<4, T, Vector4Impl, 3, 1, 0, 2> WYXZ;
			Swizzle<4, T, Vector4Impl, 3, 1, 0, 3> WYXW;
			Swizzle<4, T, Vector4Impl, 3, 1, 1, 0> WYYX;
			Swizzle<4, T, Vector4Impl, 3, 1, 1, 1> WYYY;
			Swizzle<4, T, Vector4Impl, 3, 1, 1, 2> WYYZ;
			Swizzle<4, T, Vector4Impl, 3, 1, 1, 3> WYYW;
			Swizzle<4, T, Vector4Impl, 3, 1, 2, 0> WYZX;
			Swizzle<4, T, Vector4Impl, 3, 1, 2, 1> WYZY;
			Swizzle<4, T, Vector4Impl, 3, 1, 2, 2> WYZZ;
			Swizzle<4, T, Vector4Impl, 3, 1, 2, 3> WYZW;
			Swizzle<4, T, Vector4Impl, 3, 1, 3, 0> WYWX;
			Swizzle<4, T, Vector4Impl, 3, 1, 3, 1> WYWY;
			Swizzle<4, T, Vector4Impl, 3, 1, 3, 2> WYWZ;
			Swizzle<4, T, Vector4Impl, 3, 1, 3, 3> WYWW;
			Swizzle<4, T, Vector4Impl, 3, 2, 0, 0> WZXX;
			Swizzle<4, T, Vector4Impl, 3, 2, 0, 1> WZXY;
			Swizzle<4, T, Vector4Impl, 3, 2, 0, 2> WZXZ;
			Swizzle<4, T, Vector4Impl, 3, 2, 0, 3> WZXW;
			Swizzle<4, T, Vector4Impl, 3, 2, 1, 0> WZYX;
			Swizzle<4, T, Vector4Impl, 3, 2, 1, 1> WZYY;
			Swizzle<4, T, Vector4Impl, 3, 2, 1, 2> WZYZ;
			Swizzle<4, T, Vector4Impl, 3, 2, 1, 3> WZYW;
			Swizzle<4, T, Vector4Impl, 3, 2, 2, 0> WZZX;
			Swizzle<4, T, Vector4Impl, 3, 2, 2, 1> WZZY;
			Swizzle<4, T, Vector4Impl, 3, 2, 2, 2> WZZZ;
			Swizzle<4, T, Vector4Impl, 3, 2, 2, 3> WZZW;
			Swizzle<4, T, Vector4Impl, 3, 2, 3, 0> WZWX;
			Swizzle<4, T, Vector4Impl, 3, 2, 3, 1> WZWY;
			Swizzle<4, T, Vector4Impl, 3, 2, 3, 2> WZWZ;
			Swizzle<4, T, Vector4Impl, 3, 2, 3, 3> WZWW;
			Swizzle<4, T, Vector4Impl, 3, 3, 0, 0> WWXX;
			Swizzle<4, T, Vector4Impl, 3, 3, 0, 1> WWXY;
			Swizzle<4, T, Vector4Impl, 3, 3, 0, 2> WWXZ;
			Swizzle<4, T, Vector4Impl, 3, 3, 0, 3> WWXW;
			Swizzle<4, T, Vector4Impl, 3, 3, 1, 0> WWYX;
			Swizzle<4, T, Vector4Impl, 3, 3, 1, 1> WWYY;
			Swizzle<4, T, Vector4Impl, 3, 3, 1, 2> WWYZ;
			Swizzle<4, T, Vector4Impl, 3, 3, 1, 3> WWYW;
			Swizzle<4, T, Vector4Impl, 3, 3, 2, 0> WWZX;
			Swizzle<4, T, Vector4Impl, 3, 3, 2, 1> WWZY;
			Swizzle<4, T, Vector4Impl, 3, 3, 2, 2> WWZZ;
			Swizzle<4, T, Vector4Impl, 3, 3, 2, 3> WWZW;
			Swizzle<4, T, Vector4Impl, 3, 3, 3, 0> WWWX;
			Swizzle<4, T, Vector4Impl, 3, 3, 3, 1> WWWY;
			Swizzle<4, T, Vector4Impl, 3, 3, 3, 2> WWWZ;
			Swizzle<4, T, Vector4Impl, 3, 3, 3, 3> WWWW;
		};

		Vector4Impl() = default;
		Vector4Impl(const UE_Vector& Vec) : X(Vec.X), Y(Vec.Y), Z(Vec.Z), W(Vec.W) {}
		Vector4Impl(T IntT) : X(IntT), Y(IntT), Z(IntT), W(IntT) {}
		Vector4Impl(T IntX, T IntY, T IntZ, T IntW) : X(IntX), Y(IntY), Z(IntZ), W(IntW) {}
		
		Vector4Impl operator*(const Vector4Impl& rhs) const {
			return Vector4Impl(X * rhs.X, Y * rhs.Y, Z * rhs.Z, W * rhs.W);
		}
		Vector4Impl operator+(const Vector4Impl& rhs) const {
			return Vector4Impl(X + rhs.X, Y + rhs.Y, Z + rhs.Z, W + rhs.W);
		}
		Vector4Impl operator-(const Vector4Impl& rhs) const {
			return Vector4Impl(X - rhs.X, Y - rhs.Y, Z - rhs.Z, W - rhs.W);
		}
		Vector4Impl operator/(const Vector4Impl& rhs) const {
			return Vector4Impl(X / rhs.X, Y / rhs.Y, Z / rhs.Z, W / rhs.W);
		}

		Vector4Impl& operator*=(const Vector4Impl& rhs) {
			X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z; W *= rhs.W;
			return *this;
		}
		Vector4Impl& operator+=(const Vector4Impl& rhs) {
			X += rhs.X; Y += rhs.Y; Z += rhs.Z; W += rhs.W;
			return *this;
		}
		Vector4Impl& operator-=(const Vector4Impl& rhs) {
			X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z; W -= rhs.W;
			return *this;
		}
		Vector4Impl& operator/=(const Vector4Impl& rhs) {
			X /= rhs.X; Y /= rhs.Y; Z /= rhs.Z; W /= rhs.W;
			return *this;
		}

		bool operator<(const Vector4Impl& rhs) const {
			return X < rhs.X&& Y < rhs.Y&& Z < rhs.Z&& W < rhs.W;
		}
		bool operator>(const Vector4Impl& rhs) const {
			return X > rhs.X && Y > rhs.Y && Z > rhs.Z && W > rhs.W;
		}
		bool operator<=(const Vector4Impl& rhs) const {
			return X <= rhs.X && Y <= rhs.Y && Z <= rhs.Z && W <= rhs.W;
		}
		bool operator>=(const Vector4Impl& rhs) const {
			return X >= rhs.X && Y >= rhs.Y && Z >= rhs.Z && W >= rhs.W;
		}
		//bool operator==(const Vector4Impl& rhs) const {
		//	return X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W;
		//}
		//bool operator!=(const Vector4Impl& rhs) const {
		//	return X != rhs.X || Y != rhs.Y || Z != rhs.Z || W != rhs.W;
		//}

		bool Equals(const Vector4Impl& rhs, T eps = std::numeric_limits<T>::epsilon()) const {
			return FMath::Abs(X - rhs.X) < eps && FMath::Abs(Y - rhs.Y) < eps && FMath::Abs(Z - rhs.Z) < eps && FMath::Abs(W - rhs.W) < eps;
		}

		Vector4Impl operator*(T rhs) const {
			return Vector4Impl(X * rhs, Y * rhs, Z * rhs, W * rhs);
		}
		Vector4Impl operator+(T rhs) const {
			return Vector4Impl(X + rhs, Y + rhs, Z + rhs, W + rhs);
		}
		Vector4Impl operator-(T rhs) const {
			return Vector4Impl(X - rhs, Y - rhs, Z - rhs, W - rhs);
		}
		Vector4Impl operator/(T rhs) const {
			return Vector4Impl(X / rhs, Y / rhs, Z / rhs, W / rhs);
		}

		const T& operator[](uint32 Index) const {
			checkf(Index >= 0 && Index < 4, TEXT("Invalid Index"));
			return Data[Index];
		}

		T& operator[](uint32 Index) {
			checkf(Index >= 0 && Index < 4, TEXT("Invalid Index"));
			return Data[Index];
		}

		operator UE_Vector() {
			return UE_Vector(X, Y, Z, W);
		}

		FString ToString() const {
			return FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f W=%3.3f"), X, Y, Z, W);
		}

		const T* GetData() const {
			return Data;
		}
	};

}
