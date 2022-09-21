#pragma once
#include "CommonHeader.h"
#include "Auxiliary.h"
namespace SH {

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

	template<typename Base, typename Target, uint32... Indexes>
	struct Swizzle
	{
		template <typename Base, typename Target, uint32 ... OtherIndexes>
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

		template<uint32 ... OtherIndexes>
		Target operator+(const Swizzle<Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Sum<Target>(Data, rhs.Data);
		}
		template<uint32 ... OtherIndexes>
		Target operator*(const Swizzle<Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Mul<Target>(Data, rhs.Data);
		}
		template<uint32 ... OtherIndexes>
		Target operator-(const Swizzle<Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Sub<Target>(Data, rhs.Data);
		}
		template<uint32 ... OtherIndexes>
		Target operator/(const Swizzle<Base, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>:: template Div<Target>(Data, rhs.Data);
		}

	private:
		Base Data[sizeof...(Indexes)];
	};

	template<typename T>
	struct Vector2Impl
	{
		using UE_Vector = UE::Math::TVector2<T>;
		union
		{
			struct
			{
				T X, Y;
			};
			Swizzle<T, Vector2Impl, 0, 0> XX;
			Swizzle<T, Vector2Impl, 0, 1> XY;
			Swizzle<T, Vector2Impl, 1, 0> YX;
			Swizzle<T, Vector2Impl, 1, 1> YY;

			Swizzle<T, VectorImpl<T>, 0, 0, 0> XXX;
			Swizzle<T, VectorImpl<T>, 0, 0, 1> XXY;
			Swizzle<T, VectorImpl<T>, 0, 1, 0> XYX;
			Swizzle<T, VectorImpl<T>, 0, 1, 1> XYY;
			Swizzle<T, VectorImpl<T>, 1, 0, 0> YXX;
			Swizzle<T, VectorImpl<T>, 1, 0, 1> YXY;
			Swizzle<T, VectorImpl<T>, 1, 1, 0> YYX;
			Swizzle<T, VectorImpl<T>, 1, 1, 1> YYY;

			Swizzle<T, Vector4Impl<T>, 0, 0, 0, 0> XXXX;
			Swizzle<T, Vector4Impl<T>, 0, 0, 0, 1> XXXY;
			Swizzle<T, Vector4Impl<T>, 0, 0, 1, 0> XXYX;
			Swizzle<T, Vector4Impl<T>, 0, 0, 1, 1> XXYY;
			Swizzle<T, Vector4Impl<T>, 0, 1, 0, 0> XYXX;
			Swizzle<T, Vector4Impl<T>, 0, 1, 0, 1> XYXY;
			Swizzle<T, Vector4Impl<T>, 0, 1, 1, 0> XYYX;
			Swizzle<T, Vector4Impl<T>, 0, 1, 1, 1> XYYY;
			Swizzle<T, Vector4Impl<T>, 1, 0, 0, 0> YXXX;
			Swizzle<T, Vector4Impl<T>, 1, 0, 0, 1> YXXY;
			Swizzle<T, Vector4Impl<T>, 1, 0, 1, 0> YXYX;
			Swizzle<T, Vector4Impl<T>, 1, 0, 1, 1> YXYY;
			Swizzle<T, Vector4Impl<T>, 1, 1, 0, 0> YYXX;
			Swizzle<T, Vector4Impl<T>, 1, 1, 0, 1> YYXY;
			Swizzle<T, Vector4Impl<T>, 1, 1, 1, 0> YYYX;
			Swizzle<T, Vector4Impl<T>, 1, 1, 1, 1> YYYY;
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

