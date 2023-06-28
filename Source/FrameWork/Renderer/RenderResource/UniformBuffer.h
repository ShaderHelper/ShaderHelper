#pragma once
#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include <string_view>

namespace FRAMEWORK
{
	struct UniformBufferMember
	{
		uint32 Offset;
	};

	struct UniformBufferMetaData
	{
		TMap<FString, UniformBufferMember> Members;
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

		template<typename T>
		T& GetMember(const FString& MemberName) {
			int32 MemberOffset = MetaData.Members[MemberName].Offset;
			return *reinterpret_cast<T*>(Buffer->GetMappedData());
		}

	private:
		TRefCountPtr<GpuBuffer> Buffer;
		UniformBufferMetaData MetaData;
	};

	template<typename T> struct UniformBufferMemberTypeString;
	template<> struct UniformBufferMemberTypeString<float> { static constexpr std::string_view Value = "float"; };
	template<> struct UniformBufferMemberTypeString<Vector2f> { static constexpr std::string_view Value = "float2"; };
	template<> struct UniformBufferMemberTypeString<Vector3f> { static constexpr std::string_view Value = "float3"; };
	template<> struct UniformBufferMemberTypeString<Vector4f> { static constexpr std::string_view Value = "float4"; };
	
	//For minimizing bandwidth, there is no declared struct for uniform buffer,
	//and all uniform buffer is represented by the "UniformBuffer" class, 
	//which makes it easy for us to simulate hlsl alignment rules when generating cbuffer on the shader side.
	class UniformBufferBuilder
	{
	public:
		UniformBufferBuilder(FString InUniformBufferName)
			: UniformBufferName(MoveTemp(InUniformBufferName))
		{
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

		operator TUniquePtr<UniformBuffer>() {
			TRefCountPtr<GpuBuffer> Buffer = GpuApi::CreateBuffer(MetaData.UniformBufferSize, GpuBufferUsage::Uniform);
			MetaData.UniformBufferDeclaration = FString::Printf(TEXT("cbuffer %s\r\n{\r\n%s\r\n};\r\n"), *UniformBufferName, *UniformBufferMemberNames);
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
					UniformBufferMemberNames += FString::Printf(TEXT("float Padding_%d\r\n"), SizeBeforeAligning);
					SizeBeforeAligning += 4;
				}
				MetaData.UniformBufferSize = SizeAfterAligning + MemberSize;
			}
			else
			{
				MetaData.UniformBufferSize += MemberSize;
			}
			UniformBufferMemberNames += FString::Printf(TEXT("%s %s\r\n"), ANSI_TO_TCHAR(UniformBufferMemberTypeString<T>::Value.data()), *MemberName);
			
			MetaData.Members.Add(MemberName, { MetaData.UniformBufferSize });
		}
		
	private:
		FString UniformBufferName;
		FString UniformBufferMemberNames;
		UniformBufferMetaData MetaData;
	};
}
