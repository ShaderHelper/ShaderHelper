//@Binding@

#include "Shared/Print.h"

struct PIn
{
	float4 Pos : SV_Position;
};

void mainImage(out float4 fragColor, in float2 fragCoord);

#if ENABLE_PRINT == 1
#define PrintAtMouse(Str, ...) do {                                             \
	if(iMouse.z > 0 && all(uint2(iMouse.xy) == uint2(GPrivate_fragCoord)))      \
	{                                                                           \
		Print(EXPAND(Str), ##__VA_ARGS__);                                      \
	}                                                                           \
} while(0)
#elif EDITOR_ISENSE == 1
#define PrintAtMouse(Str, ...) UNUSED_ARGS(__VA_ARGS__);
#else
#define PrintAtMouse(Str, ...)
#endif

static float2 GPrivate_fragCoord;

float4 MainPS(PIn Input) : SV_Target
{
	float2 fragCoord = float2(Input.Pos.x, Input.Pos.y);
	GPrivate_fragCoord = fragCoord;
	float4 fragColor;
	mainImage(fragColor, fragCoord);
	if (GPrivate_AssertResult != 1)
	{
		fragColor = float4(1,0,1,1);
	}
	return fragColor;
}

//@mainImage@
