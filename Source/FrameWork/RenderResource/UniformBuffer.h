#pragma once
#include "GpuApi/GpuRhi.h"
#include <string_view>

namespace FW
{
	struct UniformBufferMemberInfo
	{
		uint32 Offset;
        FString TypeName;
        
        friend FArchive& operator<<(FArchive& Ar, UniformBufferMemberInfo& Info)
        {
            Ar << Info.Offset << Info.TypeName;
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
        UniformBufferMemberWrapper(T* InReadableData, T* InWritableData)
        : ReadableData(InReadableData), WritableData(InWritableData)
        {}
        
        void operator=(const T& InData)
        {
            *ReadableData = InData;
            *WritableData = InData;
        }
        
        operator T() const
        {
            return *ReadableData;
        }
        
    private:
        T* ReadableData;
        T* WritableData;
    };

	class UniformBuffer
	{
	public:
		UniformBuffer(TRefCountPtr<GpuBuffer> InBuffer, UniformBufferMetaData InData)
			: Buffer(MoveTemp(InBuffer))
			, MetaData(MoveTemp(InData))
		{
            ReadableBackBuffer = FMemory::Malloc(MetaData.UniformBufferSize);
        }
        
        ~UniformBuffer()
        {
            FMemory::Free(ReadableBackBuffer);
        }

		template<typename T>
        UniformBufferMemberWrapper<T> GetMember(const FString& MemberName) {
			checkf(MetaData.Members.Contains(MemberName), TEXT("The uniform buffer doesn't contain \"%s\" member."), *MemberName);
			int32 MemberOffset = MetaData.Members[MemberName].Offset;
			void* BufferBaseAddr = GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Write_Only);
            T* BufferWritableData = reinterpret_cast<T*>((uint8*)BufferBaseAddr + MemberOffset);
            T* BufferReadableData = reinterpret_cast<T*>((uint8*)ReadableBackBuffer + MemberOffset);
            return {BufferReadableData, BufferWritableData};
		}

		FString GetDeclaration() const {
			return MetaData.UniformBufferDeclaration;
		}

		GpuBuffer* GetGpuResource() const { return Buffer; }

	private:
		TRefCountPtr<GpuBuffer> Buffer;
        //Avoid directly reading write-combined memory
        void* ReadableBackBuffer;
		UniformBufferMetaData MetaData;
	};

	template<typename T> struct UniformBufferMemberTypeString;
	template<> struct UniformBufferMemberTypeString<float> { static constexpr std::string_view Value = "float"; };
	template<> struct UniformBufferMemberTypeString<Vector2f> { static constexpr std::string_view Value = "float2"; };
	template<> struct UniformBufferMemberTypeString<Vector3f> { static constexpr std::string_view Value = "float3"; };
	template<> struct UniformBufferMemberTypeString<Vector4f> { static constexpr std::string_view Value = "float4"; };

	enum class UniformBufferUsage : uint32
	{
		Persistant = (uint32)GpuBufferUsage::PersistentUniform,
		Temp = (uint32)GpuBufferUsage::TemporaryUniform,
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
			DeclarationHead = TEXT("cbuffer {0}\r\n{\r\n");
			DeclarationEnd = TEXT("};\r\n");
		}
        
        friend FArchive& operator<<(FArchive& Ar, UniformBufferBuilder& UbBuilder)
        {
            Ar << UbBuilder.Usage;
            Ar << UbBuilder.DeclarationHead << UbBuilder.UniformBufferMemberNames << UbBuilder.DeclarationEnd;
            Ar << UbBuilder.MetaData.Members << UbBuilder.MetaData.UniformBufferDeclaration << UbBuilder.MetaData.UniformBufferSize;
            return Ar;
        }

	public:
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
			return DeclarationHead + UniformBufferMemberNames + DeclarationEnd;
		}
        const UniformBufferMetaData& GetMetaData() const { return MetaData; }

		TUniquePtr<UniformBuffer> Build() {
			checkf(MetaData.UniformBufferSize > 0, TEXT("Nothing added to the builder."));
			TRefCountPtr<GpuBuffer> Buffer = GGpuRhi->CreateBuffer(MetaData.UniformBufferSize, (GpuBufferUsage)Usage);
			MetaData.UniformBufferDeclaration = GetLayoutDeclaration();
			return MakeUnique<UniformBuffer>( MoveTemp(Buffer), MetaData);
		}

    private:
		template<typename T>
		void AddMember(const FString& MemberName)
		{			
			uint32 MemberSize = sizeof(T);
			uint32 SizeBeforeAligning = MetaData.UniformBufferSize;
			uint32 SizeAfterAligning = Align(MetaData.UniformBufferSize, 16);
			uint32 RemainingSize = SizeAfterAligning - SizeBeforeAligning;
			if (RemainingSize < MemberSize)
			{
				while (SizeAfterAligning > SizeBeforeAligning)
				{
					UniformBufferMemberNames += FString::Printf(TEXT("float Padding_%d;\r\n"), SizeBeforeAligning);
					SizeBeforeAligning += 4;
				}
				MetaData.UniformBufferSize = SizeAfterAligning + MemberSize;
			}
			else
			{
				MetaData.UniformBufferSize += MemberSize;
			}

            FString TypeName = ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::Value.data());
			MetaData.Members.Add(MemberName, { MetaData.UniformBufferSize - MemberSize, TypeName});

			UniformBufferMemberNames += FString::Printf(TEXT("%s %s;\r\n"), *TypeName, *MemberName);
		}
		
	private:
		UniformBufferUsage Usage;
		FString DeclarationHead, DeclarationEnd;
		FString UniformBufferMemberNames;
		UniformBufferMetaData MetaData;
	};
}
