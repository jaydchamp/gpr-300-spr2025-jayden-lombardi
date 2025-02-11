#version 450

//Vertex attributes
layout(location = 0) in vec3 vPos;			//Vertex position in model space
layout(location = 1) in vec3 vNormal;		//Vertex position in model space
layout(location = 2) in vec2 vTextureCoord; //Vertex texture coordinate (UV)

uniform mat4 _Model;						//Model->World Matrix
uniform mat4 _ViewProjection;				//Combined View->Projection Matrix
uniform vec3 camera_pos;
uniform float time;
uniform float uniform_strength;
uniform float scale;

out vec3 to_camera;
out vec2 vs_texcoords;

float calculateSurface(float x, float z) 
{
  //float scale = 10.0;
  float y = 0.0;
  y += (sin(x * 1.0 / scale + time * 1.0) + sin(x * 2.3 / scale + time * 1.5) + sin(x * 3.3 / scale + time * 0.4)) / 3.0;
  y += (sin(z * 0.2 / scale + time * 1.8) + sin(z * 1.8 / scale + time * 1.8) + sin(z * 2.8 / scale + time * 0.8)) / 3.0;
  return y;
}

void main()
{
	vec3 position = vPos;
	position += calculateSurface(position.x, position.z) * uniform_strength;
	position += calculateSurface(0.0, 0.0) * uniform_strength;

	vec4 world_pos = _Model * vec4(position, 1.0);
	to_camera = camera_pos - world_pos.xyz;

	vs_texcoords = vTextureCoord;
	gl_Position = _ViewProjection * world_pos;
}
