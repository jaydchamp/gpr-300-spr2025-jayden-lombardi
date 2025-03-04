#version 450
in vec2 in_position;
in vec2 in_texcoord;

out vec2 vs_texcoord;

void main()
{
	vs_texcoord = in_texcoord;
	gl_Position = vec4(in_position.xy, 0.0, 1.0);
}