#pragma once
#include "Auxiliary.h"
namespace FRAMEWORK {
	// ShaderHelper Vector
    // Simulates hlsl swizzle operations.
    // Follows hlsl casting rule but
    // disables some implicit conversions:
    //      1. vectorN -> vectorM (N>M)   (eg. float2 a = float3(1,1,1)
    //      2. scalar <-> vector(eg. float3 a = 1.0f, float a = float4(1,1,1,1) or float a  = .xyz
    // disables some arithmetic operations:
    //      1. vectorN op vectorM (N!=M)  (eg. .xyz + .xx)
    //      2. FVector op Vector

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

	template<uint32 ElementNum, typename Base, template<typename>typename Target, uint32... Indexes>
	struct Swizzle
	{
		template <uint32 OtherElementNum, typename OtherBase, template<typename>typename OtherTarget, uint32 ... OtherIndexes>
		friend struct Swizzle;
        
        template<typename T>
        friend struct VectorImpl;
        template<typename T>
        friend struct Vector2Impl;
        template<typename T>
        friend struct Vector4Impl;

    private:
        Swizzle() = default;
        Swizzle(const Swizzle&) = default;
        
	public:
        template<typename T>
        Swizzle& operator=(const Target<T>& rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, Indexes...>>::Assign(Data, rhs);
            return *this;
        }
        
        Swizzle& operator=(const Swizzle& rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, Indexes...>>::Assign(Data, rhs.Data);
            return *this;
        }
        
        template <uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes>
        Swizzle& operator=(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::Assign(Data, rhs.Data);
            return *this;
		}
        
        template <uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes>
        Swizzle& operator*=(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::MulAssign(Data, rhs.Data);
            return *this;
        }
        
        template <uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes>
        Swizzle& operator+=(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::AddAssign(Data, rhs.Data);
            return *this;
        }
        
        template <uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes>
        Swizzle& operator-=(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::SubAssign(Data, rhs.Data);
            return *this;
        }
        
        template <uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes>
        Swizzle& operator/=(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::DivAssign(Data, rhs.Data);
            return *this;
        }
        
