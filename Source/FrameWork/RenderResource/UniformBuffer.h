#pragma once
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuShader.h"
#include <string_view>
#include <type_traits>

namespace FW
{
	struct UniformBufferMemberInfo
	{
		uint32 Offset;
        uint32 Size;
        FString HlslTypeName;
        FString GlslTypeName;
        
        FString GetTypeName(GpuShaderLanguage Language = GpuShaderLanguage::HLSL) const
        {
            return Language == GpuShaderLanguage::GLSL ? GlslTypeName : HlslTypeName;
        }
        
        friend FArchive& operator<<(FArchive& Ar, UniformBufferMemberInfo& Info)
        {
            Ar << Info.Offset << Info.Size << Info.HlslTypeName << Info.GlslTypeName;
            return Ar;
        }
	};

	struct UniformBufferMetaData
	{
		TMap<FString, UniformBufferMemberInfo> Members;
		FString HlslDeclaration;
		FString GlslDeclaration;
		uint32 UniformBufferSize = 0;
		
		FString GetDeclaration(GpuShaderLanguage Language = GpuShaderLanguage::HLSL) const
		{
			return Language == GpuShaderLanguage::GLSL ? GlslDeclaration : HlslDeclaration;
		}
	};

    template<typename T>
    class UniformBufferMemberWrapper
    {
    public:
        UniformBufferMemberWrapper(void* InReadableBackBuffer, GpuBuffer* InWriteCombinedBuffer, uint32 InOffset)
        : ReadableBackBuffer(InReadableBackBuffer), WriteCombinedBuffer(InWriteCombinedBuffer), Offset(InOffset)
        {}
        
        void operator=(const T& InData)
        {
			T* ReadableData = reinterpret_cast<T*>((uint8*)ReadableBackBuffer + Offset);
            *ReadableData = InData;

			T* WritableData = reinterpret_cast<T*>((uint8*)GGpuRhi->MapGpuBuffer(WriteCombinedBuffer, GpuResourceMapMode::Write_Only) + Offset);
            *WritableData = InData;
        }
        
        operator T&()
        {
            return *reinterpret_cast<T*>((uint8*)ReadableBackBuffer + Offset);
        }
        
    private:
        void* ReadableBackBuffer;
		GpuBuffer* WriteCombinedBuffer;
		uint32 Offset;
    };

    template<typename T, size_t N>
    class UniformBufferMemberWrapper<T[N]>
    {
    public:
        UniformBufferMemberWrapper(void* InReadableBackBuffer, GpuBuffer* InWriteCombinedBuffer, uint32 InOffset)
        : ReadableBackBuffer(InReadableBackBuffer), WriteCombinedBuffer(InWriteCombinedBuffer), BaseOffset(InOffset)
        {
            ElementSize = sizeof(T);
            ElementAlignedSize = Align(ElementSize, 16);
        }
        
        T& operator[](size_t Index)
        {
            checkf(Index < N, TEXT("Array index %zu out of bounds [0, %zu)"), Index, N);
            uint32 ElementOffset = BaseOffset + static_cast<uint32>(Index) * ElementAlignedSize;
            return *reinterpret_cast<T*>((uint8*)ReadableBackBuffer + ElementOffset);
        }
        
        void operator=(const T(&InData)[N])
        {
            void* WritableData = GGpuRhi->MapGpuBuffer(WriteCombinedBuffer, GpuResourceMapMode::Write_Only);
            
            for (size_t i = 0; i < N; ++i)
            {
                uint32 ElementOffset = BaseOffset + static_cast<uint32>(i) * ElementAlignedSize;
                T* ReadableData = reinterpret_cast<T*>((uint8*)ReadableBackBuffer + ElementOffset);
                T* WritableElement = reinterpret_cast<T*>((uint8*)WritableData + ElementOffset);
                *ReadableData = InData[i];
                *WritableElement = InData[i];
            }
        }

        void operator=(const TArray<T>& InData)
        {
            checkf(InData.Num() == N, TEXT("Array size mismatch: expected %zu, got %d"), N, InData.Num());
            void* WritableData = GGpuRhi->MapGpuBuffer(WriteCombinedBuffer, GpuResourceMapMode::Write_Only);
            
            for (size_t i = 0; i < N; ++i)
            {
                uint32 ElementOffset = BaseOffset + static_cast<uint32>(i) * ElementAlignedSize;
                T* ReadableData = reinterpret_cast<T*>((uint8*)ReadableBackBuffer + ElementOffset);
                T* WritableElement = reinterpret_cast<T*>((uint8*)WritableData + ElementOffset);
                *ReadableData = InData[i];
                *WritableElement = InData[i];
            }
        }
        
        static constexpr size_t GetCount() { return N; }
        
