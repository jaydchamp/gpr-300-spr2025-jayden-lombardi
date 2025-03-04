#version 450
in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

out vec4 FragColor;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_color;

void main()
{
	vec3 color = texture(g_position, vs_texcoord).rgb;
	FragColor = vec4(color, 1.0);
}