        template<typename OtherBase,
            typename = std::enable_if_t<std::is_arithmetic_v<OtherBase>>
        >
        Swizzle& operator*=(OtherBase rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            ((Data[Indexes] *= rhs), ...);
            return *this;
        }
        template<typename OtherBase,
            typename = std::enable_if_t<std::is_arithmetic_v<OtherBase>>
        >
        Swizzle& operator+=(OtherBase rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            ((Data[Indexes] += rhs), ...);
            return *this;
        }
        template<typename OtherBase,
            typename = std::enable_if_t<std::is_arithmetic_v<OtherBase>>
        >
        Swizzle& operator-=(OtherBase rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            ((Data[Indexes] -= rhs), ...);
            return *this;
        }
        template<typename OtherBase,
            typename = std::enable_if_t<std::is_arithmetic_v<OtherBase>>
        >
        Swizzle& operator/=(OtherBase rhs)
        {
            static_assert(!AUX::HasDuplicateComp_V<Indexes...>, "cannot assign(contains duplicate components)");
            ((Data[Indexes] /= rhs), ...);
            return *this;
        }

        
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator+(OtherBase lhs, const Swizzle& rhs) {
            return Target<PromotedType>{lhs + rhs.Data[Indexes]...};
        }
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator*(OtherBase lhs, const Swizzle& rhs) {
            return Target<PromotedType>{lhs * rhs.Data[Indexes]...};
        }
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator-(OtherBase lhs, const Swizzle& rhs) {
            return Target<PromotedType>{lhs - rhs.Data[Indexes]...};
        }
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator/(OtherBase lhs, const Swizzle& rhs) {
            return Target<PromotedType>{lhs / rhs.Data[Indexes]...};
        }
        
        
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator+(const Swizzle& lhs, OtherBase rhs) {
            return Target<PromotedType>{lhs.Data[Indexes] + rhs...};
        }
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator*(const Swizzle& lhs, OtherBase rhs) {
            return Target<PromotedType>{lhs.Data[Indexes] * rhs...};
        }
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator-(const Swizzle& lhs, OtherBase rhs) {
            return Target<PromotedType>{lhs.Data[Indexes] - rhs...};
        }
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
        friend auto operator/(const Swizzle& lhs, OtherBase rhs) {
            return Target<PromotedType>{lhs.Data[Indexes] / rhs...};
        }
        

        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator+(const Target<OtherBase>& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>
            ::template Sum<Target<PromotedType>>(lhs, rhs.Data);
		}
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator*(const Target<OtherBase>& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>::
            template Mul<Target<PromotedType>>(lhs, rhs.Data);
		}
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator-(const Target<OtherBase>& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>::
            template Sub<Target<PromotedType>>(lhs, rhs.Data);
		}
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator/(const Target<OtherBase>& lhs, const Swizzle& rhs) {
			return AUX::Op<TMakeIntegerSequence<uint32, sizeof...(Indexes)>, TIntegerSequence<uint32, Indexes...>>::
            template Div<Target<PromotedType>>(lhs, rhs.Data);
		}
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator+(const Swizzle& lhs, const Target<OtherBase>& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::
            template Sum<Target<PromotedType>>(lhs.Data, rhs);
		}
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator*(const Swizzle& lhs, const Target<OtherBase>& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::
            template Mul<Target<PromotedType>>(lhs.Data, rhs);
		}
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator-(const Swizzle& lhs, const Target<OtherBase>& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::
            template Sub<Target<PromotedType>>(lhs.Data, rhs);
		}
        template<typename OtherBase,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		friend auto operator/(const Swizzle& lhs, const Target<OtherBase>& rhs) {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TMakeIntegerSequence<uint32, sizeof...(Indexes)>>::
            template Div<Target<PromotedType>>(lhs.Data, rhs);
		}

        
		template<uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		auto operator+(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::
            template Sum<Target<PromotedType>>(Data, rhs.Data);
		}
		template<uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		auto operator*(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::
            template Mul<Target<PromotedType>>(Data, rhs.Data);
		}
		template<uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		auto operator-(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::
            template Sub<Target<PromotedType>>(Data, rhs.Data);
		}
		template<uint32 OtherElementNum, typename OtherBase, uint32 ... OtherIndexes,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<Base,OtherBase>
        >
		auto operator/(const Swizzle<OtherElementNum, OtherBase, Target, OtherIndexes...>& rhs) const {
			return AUX::Op<TIntegerSequence<uint32, Indexes...>, TIntegerSequence<uint32, OtherIndexes...>>::
            template Div<Target<PromotedType>>(Data, rhs.Data);
		}
        
        

	private:
		Base Data[ElementNum];
	};

	template<typename T>
	struct Vector2Impl
	{
        template<typename U>
        using UE_Vector = UE::Math::TVector2<U>;
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
        template<typename U>
        Vector2Impl(const UE_Vector<U>& Vec) : X(Vec.X), Y(Vec.Y) {}
        
		explicit Vector2Impl(T IntT) : X(IntT), Y(IntT) {}
		Vector2Impl(T IntX, T IntY) : X(IntX), Y(IntY) {}
        
        template<typename U>
        Vector2Impl(const Vector2Impl<U>& Vec)
            : Vector2Impl(Vec.X, Vec.Y)
        {
        }
        
        template<uint32 ElementNum, typename Base, uint32... Indexes>
        Vector2Impl(const Swizzle<ElementNum, Base, Vector2Impl, Indexes...>& SwizzleItem)
            : Vector2Impl(SwizzleItem.Data[Indexes]...)
        {
        }
        
        Vector2Impl& operator=(const Vector2Impl& rhs)
        {
            Vector2Impl Temp{rhs};
            Swap(Temp, *this);
            return *this;
        }
        
