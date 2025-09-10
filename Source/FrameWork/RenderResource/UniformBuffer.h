#pragma once
#include "GpuApi/GpuRhi.h"
#include <string_view>

namespace FW
{
	struct UniformBufferMemberInfo
	{
		uint32 Offset;
        uint32 Size;
        FString TypeName;
        
        friend FArchive& operator<<(FArchive& Ar, UniformBufferMemberInfo& Info)
        {
            Ar << Info.Offset << Info.Size << Info.TypeName;
            return Ar;
        }
	};

	struct UniformBufferMetaData
	{
		TMap<FString, UniformBufferMemberInfo> Members;
		FString UniformBufferDeclaration;
		uint32 UniformBufferSize = 0;
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
            return {ReadableBackBuffer, WriteCombinedBuffer, MemberOffset };
		}

		FString GetDeclaration() const {
			return MetaData.UniformBufferDeclaration;
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
					FString MemberTypeName = MetaData.Members[MemberName].TypeName;
					if(MemberSize == MemberInfo.Size && MemberTypeName == MemberInfo.TypeName)
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
	template<> struct UniformBufferMemberTypeString<uint32> { static constexpr std::string_view Value = "uint"; };
	template<> struct UniformBufferMemberTypeString<float> { static constexpr std::string_view Value = "float"; };
	template<> struct UniformBufferMemberTypeString<Vector2f> { static constexpr std::string_view Value = "float2"; };
	template<> struct UniformBufferMemberTypeString<Vector3f> { static constexpr std::string_view Value = "float3"; };
	template<> struct UniformBufferMemberTypeString<Vector4f> { static constexpr std::string_view Value = "float4"; };

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
			DeclarationHead = TEXT("cbuffer {0} {\n");
			DeclarationEnd = TEXT("};\n");
		}
        
        friend FArchive& operator<<(FArchive& Ar, UniformBufferBuilder& UbBuilder)
        {
            Ar << UbBuilder.Usage;
            Ar << UbBuilder.DeclarationHead << UbBuilder.UniformBufferBody << UbBuilder.DeclarationEnd;
            Ar << UbBuilder.MetaData.Members << UbBuilder.MetaData.UniformBufferDeclaration << UbBuilder.MetaData.UniformBufferSize;
            return Ar;
        }

	public:
		UniformBufferBuilder& AddUint(const FString& MemberName)
		{
			AddMember<uint32>(MemberName);
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

		FString GetLayoutDeclaration() const
		{
			return DeclarationHead + UniformBufferBody + DeclarationEnd;
		}
        const UniformBufferMetaData& GetMetaData() const { return MetaData; }
        
		TUniquePtr<UniformBuffer> Build() {
            if(MetaData.UniformBufferSize > 0)
            {
				TRefCountPtr<GpuBuffer> Buffer = GGpuRhi->CreateBuffer({ MetaData.UniformBufferSize, (GpuBufferUsage)Usage });
                MetaData.UniformBufferDeclaration = GetLayoutDeclaration();
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
					UniformBufferBody += FString::Printf(TEXT("float {0}_Padding_%d;\n"), SizeBeforeAligning);
					SizeBeforeAligning += 4;
				}
				MetaData.UniformBufferSize = SizeAfterAligning + MemberSize;
			}
			else
			{
				MetaData.UniformBufferSize += MemberSize;
			}

            FString TypeName = ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::Value.data());
			MetaData.Members.Add(MemberName, { MetaData.UniformBufferSize - MemberSize, MemberSize, TypeName});

			UniformBufferBody += FString::Printf(TEXT("%s %s;\n"), *TypeName, *MemberName);
		}
		
	private:
		UniformBufferUsage Usage;
		FString DeclarationHead, DeclarationEnd;
		FString UniformBufferBody;
		UniformBufferMetaData MetaData;
	};
}
