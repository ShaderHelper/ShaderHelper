#pragma once
#include "Auxiliary.h"
namespace FRAMEWORK {
	// ShaderHelper Vector
    // Simulates hlsl swizzle operations.

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
        
        template<typename T>
        friend struct VectorImpl;
        template<typename T>
        friend struct Vector2Impl;
        template<typename T>
        friend struct Vector4Impl;
        
    private:
        Swizzle() = default;
        
	public:
        template<typename T>
        Swizzle& operator=(const VectorImpl<T>& rhs)
        {
            static_assert(sizeof...(Indexes) == 3, "cannot assign(the number of comps is not equal)");
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, 3>>::Assign(Data, rhs);
            return *this;
        }
        
        template<typename T>
        Swizzle& operator=(const Vector2Impl<T>& rhs)
        {
            static_assert(sizeof...(Indexes) == 2, "cannot assign(the number of comps is not equal)");
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, 2>>::Assign(Data, rhs);
            return *this;
        }

        template<typename T>
        Swizzle& operator=(const Vector4Impl<T>& rhs)
        {
            static_assert(sizeof...(Indexes) == 4, "cannot assign(the number of comps is not equal)");
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, 4>>::Assign(Data, rhs);
            return *this;
        }
        
        template <uint32 OtherElementNum, typename OtherBase, typename OtherTarget, uint32 ... OtherIndexes>
        Swizzle& operator=(const Swizzle<OtherElementNum, OtherBase, OtherTarget, OtherIndexes...>& rhs)
        {
            static_assert(sizeof...(Indexes) == sizeof...(OtherIndexes), "cannot assign(the number of comps is not equal)");
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::Assign(Data, rhs.Data);
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
                T x, y;
            };
			struct
			{
				T X, Y;
			};
            #include "Vector2SwizzleSet.h"
		};

		Vector2Impl() = default;
		Vector2Impl(const UE_Vector& Vec) : X(Vec.X), Y(Vec.Y) {}
		Vector2Impl(T IntT) : X(IntT), Y(IntT) {}
		Vector2Impl(T IntX, T IntY) : X(IntX), Y(IntY) {}
        
        template<typename U>
        Vector2Impl(const Vector2Impl<U>& Vec)
            : Vector2Impl(Vec.X, Vec.Y)
        {
        }
        
        template<uint32 ElementNum, typename Base, typename Target, uint32... Indexes>
        Vector2Impl(const Swizzle<ElementNum, Base, Target, Indexes...>& SwizzleItem)
            : Vector2Impl(SwizzleItem.Data[Indexes]...)
        {
            static_assert(sizeof...(Indexes) == 2, "cannot convert(the number of comps is not equal)");
        }

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
        
        Vector2Impl& operator*=(T rhs) {
            X *= rhs; Y *= rhs;
            return *this;
        }
        Vector2Impl& operator+=(T rhs) {
            X += rhs; Y += rhs;
            return *this;
        }
        Vector2Impl& operator-=(T rhs) {
            X -= rhs; Y -= rhs;
            return *this;
        }
        Vector2Impl& operator/=(T rhs) {
            X /= rhs; Y /= rhs;
            return *this;
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

		operator UE_Vector() const {
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
                T x, y, z;
            };
			struct
			{
				T X, Y, Z;
			};
            #include "VectorSwizzleSet.h"
		};

		VectorImpl() = default;
		VectorImpl(const UE_Vector& Vec) : X(Vec.X), Y(Vec.Y), Z(Vec.Z) {}
        VectorImpl(T IntT) : X(IntT), Y(IntT), Z(IntT) {}
		VectorImpl(T IntX, T IntY, T IntZ) : X(IntX), Y(IntY), Z(IntZ) {}
        
        template<typename U>
        VectorImpl(const VectorImpl<U>& Vec)
            : VectorImpl(Vec.X, Vec.Y, Vec.Z)
        {
        }
        
        template<uint32 ElementNum, typename Base, typename Target, uint32... Indexes>
        VectorImpl(const Swizzle<ElementNum, Base, Target, Indexes...>& SwizzleItem)
            : VectorImpl(SwizzleItem.Data[Indexes]...)
        {
            static_assert(sizeof...(Indexes) == 3, "cannot convert(the number of comps is not equal)");
        }
        
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
        
        VectorImpl& operator*=(T rhs) {
            X *= rhs; Y *= rhs; Z *= rhs;
            return *this;
        }
        VectorImpl& operator+=(T rhs) {
            X += rhs; Y += rhs; Z += rhs;
            return *this;
        }
        VectorImpl& operator-=(T rhs) {
            X -= rhs; Y -= rhs; Z -= rhs;
            return *this;
        }
        VectorImpl& operator/=(T rhs) {
            X /= rhs; Y /= rhs; Z /= rhs;
            return *this;
        }

		const T& operator[](uint32 Index) const {
			checkf(Index >= 0 && Index < 3, TEXT("Invalid Index"));
			return Data[Index];
		}

		T& operator[](uint32 Index) {
			checkf(Index >= 0 && Index < 3, TEXT("Invalid Index"));
			return Data[Index];
		}

		operator UE_Vector() const {
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
                T x, y, z, w;
            };
			struct
			{
				T X, Y, Z, W;
			};
            #include "Vector4SwizzleSet.h"
		};

		Vector4Impl() = default;
		Vector4Impl(const UE_Vector& Vec) : X(Vec.X), Y(Vec.Y), Z(Vec.Z), W(Vec.W) {}
        Vector4Impl(T IntT) : X(IntT), Y(IntT), Z(IntT), W(IntT) {}
		Vector4Impl(T IntX, T IntY, T IntZ, T IntW) : X(IntX), Y(IntY), Z(IntZ), W(IntW) {}
        
        template<typename U>
        Vector4Impl(const Vector4Impl<U>& Vec)
            : Vector4Impl(Vec.X, Vec.Y, Vec.Z, Vec.W)
        {
        }
        
        template<uint32 ElementNum, typename Base, typename Target, uint32... Indexes>
        Vector4Impl(const Swizzle<ElementNum, Base, Target, Indexes...>& SwizzleItem)
            : Vector4Impl(SwizzleItem.Data[Indexes]...)
        {
            static_assert(sizeof...(Indexes) == 4, "cannot convert(the number of comps is not equal)");
        }
		
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
        
        Vector4Impl& operator*=(T rhs) {
            X *= rhs; Y *= rhs; Z *= rhs; W *= rhs;
            return *this;
        }
        Vector4Impl& operator+=(T rhs) {
            X += rhs; Y += rhs; Z += rhs; W += rhs;
            return *this;
        }
        Vector4Impl& operator-=(T rhs) {
            X -= rhs; Y -= rhs; Z -= rhs; W -= rhs;
            return *this;
        }
        Vector4Impl& operator/=(T rhs) {
            X /= rhs; Y /= rhs; Z /= rhs; W /= rhs;
            return *this;
        }

		const T& operator[](uint32 Index) const {
			checkf(Index >= 0 && Index < 4, TEXT("Invalid Index"));
			return Data[Index];
		}

		T& operator[](uint32 Index) {
			checkf(Index >= 0 && Index < 4, TEXT("Invalid Index"));
			return Data[Index];
		}

		operator UE_Vector() const {
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