    private:
        void* ReadableBackBuffer;
        GpuBuffer* WriteCombinedBuffer;
        uint32 BaseOffset;
        uint32 ElementSize;
        uint32 ElementAlignedSize;
    };

	class UniformBuffer
	{
	public:
		UniformBuffer(TRefCountPtr<GpuBuffer> InWriteCombinedBuffer, UniformBufferMetaData InData)
			: WriteCombinedBuffer(MoveTemp(InWriteCombinedBuffer))
			, MetaData(MoveTemp(InData))
		{
            ReadableBackBuffer = FMemory::Malloc(MetaData.UniformBufferSize);
			FMemory::Memset(GetWriteCombinedData(), 0, MetaData.UniformBufferSize);
            FMemory::Memset(ReadableBackBuffer, 0, MetaData.UniformBufferSize);
        }
        
        ~UniformBuffer()
        {
            FMemory::Free(ReadableBackBuffer);
        }

		template<typename T>
        UniformBufferMemberWrapper<T> GetMember(const FString& MemberName) {
			checkf(MetaData.Members.Contains(MemberName), TEXT("The uniform buffer doesn't contain \"%s\" member."), *MemberName);
            uint32 MemberOffset = MetaData.Members[MemberName].Offset;
            
            if constexpr (std::is_array_v<T>)
            {
                const UniformBufferMemberInfo& MemberInfo = MetaData.Members[MemberName];
                FString TypeName = MemberInfo.HlslTypeName;
                int32 ArrayStartIndex = TypeName.Find(TEXT("["));
                checkf(ArrayStartIndex != INDEX_NONE, TEXT("Member \"%s\" is not an array type."), *MemberName);
                
                int32 ArrayEndIndex = TypeName.Find(TEXT("]"), ESearchCase::CaseSensitive, ESearchDir::FromStart, ArrayStartIndex);
                checkf(ArrayEndIndex != INDEX_NONE, TEXT("Invalid array type format for member \"%s\"."), *MemberName);
                
                FString ArraySizeStr = TypeName.Mid(ArrayStartIndex + 1, ArrayEndIndex - ArrayStartIndex - 1);
                uint32 ArrayCount = FCString::Atoi(*ArraySizeStr);
                constexpr size_t ExpectedArraySize = std::extent_v<T>;
                checkf(ArrayCount == ExpectedArraySize, TEXT("Array size mismatch for member \"%s\": expected %zu, got %d"), *MemberName, ExpectedArraySize, ArrayCount);
            }
            
            return {ReadableBackBuffer, WriteCombinedBuffer, MemberOffset };
		}

		FString GetDeclaration(GpuShaderLanguage Language = GpuShaderLanguage::HLSL) const {
			return MetaData.GetDeclaration(Language);
		}

		GpuBuffer* GetGpuResource() const { return WriteCombinedBuffer; }
        const UniformBufferMetaData& GetMetaData() const { return MetaData; }
        uint32 GetSize() const { return MetaData.UniformBufferSize; }
        
        void* GetReadableData() const { return ReadableBackBuffer; }
        void* GetWriteCombinedData() const { 
			return (uint8*)GGpuRhi->MapGpuBuffer(WriteCombinedBuffer, GpuResourceMapMode::Write_Only);
		}
        
        void SetData(const TArray<uint8>& InData)
        {
			check(InData.Num() == MetaData.UniformBufferSize);
            FMemory::Memcpy(ReadableBackBuffer, InData.GetData(), MetaData.UniformBufferSize);
            FMemory::Memcpy(GetWriteCombinedData(), InData.GetData(), MetaData.UniformBufferSize);
        }
        
        bool HasMember(const FString& MemberName)
        {
            return MetaData.Members.Contains(MemberName);
        }
        
        void CopySameMember(const UniformBuffer& Src)
        {
            const auto& SrcMetaData = Src.GetMetaData();
            for(const auto& [MemberName, MemberInfo]: SrcMetaData.Members)
            {
                if(HasMember(MemberName))
                {
                    uint32 MemberSize = MetaData.Members[MemberName].Size;
					FString MemberHlslTypeName = MetaData.Members[MemberName].HlslTypeName;
					if(MemberSize == MemberInfo.Size && MemberHlslTypeName == MemberInfo.HlslTypeName)
					{
						uint32 DstMemberOffset = MetaData.Members[MemberName].Offset;
						void* DstMemberWritableData = (uint8*)GetWriteCombinedData() + DstMemberOffset;
						void* DstMemberReadableData = (uint8*)GetReadableData() + DstMemberOffset;
						
						uint32 SrcMemberOffset = SrcMetaData.Members[MemberName].Offset;
						void* SrcMemberWritableData = (uint8*)Src.GetWriteCombinedData() + SrcMemberOffset;
						void* SrcMemberReadableData = (uint8*)Src.GetReadableData() + SrcMemberOffset;

						FMemory::Memcpy(DstMemberWritableData, SrcMemberWritableData, MemberSize);
						FMemory::Memcpy(DstMemberReadableData, SrcMemberReadableData, MemberSize);
					}
                }
            }
        }

