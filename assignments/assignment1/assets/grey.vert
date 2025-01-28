#version 450

//Vertex attributes
layout(location = 0) in vec3 in_position;	  //Vertex position in model space
layout(location = 1) in vec2 in_texcoord; //Vertex texture coordinate (UV)

out vec2 vs_texcoord;
out vec3 vs_normal;

void main()
{
	vs_texcoord = in_texcoord;
	gl_Position = vec4(in_position.xy, 0.0, 1.0);
}