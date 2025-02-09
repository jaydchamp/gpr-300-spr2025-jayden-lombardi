#version 450

//what is coming out of the shader
out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0; 
uniform float noiseStrength;

void main()
{
	float noise = (fract(sin(dot(vs_texcoord, vec2(12.9898, 78.233))) * 43758.5453));
	noise = noise - 0.5;	//remap noise from [0,1] to [-0.5, 0.5]

	vec3 color = texture(texture0, vs_texcoord).rgb;
	vec3 grain = color - (noise * noiseStrength);

	FragColor = vec4(grain, 1.0); 
}