	private:
		TRefCountPtr<GpuBuffer> WriteCombinedBuffer;
        //Avoid directly reading write-combined memory
        void* ReadableBackBuffer;
		UniformBufferMetaData MetaData;
	};

	template<typename T> struct UniformBufferMemberTypeString;
	template<> struct UniformBufferMemberTypeString<int32> { static constexpr std::string_view Value = "int"; static constexpr std::string_view GlslValue = "int"; };
	template<> struct UniformBufferMemberTypeString<uint32> { static constexpr std::string_view Value = "uint"; static constexpr std::string_view GlslValue = "uint"; };
	template<> struct UniformBufferMemberTypeString<float> { static constexpr std::string_view Value = "float"; static constexpr std::string_view GlslValue = "float"; };
	template<> struct UniformBufferMemberTypeString<Vector2f> { static constexpr std::string_view Value = "float2"; static constexpr std::string_view GlslValue = "vec2"; };
	template<> struct UniformBufferMemberTypeString<Vector3f> { static constexpr std::string_view Value = "float3"; static constexpr std::string_view GlslValue = "vec3"; };
	template<> struct UniformBufferMemberTypeString<Vector4f> { static constexpr std::string_view Value = "float4"; static constexpr std::string_view GlslValue = "vec4"; };

	enum class UniformBufferUsage : uint32
	{
		Persistant = (uint32)GpuBufferUsage::Uniform,
		Temp = (uint32)(GpuBufferUsage::Uniform | GpuBufferUsage::Temporary),
	};
	
	//There is no declared struct for uniform buffer,
	//and all uniform buffer is represented by the "UniformBuffer" class, 
	//which makes it easy for us to simulate hlsl alignment rules when generating cbuffer on the shader side.
	class UniformBufferBuilder
	{
	public:
		UniformBufferBuilder(UniformBufferUsage InUsage)
			: Usage(InUsage)
		{
			HlslDeclarationHead = TEXT("cbuffer {0} : register(b{1}, space{2}) {\n");
			HlslDeclarationEnd = TEXT("};\n");

			GlslDeclarationHead = TEXT("layout(std140, binding = {1}, set = {2}) uniform {0} {\n");
			GlslDeclarationEnd = TEXT("};\n");
		}
        
        friend FArchive& operator<<(FArchive& Ar, UniformBufferBuilder& UbBuilder)
        {
            Ar << UbBuilder.Usage;
            Ar << UbBuilder.HlslDeclarationHead << UbBuilder.HlslUniformBufferBody << UbBuilder.HlslDeclarationEnd;
            Ar << UbBuilder.GlslDeclarationHead << UbBuilder.GlslUniformBufferBody << UbBuilder.GlslDeclarationEnd;
            Ar << UbBuilder.MetaData.Members << UbBuilder.MetaData.HlslDeclaration << UbBuilder.MetaData.GlslDeclaration << UbBuilder.MetaData.UniformBufferSize;
            return Ar;
        }

	public:
		UniformBufferBuilder& AddUint(const FString& MemberName)
		{
			AddMember<uint32>(MemberName);
			return *this;
		}

		UniformBufferBuilder& AddInt(const FString& MemberName)
		{
			AddMember<int32>(MemberName);
			return *this;
		}

		UniformBufferBuilder& AddFloat(const FString& MemberName) 
		{
			AddMember<float>(MemberName);
			return *this;
		}

		UniformBufferBuilder& AddVector2f(const FString& MemberName) 
		{
			AddMember<Vector2f>(MemberName);
			return *this;
		}

		UniformBufferBuilder& AddVector3f(const FString& MemberName) 
		{
			AddMember<Vector3f>(MemberName);
			return *this;
		}

		UniformBufferBuilder& AddVector4f(const FString& MemberName)
		{
			AddMember<Vector4f>(MemberName);
			return *this;
		}

		UniformBufferBuilder& AddVector3fArray(const FString& MemberName, uint32 ArrayCount)
		{
			AddArrayMember<Vector3f>(MemberName, ArrayCount);
			return *this;
		}

		FString GetLayoutDeclaration(GpuShaderLanguage Language = GpuShaderLanguage::HLSL) const
		{
			if (Language == GpuShaderLanguage::GLSL)
			{
				return GlslDeclarationHead + GlslUniformBufferBody + GlslDeclarationEnd;
			}
			return HlslDeclarationHead + HlslUniformBufferBody + HlslDeclarationEnd;
		}
        const UniformBufferMetaData& GetMetaData() const { return MetaData; }
        
