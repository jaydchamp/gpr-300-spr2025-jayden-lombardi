#version 450

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D texture0; 

void main()
{
	vec3 hdr = texture(texture0, vs_texcoord).rgb;
	FragColor = vec4(hdr, 1.0);
}