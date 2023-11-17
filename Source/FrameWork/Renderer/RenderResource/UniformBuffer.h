#pragma once
#include "GpuApi/GpuApiInterface.h"
#include <string_view>

namespace FRAMEWORK
{
	struct UniformBufferMemberInfo
	{
		uint32 Offset;
#if !SH_SHIPPING
		FString TypeName;
#endif
	};

	struct UniformBufferMetaData
	{
		FString UniformBufferName;
		TMap<FString, UniformBufferMemberInfo> Members;
		FString UniformBufferDeclaration;
		uint32 UniformBufferSize = 0;
	};

	class UniformBuffer
	{
	public:
		UniformBuffer(TRefCountPtr<GpuBuffer> InBuffer, UniformBufferMetaData InData)
			: Buffer(MoveTemp(InBuffer))
			, MetaData(MoveTemp(InData))
		{}

		//Only write data, not read.
		template<typename T>
		T& GetMember(const FString& MemberName) {
			checkf(MetaData.Members.Contains(MemberName), TEXT("The uniform buffer doesn't contain \"%s\" member."), *MemberName);
			checkf(AUX::TTypename<T>::Value == MetaData.Members[MemberName].TypeName, TEXT("Mismatched type: %s, Expected : %s"), *AUX::TTypename<T>::Value, *MetaData.Members[MemberName].TypeName);
			int32 MemberOffset = MetaData.Members[MemberName].Offset;
			void* BufferBaseAddr = GpuApi::MapGpuBuffer(Buffer, GpuResourceMapMode::Write_Only);
			return *reinterpret_cast<T*>((uint8*)BufferBaseAddr + MemberOffset);
		}

		const UniformBufferMetaData& GetMetaData() const {
			return MetaData;
		}

		GpuBuffer* GetGpuResource() const { return Buffer; }

	private:
		TRefCountPtr<GpuBuffer> Buffer;
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
		UniformBufferBuilder(FString InUniformBufferName, UniformBufferUsage InUsage)
			: Usage(InUsage)
		{
			MetaData.UniformBufferName = MoveTemp(InUniformBufferName);
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

		auto Build() {
			checkf(MetaData.UniformBufferSize > 0, TEXT("Nothing added to the builder."));
			TRefCountPtr<GpuBuffer> Buffer = GpuApi::CreateBuffer(MetaData.UniformBufferSize, (GpuBufferUsage)Usage);
			MetaData.UniformBufferDeclaration = FString::Printf(TEXT("cbuffer %s\r\n{\r\n%s\r\n};\r\n"), *MetaData.UniformBufferName, *UniformBufferMemberNames);
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

#if !SH_SHIPPING
			MetaData.Members.Add(MemberName, { MetaData.UniformBufferSize - MemberSize, AUX::TTypename<T>::Value });
#else
			MetaData.Members.Add(MemberName, { MetaData.UniformBufferSize - MemberSize });
#endif

			UniformBufferMemberNames += FString::Printf(TEXT("%s %s;\r\n"), ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::Value.data()), *MemberName);
		}
		
	private:
		UniformBufferUsage Usage;
		FString UniformBufferMemberNames;
		UniformBufferMetaData MetaData;
	};
}