		TUniquePtr<UniformBuffer> Build() {
            if(MetaData.UniformBufferSize > 0)
            {
				TRefCountPtr<GpuBuffer> Buffer = GGpuRhi->CreateBuffer({ MetaData.UniformBufferSize, (GpuBufferUsage)Usage });
                MetaData.HlslDeclaration = GetLayoutDeclaration(GpuShaderLanguage::HLSL);
                MetaData.GlslDeclaration = GetLayoutDeclaration(GpuShaderLanguage::GLSL);
                return MakeUnique<UniformBuffer>( MoveTemp(Buffer), MetaData);
            }
            return {};
		}
        
        bool HasMember(const FString& MemberName)
        {
            return MetaData.Members.Contains(MemberName);
        }

    private:
		template<typename T>
		void AddMember(const FString& MemberName)
		{
            checkf(!HasMember(MemberName), TEXT("UniformBuffer can not have the same member name:%s."), *MemberName);
			uint32 MemberSize = sizeof(T);
			uint32 SizeBeforeAligning = MetaData.UniformBufferSize;
			uint32 SizeAfterAligning = Align(MetaData.UniformBufferSize, 16);
			uint32 RemainingSize = SizeAfterAligning - SizeBeforeAligning;
			if (RemainingSize < MemberSize)
			{
				while (SizeAfterAligning > SizeBeforeAligning)
				{
					HlslUniformBufferBody += FString::Printf(TEXT("float {0}_Padding_%d;\n"), SizeBeforeAligning);
					GlslUniformBufferBody += FString::Printf(TEXT("float {0}_Padding_%d;\n"), SizeBeforeAligning);
					SizeBeforeAligning += 4;
				}
				MetaData.UniformBufferSize = SizeAfterAligning + MemberSize;
			}
			else
			{
				MetaData.UniformBufferSize += MemberSize;
			}

            FString HlslTypeName = ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::Value.data());
            FString GlslTypeName = ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::GlslValue.data());
			MetaData.Members.Add(MemberName, { MetaData.UniformBufferSize - MemberSize, MemberSize, HlslTypeName, GlslTypeName });

			HlslUniformBufferBody += FString::Printf(TEXT("%s %s;\n"), *HlslTypeName, *MemberName);
			GlslUniformBufferBody += FString::Printf(TEXT("%s %s;\n"), *GlslTypeName, *MemberName);
		}

		template<typename T>
		void AddArrayMember(const FString& MemberName, uint32 ArrayCount)
		{
            checkf(!HasMember(MemberName), TEXT("UniformBuffer can not have the same member name:%s."), *MemberName);
			checkf(ArrayCount > 0, TEXT("Array count must be greater than 0."));

			uint32 ElementSize = sizeof(T);
			uint32 ElementAlignedSize = Align(ElementSize, 16);
			uint32 ArraySize = ArrayCount * ElementAlignedSize;
			
			uint32 SizeBeforeAligning = MetaData.UniformBufferSize;
			uint32 SizeAfterAligning = Align(MetaData.UniformBufferSize, 16);
			uint32 RemainingSize = SizeAfterAligning - SizeBeforeAligning;
			if (RemainingSize < ElementSize)
			{
				while (SizeAfterAligning > SizeBeforeAligning)
				{
					HlslUniformBufferBody += FString::Printf(TEXT("float {0}_Padding_%d;\n"), SizeBeforeAligning);
					GlslUniformBufferBody += FString::Printf(TEXT("float {0}_Padding_%d;\n"), SizeBeforeAligning);
					SizeBeforeAligning += 4;
				}
				MetaData.UniformBufferSize = SizeAfterAligning + ArraySize;
			}
			else
			{
				MetaData.UniformBufferSize = SizeAfterAligning + ArraySize;
			}

            FString HlslElementTypeName = ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::Value.data());
            FString GlslElementTypeName = ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::GlslValue.data());
			FString HlslTypeName = FString::Printf(TEXT("%s[%d]"), *HlslElementTypeName, ArrayCount);
			FString GlslTypeName = FString::Printf(TEXT("%s[%d]"), *GlslElementTypeName, ArrayCount);
			uint32 ArrayOffset = MetaData.UniformBufferSize - ArraySize;
			MetaData.Members.Add(MemberName, { ArrayOffset, ArraySize, HlslTypeName, GlslTypeName });

			HlslUniformBufferBody += FString::Printf(TEXT("%s %s[%d];\n"), *HlslElementTypeName, *MemberName, ArrayCount);
			GlslUniformBufferBody += FString::Printf(TEXT("%s %s[%d];\n"), *GlslElementTypeName, *MemberName, ArrayCount);
		}
		
	private:
		UniformBufferUsage Usage;
		FString HlslDeclarationHead, HlslDeclarationEnd;
		FString GlslDeclarationHead, GlslDeclarationEnd;
		FString HlslUniformBufferBody;
		FString GlslUniformBufferBody;
		UniformBufferMetaData MetaData;
	};
}