		Vector2Impl operator*=(const Vector2Impl& rhs) {
			X *= rhs.X; Y *= rhs.Y;
			return *this;
		}
		Vector2Impl operator+=(const Vector2Impl& rhs) {
			X += rhs.X; Y += rhs.Y;
			return *this;
		}
		Vector2Impl operator-=(const Vector2Impl& rhs) {
			X -= rhs.X; Y -= rhs.Y;
			return *this;
		}
		Vector2Impl operator/=(const Vector2Impl& rhs) {
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
		bool operator==(const Vector2Impl& rhs) const {
			return X == rhs.X && Y == rhs.Y;
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

		T operator[](uint32 Index) const {
			return *(reinterpret_cast<const T*>(this) + Index);
		}

		operator UE_Vector() {
			return UE_Vector(X, Y);
		}
	};

	template<typename T>
	struct VectorImpl
	{
		using UE_Vector = UE::Math::TVector<T>;
		union
		{
			struct
			{
				T X, Y, Z;
			};
			Swizzle<T, Vector2Impl<T>, 0, 0> XX;
			Swizzle<T, Vector2Impl<T>, 0, 1> XY;
			Swizzle<T, Vector2Impl<T>, 1, 0> YX;
			Swizzle<T, Vector2Impl<T>, 1, 1> YY;
			Swizzle<T, Vector2Impl<T>, 0, 2> XZ;
			Swizzle<T, Vector2Impl<T>, 1, 2> YZ;
			Swizzle<T, Vector2Impl<T>, 2, 0> ZX;
			Swizzle<T, Vector2Impl<T>, 2, 1> ZY;
			Swizzle<T, Vector2Impl<T>, 2, 2> ZZ;

			Swizzle<T, VectorImpl, 0, 0, 0> XXX;
			Swizzle<T, VectorImpl, 0, 0, 1> XXY;
			Swizzle<T, VectorImpl, 0, 0, 2> XXZ;
			Swizzle<T, VectorImpl, 0, 1, 0> XYX;
			Swizzle<T, VectorImpl, 0, 1, 1> XYY;
			Swizzle<T, VectorImpl, 0, 1, 2> XYZ;
			Swizzle<T, VectorImpl, 0, 2, 0> XZX;
			Swizzle<T, VectorImpl, 0, 2, 1> XZY;
			Swizzle<T, VectorImpl, 0, 2, 2> XZZ;
			Swizzle<T, VectorImpl, 1, 0, 0> YXX;
			Swizzle<T, VectorImpl, 1, 0, 1> YXY;
			Swizzle<T, VectorImpl, 1, 0, 2> YXZ;
			Swizzle<T, VectorImpl, 1, 1, 0> YYX;
			Swizzle<T, VectorImpl, 1, 1, 1> YYY;
			Swizzle<T, VectorImpl, 1, 1, 2> YYZ;
			Swizzle<T, VectorImpl, 1, 2, 0> YZX;
			Swizzle<T, VectorImpl, 1, 2, 1> YZY;
			Swizzle<T, VectorImpl, 1, 2, 2> YZZ;
			Swizzle<T, VectorImpl, 2, 0, 0> ZXX;
			Swizzle<T, VectorImpl, 2, 0, 1> ZXY;
			Swizzle<T, VectorImpl, 2, 0, 2> ZXZ;
			Swizzle<T, VectorImpl, 2, 1, 0> ZYX;
			Swizzle<T, VectorImpl, 2, 1, 1> ZYY;
			Swizzle<T, VectorImpl, 2, 1, 2> ZYZ;
			Swizzle<T, VectorImpl, 2, 2, 0> ZZX;
			Swizzle<T, VectorImpl, 2, 2, 1> ZZY;
			Swizzle<T, VectorImpl, 2, 2, 2> ZZZ;

			Swizzle<T, Vector4Impl<T>, 0, 0, 0, 0> XXXX;
			Swizzle<T, Vector4Impl<T>, 0, 0, 0, 1> XXXY;
			Swizzle<T, Vector4Impl<T>, 0, 0, 0, 2> XXXZ;
			Swizzle<T, Vector4Impl<T>, 0, 0, 1, 0> XXYX;
			Swizzle<T, Vector4Impl<T>, 0, 0, 1, 1> XXYY;
			Swizzle<T, Vector4Impl<T>, 0, 0, 1, 2> XXYZ;
			Swizzle<T, Vector4Impl<T>, 0, 0, 2, 0> XXZX;
			Swizzle<T, Vector4Impl<T>, 0, 0, 2, 1> XXZY;
			Swizzle<T, Vector4Impl<T>, 0, 0, 2, 2> XXZZ;
			Swizzle<T, Vector4Impl<T>, 0, 1, 0, 0> XYXX;
			Swizzle<T, Vector4Impl<T>, 0, 1, 0, 1> XYXY;
			Swizzle<T, Vector4Impl<T>, 0, 1, 0, 2> XYXZ;
			Swizzle<T, Vector4Impl<T>, 0, 1, 1, 0> XYYX;
			Swizzle<T, Vector4Impl<T>, 0, 1, 1, 1> XYYY;
			Swizzle<T, Vector4Impl<T>, 0, 1, 1, 2> XYYZ;
			Swizzle<T, Vector4Impl<T>, 0, 1, 2, 0> XYZX;
			Swizzle<T, Vector4Impl<T>, 0, 1, 2, 1> XYZY;
			Swizzle<T, Vector4Impl<T>, 0, 1, 2, 2> XYZZ;
			Swizzle<T, Vector4Impl<T>, 0, 2, 0, 0> XZXX;
			Swizzle<T, Vector4Impl<T>, 0, 2, 0, 1> XZXY;
			Swizzle<T, Vector4Impl<T>, 0, 2, 0, 2> XZXZ;
			Swizzle<T, Vector4Impl<T>, 0, 2, 1, 0> XZYX;
			Swizzle<T, Vector4Impl<T>, 0, 2, 1, 1> XZYY;
			Swizzle<T, Vector4Impl<T>, 0, 2, 1, 2> XZYZ;
			Swizzle<T, Vector4Impl<T>, 0, 2, 2, 0> XZZX;
			Swizzle<T, Vector4Impl<T>, 0, 2, 2, 1> XZZY;
			Swizzle<T, Vector4Impl<T>, 0, 2, 2, 2> XZZZ;
			Swizzle<T, Vector4Impl<T>, 1, 0, 0, 0> YXXX;
			Swizzle<T, Vector4Impl<T>, 1, 0, 0, 1> YXXY;
			Swizzle<T, Vector4Impl<T>, 1, 0, 0, 2> YXXZ;
			Swizzle<T, Vector4Impl<T>, 1, 0, 1, 0> YXYX;
			Swizzle<T, Vector4Impl<T>, 1, 0, 1, 1> YXYY;
			Swizzle<T, Vector4Impl<T>, 1, 0, 1, 2> YXYZ;
			Swizzle<T, Vector4Impl<T>, 1, 0, 2, 0> YXZX;
			Swizzle<T, Vector4Impl<T>, 1, 0, 2, 1> YXZY;
			Swizzle<T, Vector4Impl<T>, 1, 0, 2, 2> YXZZ;
			Swizzle<T, Vector4Impl<T>, 1, 1, 0, 0> YYXX;
			Swizzle<T, Vector4Impl<T>, 1, 1, 0, 1> YYXY;
			Swizzle<T, Vector4Impl<T>, 1, 1, 0, 2> YYXZ;
			Swizzle<T, Vector4Impl<T>, 1, 1, 1, 0> YYYX;
			Swizzle<T, Vector4Impl<T>, 1, 1, 1, 1> YYYY;
			Swizzle<T, Vector4Impl<T>, 1, 1, 1, 2> YYYZ;
			Swizzle<T, Vector4Impl<T>, 1, 1, 2, 0> YYZX;
			Swizzle<T, Vector4Impl<T>, 1, 1, 2, 1> YYZY;
			Swizzle<T, Vector4Impl<T>, 1, 1, 2, 2> YYZZ;
			Swizzle<T, Vector4Impl<T>, 1, 2, 0, 0> YZXX;
			Swizzle<T, Vector4Impl<T>, 1, 2, 0, 1> YZXY;
			Swizzle<T, Vector4Impl<T>, 1, 2, 0, 2> YZXZ;
			Swizzle<T, Vector4Impl<T>, 1, 2, 1, 0> YZYX;
			Swizzle<T, Vector4Impl<T>, 1, 2, 1, 1> YZYY;
			Swizzle<T, Vector4Impl<T>, 1, 2, 1, 2> YZYZ;
			Swizzle<T, Vector4Impl<T>, 1, 2, 2, 0> YZZX;
			Swizzle<T, Vector4Impl<T>, 1, 2, 2, 1> YZZY;
			Swizzle<T, Vector4Impl<T>, 1, 2, 2, 2> YZZZ;
			Swizzle<T, Vector4Impl<T>, 2, 0, 0, 0> ZXXX;
			Swizzle<T, Vector4Impl<T>, 2, 0, 0, 1> ZXXY;
			Swizzle<T, Vector4Impl<T>, 2, 0, 0, 2> ZXXZ;
			Swizzle<T, Vector4Impl<T>, 2, 0, 1, 0> ZXYX;
			Swizzle<T, Vector4Impl<T>, 2, 0, 1, 1> ZXYY;
			Swizzle<T, Vector4Impl<T>, 2, 0, 1, 2> ZXYZ;
			Swizzle<T, Vector4Impl<T>, 2, 0, 2, 0> ZXZX;
			Swizzle<T, Vector4Impl<T>, 2, 0, 2, 1> ZXZY;
			Swizzle<T, Vector4Impl<T>, 2, 0, 2, 2> ZXZZ;
			Swizzle<T, Vector4Impl<T>, 2, 1, 0, 0> ZYXX;
			Swizzle<T, Vector4Impl<T>, 2, 1, 0, 1> ZYXY;
			Swizzle<T, Vector4Impl<T>, 2, 1, 0, 2> ZYXZ;
			Swizzle<T, Vector4Impl<T>, 2, 1, 1, 0> ZYYX;
			Swizzle<T, Vector4Impl<T>, 2, 1, 1, 1> ZYYY;
			Swizzle<T, Vector4Impl<T>, 2, 1, 1, 2> ZYYZ;
			Swizzle<T, Vector4Impl<T>, 2, 1, 2, 0> ZYZX;
			Swizzle<T, Vector4Impl<T>, 2, 1, 2, 1> ZYZY;
			Swizzle<T, Vector4Impl<T>, 2, 1, 2, 2> ZYZZ;
			Swizzle<T, Vector4Impl<T>, 2, 2, 0, 0> ZZXX;
			Swizzle<T, Vector4Impl<T>, 2, 2, 0, 1> ZZXY;
			Swizzle<T, Vector4Impl<T>, 2, 2, 0, 2> ZZXZ;
			Swizzle<T, Vector4Impl<T>, 2, 2, 1, 0> ZZYX;
			Swizzle<T, Vector4Impl<T>, 2, 2, 1, 1> ZZYY;
			Swizzle<T, Vector4Impl<T>, 2, 2, 1, 2> ZZYZ;
			Swizzle<T, Vector4Impl<T>, 2, 2, 2, 0> ZZZX;
			Swizzle<T, Vector4Impl<T>, 2, 2, 2, 1> ZZZY;
			Swizzle<T, Vector4Impl<T>, 2, 2, 2, 2> ZZZZ;
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

		VectorImpl operator*=(const VectorImpl& rhs) {
			X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z;
			return *this;
		}
		VectorImpl operator+=(const VectorImpl& rhs) {
			X += rhs.X; Y += rhs.Y; Z += rhs.Z;
			return *this;
		}
		VectorImpl operator-=(const VectorImpl& rhs) {
			X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z;
			return *this;
		}
		VectorImpl operator/=(const VectorImpl& rhs) {
			X /= rhs.X; Y /= rhs.Y; Z /= rhs.Z;
			return *this;
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
		bool operator==(const VectorImpl& rhs) const {
			return X == rhs.X && Y == rhs.Y && Z == rhs.Z;
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

		T operator[](uint32 Index) const {
			return *(reinterpret_cast<const T*>(this) + Index);
		}

		operator UE_Vector() {
			return UE_Vector(X, Y, Z);
		}
	};

	template<typename T>
	struct Vector4Impl
	{
		using UE_Vector = UE::Math::TVector4<T>;
		union
		{
			struct
			{
				T X, Y, Z, W;
			};
			Swizzle<T, Vector2Impl<T>, 0, 0> XX;
			Swizzle<T, Vector2Impl<T>, 0, 1> XY;
			Swizzle<T, Vector2Impl<T>, 0, 2> XZ;
			Swizzle<T, Vector2Impl<T>, 0, 3> XW;
			Swizzle<T, Vector2Impl<T>, 1, 0> YX;
			Swizzle<T, Vector2Impl<T>, 1, 1> YY;
			Swizzle<T, Vector2Impl<T>, 1, 2> YZ;
			Swizzle<T, Vector2Impl<T>, 1, 3> YW;
			Swizzle<T, Vector2Impl<T>, 2, 0> ZX;
			Swizzle<T, Vector2Impl<T>, 2, 1> ZY;
			Swizzle<T, Vector2Impl<T>, 2, 2> ZZ;
			Swizzle<T, Vector2Impl<T>, 2, 3> ZW;
			Swizzle<T, Vector2Impl<T>, 3, 0> WX;
			Swizzle<T, Vector2Impl<T>, 3, 1> WY;
			Swizzle<T, Vector2Impl<T>, 3, 2> WZ;
			Swizzle<T, Vector2Impl<T>, 3, 3> WW;

			Swizzle<T, VectorImpl<T>, 0, 0, 0> XXX;
			Swizzle<T, VectorImpl<T>, 0, 0, 1> XXY;
			Swizzle<T, VectorImpl<T>, 0, 0, 2> XXZ;
			Swizzle<T, VectorImpl<T>, 0, 0, 3> XXW;
			Swizzle<T, VectorImpl<T>, 0, 1, 0> XYX;
			Swizzle<T, VectorImpl<T>, 0, 1, 1> XYY;
			Swizzle<T, VectorImpl<T>, 0, 1, 2> XYZ;
			Swizzle<T, VectorImpl<T>, 0, 1, 3> XYW;
			Swizzle<T, VectorImpl<T>, 0, 2, 0> XZX;
			Swizzle<T, VectorImpl<T>, 0, 2, 1> XZY;
			Swizzle<T, VectorImpl<T>, 0, 2, 2> XZZ;
			Swizzle<T, VectorImpl<T>, 0, 2, 3> XZW;
			Swizzle<T, VectorImpl<T>, 0, 3, 0> XWX;
			Swizzle<T, VectorImpl<T>, 0, 3, 1> XWY;
			Swizzle<T, VectorImpl<T>, 0, 3, 2> XWZ;
			Swizzle<T, VectorImpl<T>, 0, 3, 3> XWW;
			Swizzle<T, VectorImpl<T>, 1, 0, 0> YXX;
			Swizzle<T, VectorImpl<T>, 1, 0, 1> YXY;
			Swizzle<T, VectorImpl<T>, 1, 0, 2> YXZ;
			Swizzle<T, VectorImpl<T>, 1, 0, 3> YXW;
			Swizzle<T, VectorImpl<T>, 1, 1, 0> YYX;
			Swizzle<T, VectorImpl<T>, 1, 1, 1> YYY;
			Swizzle<T, VectorImpl<T>, 1, 1, 2> YYZ;
			Swizzle<T, VectorImpl<T>, 1, 1, 3> YYW;
			Swizzle<T, VectorImpl<T>, 1, 2, 0> YZX;
			Swizzle<T, VectorImpl<T>, 1, 2, 1> YZY;
			Swizzle<T, VectorImpl<T>, 1, 2, 2> YZZ;
			Swizzle<T, VectorImpl<T>, 1, 2, 3> YZW;
			Swizzle<T, VectorImpl<T>, 1, 3, 0> YWX;
			Swizzle<T, VectorImpl<T>, 1, 3, 1> YWY;
			Swizzle<T, VectorImpl<T>, 1, 3, 2> YWZ;
			Swizzle<T, VectorImpl<T>, 1, 3, 3> YWW;
			Swizzle<T, VectorImpl<T>, 2, 0, 0> ZXX;
			Swizzle<T, VectorImpl<T>, 2, 0, 1> ZXY;
			Swizzle<T, VectorImpl<T>, 2, 0, 2> ZXZ;
			Swizzle<T, VectorImpl<T>, 2, 0, 3> ZXW;
			Swizzle<T, VectorImpl<T>, 2, 1, 0> ZYX;
			Swizzle<T, VectorImpl<T>, 2, 1, 1> ZYY;
			Swizzle<T, VectorImpl<T>, 2, 1, 2> ZYZ;
			Swizzle<T, VectorImpl<T>, 2, 1, 3> ZYW;
			Swizzle<T, VectorImpl<T>, 2, 2, 0> ZZX;
			Swizzle<T, VectorImpl<T>, 2, 2, 1> ZZY;
			Swizzle<T, VectorImpl<T>, 2, 2, 2> ZZZ;
			Swizzle<T, VectorImpl<T>, 2, 2, 3> ZZW;
			Swizzle<T, VectorImpl<T>, 2, 3, 0> ZWX;
			Swizzle<T, VectorImpl<T>, 2, 3, 1> ZWY;
			Swizzle<T, VectorImpl<T>, 2, 3, 2> ZWZ;
			Swizzle<T, VectorImpl<T>, 2, 3, 3> ZWW;
			Swizzle<T, VectorImpl<T>, 3, 0, 0> WXX;
			Swizzle<T, VectorImpl<T>, 3, 0, 1> WXY;
			Swizzle<T, VectorImpl<T>, 3, 0, 2> WXZ;
			Swizzle<T, VectorImpl<T>, 3, 0, 3> WXW;
			Swizzle<T, VectorImpl<T>, 3, 1, 0> WYX;
			Swizzle<T, VectorImpl<T>, 3, 1, 1> WYY;
			Swizzle<T, VectorImpl<T>, 3, 1, 2> WYZ;
			Swizzle<T, VectorImpl<T>, 3, 1, 3> WYW;
			Swizzle<T, VectorImpl<T>, 3, 2, 0> WZX;
			Swizzle<T, VectorImpl<T>, 3, 2, 1> WZY;
			Swizzle<T, VectorImpl<T>, 3, 2, 2> WZZ;
			Swizzle<T, VectorImpl<T>, 3, 2, 3> WZW;
			Swizzle<T, VectorImpl<T>, 3, 3, 0> WWX;
			Swizzle<T, VectorImpl<T>, 3, 3, 1> WWY;
			Swizzle<T, VectorImpl<T>, 3, 3, 2> WWZ;
			Swizzle<T, VectorImpl<T>, 3, 3, 3> WWW;

			Swizzle<T, Vector4Impl, 0, 0, 0, 0> XXXX;
			Swizzle<T, Vector4Impl, 0, 0, 0, 1> XXXY;
			Swizzle<T, Vector4Impl, 0, 0, 0, 2> XXXZ;
			Swizzle<T, Vector4Impl, 0, 0, 0, 3> XXXW;
			Swizzle<T, Vector4Impl, 0, 0, 1, 0> XXYX;
			Swizzle<T, Vector4Impl, 0, 0, 1, 1> XXYY;
			Swizzle<T, Vector4Impl, 0, 0, 1, 2> XXYZ;
			Swizzle<T, Vector4Impl, 0, 0, 1, 3> XXYW;
			Swizzle<T, Vector4Impl, 0, 0, 2, 0> XXZX;
			Swizzle<T, Vector4Impl, 0, 0, 2, 1> XXZY;
			Swizzle<T, Vector4Impl, 0, 0, 2, 2> XXZZ;
			Swizzle<T, Vector4Impl, 0, 0, 2, 3> XXZW;
			Swizzle<T, Vector4Impl, 0, 0, 3, 0> XXWX;
			Swizzle<T, Vector4Impl, 0, 0, 3, 1> XXWY;
			Swizzle<T, Vector4Impl, 0, 0, 3, 2> XXWZ;
			Swizzle<T, Vector4Impl, 0, 0, 3, 3> XXWW;
			Swizzle<T, Vector4Impl, 0, 1, 0, 0> XYXX;
			Swizzle<T, Vector4Impl, 0, 1, 0, 1> XYXY;
			Swizzle<T, Vector4Impl, 0, 1, 0, 2> XYXZ;
			Swizzle<T, Vector4Impl, 0, 1, 0, 3> XYXW;
			Swizzle<T, Vector4Impl, 0, 1, 1, 0> XYYX;
			Swizzle<T, Vector4Impl, 0, 1, 1, 1> XYYY;
			Swizzle<T, Vector4Impl, 0, 1, 1, 2> XYYZ;
			Swizzle<T, Vector4Impl, 0, 1, 1, 3> XYYW;
			Swizzle<T, Vector4Impl, 0, 1, 2, 0> XYZX;
			Swizzle<T, Vector4Impl, 0, 1, 2, 1> XYZY;
			Swizzle<T, Vector4Impl, 0, 1, 2, 2> XYZZ;
			Swizzle<T, Vector4Impl, 0, 1, 2, 3> XYZW;
			Swizzle<T, Vector4Impl, 0, 1, 3, 0> XYWX;
			Swizzle<T, Vector4Impl, 0, 1, 3, 1> XYWY;
			Swizzle<T, Vector4Impl, 0, 1, 3, 2> XYWZ;
			Swizzle<T, Vector4Impl, 0, 1, 3, 3> XYWW;
			Swizzle<T, Vector4Impl, 0, 2, 0, 0> XZXX;
			Swizzle<T, Vector4Impl, 0, 2, 0, 1> XZXY;
			Swizzle<T, Vector4Impl, 0, 2, 0, 2> XZXZ;
			Swizzle<T, Vector4Impl, 0, 2, 0, 3> XZXW;
			Swizzle<T, Vector4Impl, 0, 2, 1, 0> XZYX;
			Swizzle<T, Vector4Impl, 0, 2, 1, 1> XZYY;
			Swizzle<T, Vector4Impl, 0, 2, 1, 2> XZYZ;
			Swizzle<T, Vector4Impl, 0, 2, 1, 3> XZYW;
			Swizzle<T, Vector4Impl, 0, 2, 2, 0> XZZX;
			Swizzle<T, Vector4Impl, 0, 2, 2, 1> XZZY;
			Swizzle<T, Vector4Impl, 0, 2, 2, 2> XZZZ;
			Swizzle<T, Vector4Impl, 0, 2, 2, 3> XZZW;
			Swizzle<T, Vector4Impl, 0, 2, 3, 0> XZWX;
			Swizzle<T, Vector4Impl, 0, 2, 3, 1> XZWY;
			Swizzle<T, Vector4Impl, 0, 2, 3, 2> XZWZ;
			Swizzle<T, Vector4Impl, 0, 2, 3, 3> XZWW;
			Swizzle<T, Vector4Impl, 0, 3, 0, 0> XWXX;
			Swizzle<T, Vector4Impl, 0, 3, 0, 1> XWXY;
			Swizzle<T, Vector4Impl, 0, 3, 0, 2> XWXZ;
			Swizzle<T, Vector4Impl, 0, 3, 0, 3> XWXW;
			Swizzle<T, Vector4Impl, 0, 3, 1, 0> XWYX;
			Swizzle<T, Vector4Impl, 0, 3, 1, 1> XWYY;
			Swizzle<T, Vector4Impl, 0, 3, 1, 2> XWYZ;
			Swizzle<T, Vector4Impl, 0, 3, 1, 3> XWYW;
			Swizzle<T, Vector4Impl, 0, 3, 2, 0> XWZX;
			Swizzle<T, Vector4Impl, 0, 3, 2, 1> XWZY;
			Swizzle<T, Vector4Impl, 0, 3, 2, 2> XWZZ;
			Swizzle<T, Vector4Impl, 0, 3, 2, 3> XWZW;
			Swizzle<T, Vector4Impl, 0, 3, 3, 0> XWWX;
			Swizzle<T, Vector4Impl, 0, 3, 3, 1> XWWY;
			Swizzle<T, Vector4Impl, 0, 3, 3, 2> XWWZ;
			Swizzle<T, Vector4Impl, 0, 3, 3, 3> XWWW;
			Swizzle<T, Vector4Impl, 1, 0, 0, 0> YXXX;
			Swizzle<T, Vector4Impl, 1, 0, 0, 1> YXXY;
			Swizzle<T, Vector4Impl, 1, 0, 0, 2> YXXZ;
			Swizzle<T, Vector4Impl, 1, 0, 0, 3> YXXW;
			Swizzle<T, Vector4Impl, 1, 0, 1, 0> YXYX;
			Swizzle<T, Vector4Impl, 1, 0, 1, 1> YXYY;
			Swizzle<T, Vector4Impl, 1, 0, 1, 2> YXYZ;
			Swizzle<T, Vector4Impl, 1, 0, 1, 3> YXYW;
			Swizzle<T, Vector4Impl, 1, 0, 2, 0> YXZX;
			Swizzle<T, Vector4Impl, 1, 0, 2, 1> YXZY;
			Swizzle<T, Vector4Impl, 1, 0, 2, 2> YXZZ;
			Swizzle<T, Vector4Impl, 1, 0, 2, 3> YXZW;
			Swizzle<T, Vector4Impl, 1, 0, 3, 0> YXWX;
			Swizzle<T, Vector4Impl, 1, 0, 3, 1> YXWY;
			Swizzle<T, Vector4Impl, 1, 0, 3, 2> YXWZ;
			Swizzle<T, Vector4Impl, 1, 0, 3, 3> YXWW;
			Swizzle<T, Vector4Impl, 1, 1, 0, 0> YYXX;
			Swizzle<T, Vector4Impl, 1, 1, 0, 1> YYXY;
			Swizzle<T, Vector4Impl, 1, 1, 0, 2> YYXZ;
			Swizzle<T, Vector4Impl, 1, 1, 0, 3> YYXW;
			Swizzle<T, Vector4Impl, 1, 1, 1, 0> YYYX;
			Swizzle<T, Vector4Impl, 1, 1, 1, 1> YYYY;
			Swizzle<T, Vector4Impl, 1, 1, 1, 2> YYYZ;
			Swizzle<T, Vector4Impl, 1, 1, 1, 3> YYYW;
			Swizzle<T, Vector4Impl, 1, 1, 2, 0> YYZX;
			Swizzle<T, Vector4Impl, 1, 1, 2, 1> YYZY;
			Swizzle<T, Vector4Impl, 1, 1, 2, 2> YYZZ;
			Swizzle<T, Vector4Impl, 1, 1, 2, 3> YYZW;
			Swizzle<T, Vector4Impl, 1, 1, 3, 0> YYWX;
			Swizzle<T, Vector4Impl, 1, 1, 3, 1> YYWY;
			Swizzle<T, Vector4Impl, 1, 1, 3, 2> YYWZ;
			Swizzle<T, Vector4Impl, 1, 1, 3, 3> YYWW;
			Swizzle<T, Vector4Impl, 1, 2, 0, 0> YZXX;
			Swizzle<T, Vector4Impl, 1, 2, 0, 1> YZXY;
			Swizzle<T, Vector4Impl, 1, 2, 0, 2> YZXZ;
			Swizzle<T, Vector4Impl, 1, 2, 0, 3> YZXW;
			Swizzle<T, Vector4Impl, 1, 2, 1, 0> YZYX;
			Swizzle<T, Vector4Impl, 1, 2, 1, 1> YZYY;
			Swizzle<T, Vector4Impl, 1, 2, 1, 2> YZYZ;
			Swizzle<T, Vector4Impl, 1, 2, 1, 3> YZYW;
			Swizzle<T, Vector4Impl, 1, 2, 2, 0> YZZX;
			Swizzle<T, Vector4Impl, 1, 2, 2, 1> YZZY;
			Swizzle<T, Vector4Impl, 1, 2, 2, 2> YZZZ;
			Swizzle<T, Vector4Impl, 1, 2, 2, 3> YZZW;
			Swizzle<T, Vector4Impl, 1, 2, 3, 0> YZWX;
			Swizzle<T, Vector4Impl, 1, 2, 3, 1> YZWY;
			Swizzle<T, Vector4Impl, 1, 2, 3, 2> YZWZ;
			Swizzle<T, Vector4Impl, 1, 2, 3, 3> YZWW;
			Swizzle<T, Vector4Impl, 1, 3, 0, 0> YWXX;
			Swizzle<T, Vector4Impl, 1, 3, 0, 1> YWXY;
			Swizzle<T, Vector4Impl, 1, 3, 0, 2> YWXZ;
			Swizzle<T, Vector4Impl, 1, 3, 0, 3> YWXW;
			Swizzle<T, Vector4Impl, 1, 3, 1, 0> YWYX;
			Swizzle<T, Vector4Impl, 1, 3, 1, 1> YWYY;
			Swizzle<T, Vector4Impl, 1, 3, 1, 2> YWYZ;
			Swizzle<T, Vector4Impl, 1, 3, 1, 3> YWYW;
			Swizzle<T, Vector4Impl, 1, 3, 2, 0> YWZX;
			Swizzle<T, Vector4Impl, 1, 3, 2, 1> YWZY;
			Swizzle<T, Vector4Impl, 1, 3, 2, 2> YWZZ;
			Swizzle<T, Vector4Impl, 1, 3, 2, 3> YWZW;
			Swizzle<T, Vector4Impl, 1, 3, 3, 0> YWWX;
			Swizzle<T, Vector4Impl, 1, 3, 3, 1> YWWY;
			Swizzle<T, Vector4Impl, 1, 3, 3, 2> YWWZ;
			Swizzle<T, Vector4Impl, 1, 3, 3, 3> YWWW;
			Swizzle<T, Vector4Impl, 2, 0, 0, 0> ZXXX;
			Swizzle<T, Vector4Impl, 2, 0, 0, 1> ZXXY;
			Swizzle<T, Vector4Impl, 2, 0, 0, 2> ZXXZ;
			Swizzle<T, Vector4Impl, 2, 0, 0, 3> ZXXW;
			Swizzle<T, Vector4Impl, 2, 0, 1, 0> ZXYX;
			Swizzle<T, Vector4Impl, 2, 0, 1, 1> ZXYY;
			Swizzle<T, Vector4Impl, 2, 0, 1, 2> ZXYZ;
			Swizzle<T, Vector4Impl, 2, 0, 1, 3> ZXYW;
			Swizzle<T, Vector4Impl, 2, 0, 2, 0> ZXZX;
			Swizzle<T, Vector4Impl, 2, 0, 2, 1> ZXZY;
			Swizzle<T, Vector4Impl, 2, 0, 2, 2> ZXZZ;
			Swizzle<T, Vector4Impl, 2, 0, 2, 3> ZXZW;
			Swizzle<T, Vector4Impl, 2, 0, 3, 0> ZXWX;
			Swizzle<T, Vector4Impl, 2, 0, 3, 1> ZXWY;
			Swizzle<T, Vector4Impl, 2, 0, 3, 2> ZXWZ;
			Swizzle<T, Vector4Impl, 2, 0, 3, 3> ZXWW;
			Swizzle<T, Vector4Impl, 2, 1, 0, 0> ZYXX;
			Swizzle<T, Vector4Impl, 2, 1, 0, 1> ZYXY;
			Swizzle<T, Vector4Impl, 2, 1, 0, 2> ZYXZ;
			Swizzle<T, Vector4Impl, 2, 1, 0, 3> ZYXW;
			Swizzle<T, Vector4Impl, 2, 1, 1, 0> ZYYX;
			Swizzle<T, Vector4Impl, 2, 1, 1, 1> ZYYY;
			Swizzle<T, Vector4Impl, 2, 1, 1, 2> ZYYZ;
			Swizzle<T, Vector4Impl, 2, 1, 1, 3> ZYYW;
			Swizzle<T, Vector4Impl, 2, 1, 2, 0> ZYZX;
			Swizzle<T, Vector4Impl, 2, 1, 2, 1> ZYZY;
			Swizzle<T, Vector4Impl, 2, 1, 2, 2> ZYZZ;
			Swizzle<T, Vector4Impl, 2, 1, 2, 3> ZYZW;
			Swizzle<T, Vector4Impl, 2, 1, 3, 0> ZYWX;
			Swizzle<T, Vector4Impl, 2, 1, 3, 1> ZYWY;
			Swizzle<T, Vector4Impl, 2, 1, 3, 2> ZYWZ;
			Swizzle<T, Vector4Impl, 2, 1, 3, 3> ZYWW;
			Swizzle<T, Vector4Impl, 2, 2, 0, 0> ZZXX;
			Swizzle<T, Vector4Impl, 2, 2, 0, 1> ZZXY;
			Swizzle<T, Vector4Impl, 2, 2, 0, 2> ZZXZ;
			Swizzle<T, Vector4Impl, 2, 2, 0, 3> ZZXW;
			Swizzle<T, Vector4Impl, 2, 2, 1, 0> ZZYX;
			Swizzle<T, Vector4Impl, 2, 2, 1, 1> ZZYY;
			Swizzle<T, Vector4Impl, 2, 2, 1, 2> ZZYZ;
			Swizzle<T, Vector4Impl, 2, 2, 1, 3> ZZYW;
			Swizzle<T, Vector4Impl, 2, 2, 2, 0> ZZZX;
			Swizzle<T, Vector4Impl, 2, 2, 2, 1> ZZZY;
			Swizzle<T, Vector4Impl, 2, 2, 2, 2> ZZZZ;
			Swizzle<T, Vector4Impl, 2, 2, 2, 3> ZZZW;
			Swizzle<T, Vector4Impl, 2, 2, 3, 0> ZZWX;
			Swizzle<T, Vector4Impl, 2, 2, 3, 1> ZZWY;
			Swizzle<T, Vector4Impl, 2, 2, 3, 2> ZZWZ;
			Swizzle<T, Vector4Impl, 2, 2, 3, 3> ZZWW;
			Swizzle<T, Vector4Impl, 2, 3, 0, 0> ZWXX;
			Swizzle<T, Vector4Impl, 2, 3, 0, 1> ZWXY;
			Swizzle<T, Vector4Impl, 2, 3, 0, 2> ZWXZ;
			Swizzle<T, Vector4Impl, 2, 3, 0, 3> ZWXW;
			Swizzle<T, Vector4Impl, 2, 3, 1, 0> ZWYX;
			Swizzle<T, Vector4Impl, 2, 3, 1, 1> ZWYY;
			Swizzle<T, Vector4Impl, 2, 3, 1, 2> ZWYZ;
			Swizzle<T, Vector4Impl, 2, 3, 1, 3> ZWYW;
			Swizzle<T, Vector4Impl, 2, 3, 2, 0> ZWZX;
			Swizzle<T, Vector4Impl, 2, 3, 2, 1> ZWZY;
			Swizzle<T, Vector4Impl, 2, 3, 2, 2> ZWZZ;
			Swizzle<T, Vector4Impl, 2, 3, 2, 3> ZWZW;
			Swizzle<T, Vector4Impl, 2, 3, 3, 0> ZWWX;
			Swizzle<T, Vector4Impl, 2, 3, 3, 1> ZWWY;
			Swizzle<T, Vector4Impl, 2, 3, 3, 2> ZWWZ;
			Swizzle<T, Vector4Impl, 2, 3, 3, 3> ZWWW;
			Swizzle<T, Vector4Impl, 3, 0, 0, 0> WXXX;
			Swizzle<T, Vector4Impl, 3, 0, 0, 1> WXXY;
			Swizzle<T, Vector4Impl, 3, 0, 0, 2> WXXZ;
			Swizzle<T, Vector4Impl, 3, 0, 0, 3> WXXW;
			Swizzle<T, Vector4Impl, 3, 0, 1, 0> WXYX;
			Swizzle<T, Vector4Impl, 3, 0, 1, 1> WXYY;
			Swizzle<T, Vector4Impl, 3, 0, 1, 2> WXYZ;
			Swizzle<T, Vector4Impl, 3, 0, 1, 3> WXYW;
			Swizzle<T, Vector4Impl, 3, 0, 2, 0> WXZX;
			Swizzle<T, Vector4Impl, 3, 0, 2, 1> WXZY;
			Swizzle<T, Vector4Impl, 3, 0, 2, 2> WXZZ;
			Swizzle<T, Vector4Impl, 3, 0, 2, 3> WXZW;
			Swizzle<T, Vector4Impl, 3, 0, 3, 0> WXWX;
			Swizzle<T, Vector4Impl, 3, 0, 3, 1> WXWY;
			Swizzle<T, Vector4Impl, 3, 0, 3, 2> WXWZ;
			Swizzle<T, Vector4Impl, 3, 0, 3, 3> WXWW;
			Swizzle<T, Vector4Impl, 3, 1, 0, 0> WYXX;
			Swizzle<T, Vector4Impl, 3, 1, 0, 1> WYXY;
			Swizzle<T, Vector4Impl, 3, 1, 0, 2> WYXZ;
			Swizzle<T, Vector4Impl, 3, 1, 0, 3> WYXW;
			Swizzle<T, Vector4Impl, 3, 1, 1, 0> WYYX;
			Swizzle<T, Vector4Impl, 3, 1, 1, 1> WYYY;
			Swizzle<T, Vector4Impl, 3, 1, 1, 2> WYYZ;
			Swizzle<T, Vector4Impl, 3, 1, 1, 3> WYYW;
			Swizzle<T, Vector4Impl, 3, 1, 2, 0> WYZX;
			Swizzle<T, Vector4Impl, 3, 1, 2, 1> WYZY;
			Swizzle<T, Vector4Impl, 3, 1, 2, 2> WYZZ;
			Swizzle<T, Vector4Impl, 3, 1, 2, 3> WYZW;
			Swizzle<T, Vector4Impl, 3, 1, 3, 0> WYWX;
			Swizzle<T, Vector4Impl, 3, 1, 3, 1> WYWY;
			Swizzle<T, Vector4Impl, 3, 1, 3, 2> WYWZ;
			Swizzle<T, Vector4Impl, 3, 1, 3, 3> WYWW;
			Swizzle<T, Vector4Impl, 3, 2, 0, 0> WZXX;
			Swizzle<T, Vector4Impl, 3, 2, 0, 1> WZXY;
			Swizzle<T, Vector4Impl, 3, 2, 0, 2> WZXZ;
			Swizzle<T, Vector4Impl, 3, 2, 0, 3> WZXW;
			Swizzle<T, Vector4Impl, 3, 2, 1, 0> WZYX;
			Swizzle<T, Vector4Impl, 3, 2, 1, 1> WZYY;
			Swizzle<T, Vector4Impl, 3, 2, 1, 2> WZYZ;
			Swizzle<T, Vector4Impl, 3, 2, 1, 3> WZYW;
			Swizzle<T, Vector4Impl, 3, 2, 2, 0> WZZX;
			Swizzle<T, Vector4Impl, 3, 2, 2, 1> WZZY;
			Swizzle<T, Vector4Impl, 3, 2, 2, 2> WZZZ;
			Swizzle<T, Vector4Impl, 3, 2, 2, 3> WZZW;
			Swizzle<T, Vector4Impl, 3, 2, 3, 0> WZWX;
			Swizzle<T, Vector4Impl, 3, 2, 3, 1> WZWY;
			Swizzle<T, Vector4Impl, 3, 2, 3, 2> WZWZ;
			Swizzle<T, Vector4Impl, 3, 2, 3, 3> WZWW;
			Swizzle<T, Vector4Impl, 3, 3, 0, 0> WWXX;
			Swizzle<T, Vector4Impl, 3, 3, 0, 1> WWXY;
			Swizzle<T, Vector4Impl, 3, 3, 0, 2> WWXZ;
			Swizzle<T, Vector4Impl, 3, 3, 0, 3> WWXW;
			Swizzle<T, Vector4Impl, 3, 3, 1, 0> WWYX;
			Swizzle<T, Vector4Impl, 3, 3, 1, 1> WWYY;
			Swizzle<T, Vector4Impl, 3, 3, 1, 2> WWYZ;
			Swizzle<T, Vector4Impl, 3, 3, 1, 3> WWYW;
			Swizzle<T, Vector4Impl, 3, 3, 2, 0> WWZX;
			Swizzle<T, Vector4Impl, 3, 3, 2, 1> WWZY;
			Swizzle<T, Vector4Impl, 3, 3, 2, 2> WWZZ;
			Swizzle<T, Vector4Impl, 3, 3, 2, 3> WWZW;
			Swizzle<T, Vector4Impl, 3, 3, 3, 0> WWWX;
			Swizzle<T, Vector4Impl, 3, 3, 3, 1> WWWY;
			Swizzle<T, Vector4Impl, 3, 3, 3, 2> WWWZ;
			Swizzle<T, Vector4Impl, 3, 3, 3, 3> WWWW;
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

		Vector4Impl operator*=(const Vector4Impl& rhs) {
			X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z; W *= rhs.W;
			return *this;
		}
		Vector4Impl operator+=(const Vector4Impl& rhs) {
			X += rhs.X; Y += rhs.Y; Z += rhs.Z; W += rhs.W;
			return *this;
		}
		Vector4Impl operator-=(const Vector4Impl& rhs) {
			X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z; W -= rhs.W;
			return *this;
		}
		Vector4Impl operator/=(const Vector4Impl& rhs) {
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
		bool operator==(const Vector4Impl& rhs) const {
			return X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W;
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

		T operator[](uint32 Index) const {
			return *(reinterpret_cast<const T*>(this) + Index);
		}

		operator UE_Vector() {
			return UE_Vector(X, Y, Z, W);
		}
	};
}