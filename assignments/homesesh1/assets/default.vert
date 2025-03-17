#version 450

//in attributes
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

//out
out vec3 vs_world_pos;
out vec3 vs_world_normal;
out vec2 vs_texcoord;

//uniforms
uniform mat4 _Model;
uniform mat4 _ViewProj;

void main()
{
	vs_world_pos = vec3(_Model * vec4(in_position,1.0));
	vs_world_normal = transpose(inverse(mat3(_Model))) * in_normal;
	vs_texcoord = in_texcoord;

	gl_Position = _ViewProj * _Model * vec4(in_position, 1.0);
}