        template<typename U> friend auto operator*(const UE_Vector<U>&, const Vector2Impl&) = delete;
        template<typename U> friend auto operator*(const Vector2Impl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator+(const UE_Vector<U>&, const Vector2Impl&) = delete;
        template<typename U> friend auto operator+(const Vector2Impl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator-(const UE_Vector<U>&, const Vector2Impl&) = delete;
        template<typename U> friend auto operator-(const Vector2Impl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator/(const UE_Vector<U>&, const Vector2Impl&) = delete;
        template<typename U> friend auto operator/(const Vector2Impl&, const UE_Vector<U>&) = delete;

        //Follows arithmetic conversions
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator*(const Vector2Impl<U>& rhs) const {
			return Vector2Impl<PromotedType>(X * rhs.X, Y * rhs.Y);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator+(const Vector2Impl<U>& rhs) const {
			return Vector2Impl(X + rhs.X, Y + rhs.Y);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator-(const Vector2Impl& rhs) const {
			return Vector2Impl<PromotedType>(X - rhs.X, Y - rhs.Y);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator/(const Vector2Impl<U>& rhs) const {
			return Vector2Impl<PromotedType>(X / rhs.X, Y / rhs.Y);
		}

        template<typename U>
		Vector2Impl& operator*=(const Vector2Impl<U>& rhs) {
			X *= rhs.X; Y *= rhs.Y;
			return *this;
		}
        template<typename U>
		Vector2Impl& operator+=(const Vector2Impl<U>& rhs) {
			X += rhs.X; Y += rhs.Y;
			return *this;
		}
        template<typename U>
		Vector2Impl& operator-=(const Vector2Impl<U>& rhs) {
			X -= rhs.X; Y -= rhs.Y;
			return *this;
		}
        template<typename U>
		Vector2Impl& operator/=(const Vector2Impl<U>& rhs) {
			X /= rhs.X; Y /= rhs.Y;
			return *this;
		}
        
        template<typename U>
        bool operator==(const Vector2Impl<U>& rhs) const {
            return X == rhs.X && Y == rhs.Y;
        }
        template<typename U>
        bool operator!=(const Vector2Impl<U>& rhs) const {
            return X != rhs.X || Y != rhs.Y;
        }

        template<typename U>
		bool Equals(const Vector2Impl<U>& rhs, T eps = std::numeric_limits<T>::epsilon()) const {
			return FMath::Abs(X - rhs.X) < eps && FMath::Abs(Y - rhs.Y) < eps;
		}
        
        Vector2Impl operator-() const
        {
            return {-X, -Y};
        }

        //Scalar support
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        auto operator*(U rhs) const {
            return Vector2Impl<PromotedType>(X * rhs, Y * rhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        auto operator+(U rhs) const {
            return Vector2Impl<PromotedType>(X + rhs, Y + rhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        auto operator-(U rhs) const {
            return Vector2Impl<PromotedType>(X - rhs, Y - rhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        auto operator/(U rhs) const {
            return Vector2Impl<PromotedType>(X / rhs, Y / rhs);
        }
        
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator*(U lhs, const Vector2Impl& rhs) {
            return rhs.operator*(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator+(U lhs, const Vector2Impl& rhs) {
            return rhs.operator+(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator-(U lhs, const Vector2Impl& rhs) {
            return -rhs.operator-(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator/(U lhs, const Vector2Impl& rhs) {
            return Vector2Impl<PromotedType>(lhs / rhs.X, lhs / rhs.Y);
        }
        
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector2Impl& operator*=(U rhs) {
            X *= rhs; Y *= rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector2Impl& operator+=(U rhs) {
            X += rhs; Y += rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector2Impl& operator-=(U rhs) {
            X -= rhs; Y -= rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector2Impl& operator/=(U rhs) {
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

        template<typename U>
		operator UE_Vector<U>() const {
			return UE_Vector<U>(X, Y);
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
        template<typename U>
		using UE_Vector = UE::Math::TVector<U>;
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
        template<typename U>
		VectorImpl(const UE_Vector<U>& Vec) : X(Vec.X), Y(Vec.Y), Z(Vec.Z) {}
        
        explicit VectorImpl(T IntT) : X(IntT), Y(IntT), Z(IntT) {}
		VectorImpl(T IntX, T IntY, T IntZ) : X(IntX), Y(IntY), Z(IntZ) {}
        
        template<typename U>
        VectorImpl(const VectorImpl<U>& Vec)
            : VectorImpl(Vec.X, Vec.Y, Vec.Z)
        {
        }
        
        template<uint32 ElementNum, typename Base, uint32... Indexes>
        VectorImpl(const Swizzle<ElementNum, Base, VectorImpl, Indexes...>& SwizzleItem)
            : VectorImpl(SwizzleItem.Data[Indexes]...)
        {
        }
        
        VectorImpl& operator=(const VectorImpl& rhs)
        {
            VectorImpl Temp{rhs};
            Swap(Temp, *this);
            return *this;
        }
        
        template<typename U> friend auto operator*(const UE_Vector<U>&, const VectorImpl&) = delete;
        template<typename U> friend auto operator*(const VectorImpl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator+(const UE_Vector<U>&, const VectorImpl&) = delete;
        template<typename U> friend auto operator+(const VectorImpl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator-(const UE_Vector<U>&, const VectorImpl&) = delete;
        template<typename U> friend auto operator-(const VectorImpl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator/(const UE_Vector<U>&, const VectorImpl&) = delete;
        template<typename U> friend auto operator/(const VectorImpl&, const UE_Vector<U>&) = delete;
        
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator*(const VectorImpl<U>& rhs) const {
			return VectorImpl<PromotedType>(X * rhs.X, Y * rhs.Y, Z * rhs.Z);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator+(const VectorImpl<U>& rhs) const {
			return VectorImpl<PromotedType>(X + rhs.X, Y + rhs.Y, Z + rhs.Z);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator-(const VectorImpl<U>& rhs) const {
			return VectorImpl<PromotedType>(X - rhs.X, Y - rhs.Y, Z - rhs.Z);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator/(const VectorImpl<U>& rhs) const {
			return VectorImpl<PromotedType>(X / rhs.X, Y / rhs.Y, Z / rhs.Z);
		}

        template<typename U>
		VectorImpl& operator*=(const VectorImpl<U>& rhs) {
			X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z;
			return *this;
		}
        template<typename U>
		VectorImpl& operator+=(const VectorImpl<U>& rhs) {
			X += rhs.X; Y += rhs.Y; Z += rhs.Z;
			return *this;
		}
        template<typename U>
		VectorImpl& operator-=(const VectorImpl<U>& rhs) {
			X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z;
			return *this;
		}
        template<typename U>
		VectorImpl& operator/=(const VectorImpl<U>& rhs) {
			X /= rhs.X; Y /= rhs.Y; Z /= rhs.Z;
			return *this;
		}
        
        template<typename U>
        bool operator==(const VectorImpl<U>& rhs) const {
            return X == rhs.X && Y == rhs.Y && Z == rhs.Z;
        }
        template<typename U>
        bool operator!=(const VectorImpl<U>& rhs) const {
            return X != rhs.X || Y != rhs.Y || Z != rhs.Z;
        }
        template<typename U>
		bool Equals(const VectorImpl<U>& rhs, T eps = std::numeric_limits<T>::epsilon()) const {
			return FMath::Abs(X - rhs.X) < eps && FMath::Abs(Y - rhs.Y) < eps && FMath::Abs(Z - rhs.Z) < eps;
		}
        
        VectorImpl operator-() const
        {
            return {-X, -Y, -Z};
        }

        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator*(U rhs) const {
			return VectorImpl<PromotedType>(X * rhs, Y * rhs, Z * rhs);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator+(U rhs) const {
			return VectorImpl<PromotedType>(X + rhs, Y + rhs, Z + rhs);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator-(U rhs) const {
			return VectorImpl<PromotedType>(X - rhs, Y - rhs, Z - rhs);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator/(U rhs) const {
			return VectorImpl<PromotedType>(X / rhs, Y / rhs, Z / rhs);
		}
        
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator*(U lhs, const VectorImpl& rhs) {
            return rhs.operator*(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator+(U lhs, const VectorImpl& rhs) {
            return rhs.operator*(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator-(U lhs, const VectorImpl& rhs) {
            return -rhs.operator-(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator/(U lhs, const VectorImpl& rhs) {
            return VectorImpl<PromotedType>(lhs / rhs.X, lhs / rhs.Y, lhs / rhs.Z);
        }
        
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        VectorImpl& operator*=(U rhs) {
            X *= rhs; Y *= rhs; Z *= rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        VectorImpl& operator+=(U rhs) {
            X += rhs; Y += rhs; Z += rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        VectorImpl& operator-=(U rhs) {
            X -= rhs; Y -= rhs; Z -= rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        VectorImpl& operator/=(U rhs) {
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

        template<typename U>
		operator UE_Vector<U>() const {
            return UE_Vector<U>(X, Y, Z);
		}

		FString ToString() const {
			return FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f"), X, Y, Z);
		}

		const T* GetData() const {
			return Data;
		}
	};

	template<typename T>
	struct Vector4Impl
	{
        template<typename U>
        using UE_Vector = UE::Math::TVector4<U>;
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
        template<typename U>
		Vector4Impl(const UE_Vector<U>& Vec) : X(Vec.X), Y(Vec.Y), Z(Vec.Z), W(Vec.W) {}
        
        explicit Vector4Impl(T IntT) : X(IntT), Y(IntT), Z(IntT), W(IntT) {}
		Vector4Impl(T IntX, T IntY, T IntZ, T IntW) : X(IntX), Y(IntY), Z(IntZ), W(IntW) {}
        
        template<typename U>
        Vector4Impl(const Vector4Impl<U>& Vec)
            : Vector4Impl(Vec.X, Vec.Y, Vec.Z, Vec.W)
        {
        }
        
        template<uint32 ElementNum, typename Base, uint32... Indexes>
        Vector4Impl(const Swizzle<ElementNum, Base, Vector4Impl, Indexes...>& SwizzleItem)
            : Vector4Impl(SwizzleItem.Data[Indexes]...)
        {
        }
        
        Vector4Impl& operator=(const Vector4Impl& rhs)
        {
            Vector4Impl Temp{rhs};
            Swap(Temp, *this);
            return *this;
        }
        
        template<typename U> friend auto operator*(const UE_Vector<U>&, const Vector4Impl&) = delete;
        template<typename U> friend auto operator*(const Vector4Impl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator+(const UE_Vector<U>&, const Vector4Impl&) = delete;
        template<typename U> friend auto operator+(const Vector4Impl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator-(const UE_Vector<U>&, const Vector4Impl&) = delete;
        template<typename U> friend auto operator-(const Vector4Impl&, const UE_Vector<U>&) = delete;
        template<typename U> friend auto operator/(const UE_Vector<U>&, const Vector4Impl&) = delete;
        template<typename U> friend auto operator/(const Vector4Impl&, const UE_Vector<U>&) = delete;
		
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator*(const Vector4Impl<U>& rhs) const {
			return Vector4Impl<PromotedType>(X * rhs.X, Y * rhs.Y, Z * rhs.Z, W * rhs.W);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator+(const Vector4Impl<U>& rhs) const {
			return Vector4Impl<PromotedType>(X + rhs.X, Y + rhs.Y, Z + rhs.Z, W + rhs.W);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator-(const Vector4Impl<U>& rhs) const {
			return Vector4Impl<PromotedType>(X - rhs.X, Y - rhs.Y, Z - rhs.Z, W - rhs.W);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator/(const Vector4Impl<U>& rhs) const {
			return Vector4Impl<PromotedType>(X / rhs.X, Y / rhs.Y, Z / rhs.Z, W / rhs.W);
		}

        template<typename U>
		Vector4Impl& operator*=(const Vector4Impl<U>& rhs) {
			X *= rhs.X; Y *= rhs.Y; Z *= rhs.Z; W *= rhs.W;
			return *this;
		}
        template<typename U>
		Vector4Impl& operator+=(const Vector4Impl<U>& rhs) {
			X += rhs.X; Y += rhs.Y; Z += rhs.Z; W += rhs.W;
			return *this;
		}
        template<typename U>
		Vector4Impl& operator-=(const Vector4Impl<U>& rhs) {
			X -= rhs.X; Y -= rhs.Y; Z -= rhs.Z; W -= rhs.W;
			return *this;
		}
        template<typename U>
		Vector4Impl& operator/=(const Vector4Impl<U>& rhs) {
			X /= rhs.X; Y /= rhs.Y; Z /= rhs.Z; W /= rhs.W;
			return *this;
		}
        
        template<typename U>
		bool operator==(const Vector4Impl<U>& rhs) const {
			return X == rhs.X && Y == rhs.Y && Z == rhs.Z && W == rhs.W;
		}
        template<typename U>
		bool operator!=(const Vector4Impl<U>& rhs) const {
			return X != rhs.X || Y != rhs.Y || Z != rhs.Z || W != rhs.W;
		}
        template<typename U>
		bool Equals(const Vector4Impl<U>& rhs, T eps = std::numeric_limits<T>::epsilon()) const {
			return FMath::Abs(X - rhs.X) < eps && FMath::Abs(Y - rhs.Y) < eps && FMath::Abs(Z - rhs.Z) < eps && FMath::Abs(W - rhs.W) < eps;
		}
        
        Vector4Impl operator-() const
        {
            return {-X, -Y, -Z, -W};
        }

        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator*(U rhs) const {
			return Vector4Impl<PromotedType>(X * rhs, Y * rhs, Z * rhs, W * rhs);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator+(U rhs) const {
			return Vector4Impl<PromotedType>(X + rhs, Y + rhs, Z + rhs, W + rhs);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator-(U rhs) const {
			return Vector4Impl<PromotedType>(X - rhs, Y - rhs, Z - rhs, W - rhs);
		}
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
		auto operator/(U rhs) const {
			return Vector4Impl<PromotedType>(X / rhs, Y / rhs, Z / rhs, W / rhs);
		}
        
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator*(U lhs, const Vector4Impl& rhs) {
            return rhs.operator*(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator+(U lhs, const Vector4Impl& rhs) {
            return rhs.operator*(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator-(U lhs, const Vector4Impl& rhs) {
            return -rhs.operator-(lhs);
        }
        template<typename U,
            typename PromotedType = AUX::GetPromotedArithmeticType_T<T,U>
        >
        friend auto operator/(U lhs, const Vector4Impl& rhs) {
            return Vector4Impl<PromotedType>(lhs / rhs.X, lhs / rhs.Y, lhs / rhs.Z, lhs / rhs.W);
        }
        
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector4Impl& operator*=(U rhs) {
            X *= rhs; Y *= rhs; Z *= rhs; W *= rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector4Impl& operator+=(U rhs) {
            X += rhs; Y += rhs; Z += rhs; W += rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector4Impl& operator-=(U rhs) {
            X -= rhs; Y -= rhs; Z -= rhs; W -= rhs;
            return *this;
        }
        template<typename U,
            typename = std::enable_if_t<std::is_arithmetic_v<U>>
        >
        Vector4Impl& operator/=(U rhs) {
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
        
        template<typename U>
		operator UE_Vector<U>() const {
			return UE_Vector<U>(X, Y, Z, W);
		}

		FString ToString() const {
			return FString::Printf(TEXT("X=%3.3f Y=%3.3f Z=%3.3f W=%3.3f"), X, Y, Z, W);
		}

		const T* GetData() const {
			return Data;
		}
	};

}
