#pragma once

#define JOIN_IMPL(A, B) A##B
#define JOIN(A, B) JOIN_IMPL(A, B)
#define EXPAND(...) __VA_ARGS__

//Note that dont use the same slot in a shader even if declare a different type
#define DECLARE_SHADER_TEXTURE(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(t, RegisterSlot), space2);
#define DECLARE_SHADER_SAMPLER(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(s, RegisterSlot), space2);
#define DECLARE_SHADER_RW_BUFFER(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(u, RegisterSlot), space2);

#define DECLARE_GLOBAL_TEXTURE(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(t, RegisterSlot), space0);
#define DECLARE_GLOBAL_SAMPLER(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(s, RegisterSlot), space0);
#define DECLARE_GLOBAL_RW_BUFFER(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(u, RegisterSlot), space0);

#define DECLARE_PASS_TEXTURE(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(t, RegisterSlot), space1);
#define DECLARE_PASS_SAMPLER(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(s, RegisterSlot), space1);
#define DECLARE_PASS_RW_BUFFER(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(u, RegisterSlot), space1);

template<typename T, typename U>
struct is_same
{
	static const bool value = false;
};
template<typename T, typename U>
struct is_same<T, T>
{
	static const bool value = true;
};

