#version 450

//Vertex attributes
layout(location = 0) in vec3 vPos;	  
layout(location = 1) in vec3 vNormal; 
layout(location = 2) in vec2 vTextureCoord; 

out Surface
{
	vec3 WorldPosition;	
	vec3 WorldNormal;	
	vec2 TextCoord;
}vs_out;

uniform mat4 _Model; 
uniform mat4 _ViewProjection; 

void main()
{
	vs_out.WorldPosition = vec3(_Model * vec4(vPos, 1.0));
	vs_out.WorldNormal = transpose(inverse(mat3(_Model))) * vNormal;

	vs_out.TextCoord = vTextureCoord;
	gl_Position = _ViewProjection * _Model * vec4(vPos, 1.0);
}

//	gl_Position = vec4(in_position.xy, 0.0, 1.0);
//	vs_texcoord = in_texcoord;