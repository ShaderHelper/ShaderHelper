//@Binding@

#include "ShaderHelper/Print.glsl"

layout(location = 0) out vec4 outColor;

void mainImage(out vec4 fragColor, in vec2 fragCoord);

vec2 GPrivate_fragCoord;

#if ENABLE_PRINT == 1
#define PrintAtMouse0(StrArrDecl) do {                                                   \
    if (iMouse.z > 0.0 && all(equal(uvec2(iMouse.xy), uvec2(GPrivate_fragCoord))))       \
    {                                                                                    \
        Print0(StrArrDecl);                                                              \
    }                                                                                    \
} while(false)

#define PrintAtMouse1(StrArrDecl, Arg1) do {                                              \
    if (iMouse.z > 0.0 && all(equal(uvec2(iMouse.xy), uvec2(GPrivate_fragCoord))))       \
    {                                                                                    \
        Print1(StrArrDecl, Arg1);                                                        \
    }                                                                                    \
} while(false)

#define PrintAtMouse2(StrArrDecl, Arg1, Arg2) do {                                       \
    if (iMouse.z > 0.0 && all(equal(uvec2(iMouse.xy), uvec2(GPrivate_fragCoord))))       \
    {                                                                                    \
        Print2(StrArrDecl, Arg1, Arg2);                                                  \
    }                                                                                    \
} while(false)

#define PrintAtMouse3(StrArrDecl, Arg1, Arg2, Arg3) do {                                 \
    if (iMouse.z > 0.0 && all(equal(uvec2(iMouse.xy), uvec2(GPrivate_fragCoord))))       \
    {                                                                                    \
        Print3(StrArrDecl, Arg1, Arg2, Arg3);                                            \
    }                                                                                    \
} while(false)
#else
#define PrintAtMouse0(StrArrDecl)
#define PrintAtMouse1(StrArrDecl, Arg1)
#define PrintAtMouse2(StrArrDecl, Arg1, Arg2)
#define PrintAtMouse3(StrArrDecl, Arg1, Arg2, Arg3)
#endif

#define iChannel0 sampler2D(iChannel0_texture, iChannel0_sampler)
#define iChannel1 sampler2D(iChannel1_texture, iChannel1_sampler)
#define iChannel2 sampler2D(iChannel2_texture, iChannel2_sampler)
#define iChannel3 sampler2D(iChannel3_texture, iChannel3_sampler)

void main()
{
	vec2 fragCoord = gl_FragCoord.xy;
	GPrivate_fragCoord = fragCoord;
	vec4 fragColor;
	mainImage(fragColor, fragCoord);
	if (GPrivate_AssertResult != 1)
	{
		fragColor = vec4(1,0,1,1);
	}
	outColor = fragColor;
}

//@mainImage@
