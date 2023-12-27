#pragma once

#define JOIN_IMPL(A, B) A##B
#define JOIN(A, B) JOIN_IMPL(A, B)

//Note that dont use the same slot in a shader even if declare a different type
#define DECLARE_TEXTURE(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(t, RegisterSlot), space2);

#define DECLARE_SAMPLER(Type, Name, RegisterSlot) \
	Type Name : register(JOIN(s, RegisterSlot), space2);


