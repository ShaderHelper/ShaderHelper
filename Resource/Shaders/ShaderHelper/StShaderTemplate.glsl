//@Binding@

layout(location = 0) out vec4 outColor;

void mainImage(out vec4 fragColor, in vec2 fragCoord);

vec2 GPrivate_fragCoord;

void MainPS()
{
	vec2 fragCoord = gl_FragCoord.xy;
	GPrivate_fragCoord = fragCoord;
	vec4 fragColor;
	mainImage(fragColor, fragCoord);
	outColor = fragColor;
}

//@mainImage@
