#version 450
in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_texcoord;

out vec4 FragColor0;
out vec4 FragColor1;
out vec4 FragColor2;

void main()
{
	FragColor0 = vec4(vs_normal, 1.0);
}
