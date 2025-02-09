#version 450

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D texture0; 
uniform float vigStrength;

void main()
{
	vec4 color = texture(texture0, vs_texcoord);
	float distFromCenter = distance(vs_texcoord, vec2(0.5));

	float vignette = smoothstep(1.0, vigStrength, distFromCenter);

	color.rgb *= vignette;	
	FragColor = color;
}