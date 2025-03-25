#version 450

//out attributes
layout (location = 3) out vec4 fragColor3;

//in
in vec2 vs_texcoord;

//structs
struct Light 
{
	vec3 color;
	vec3 pos;
};
struct Material 
{
	vec3 ambientK;
	vec3 diffuseK;
	vec3 specularK;
	float shininess;
};

//uniforms
uniform sampler2D _PositionTex;
uniform sampler2D _NormalTex;
uniform sampler2D _PrevLightPass;
uniform vec3 _CamPos;

uniform Light _Light;
uniform Material _Material;

//blinn phong function
vec3 blinnphong(vec3 normal, vec3 frag_pos, vec3 lightDirection)
{
	//normalize inputs
	vec3 view_direction = normalize(_CamPos - frag_pos);
	vec3 halfway_dir = normalize(lightDirection + view_direction);

	//dot products
	float ndotL = max(dot(normal, lightDirection), 0.0);
	float ndotH = max(dot(normal, halfway_dir), 0.0);

	//light components
	vec3 diffuse = ndotL * _Material.diffuseK; 
	vec3 specular = pow(ndotH, _Material.shininess * 128.0) * _Material.specularK;

	return (diffuse + specular);
}

void main() 
{
   vec3 positionColor = texture(_PositionTex, vs_texcoord).xyz;
   vec3 normalColor = texture(_NormalTex, vs_texcoord).xyz;
   vec3 prevLightPassColor = texture(_PrevLightPass, vs_texcoord).xyz;

   vec3 lightDir = normalize(_Light.pos - positionColor);

   vec3 lighting = blinnphong(normalColor, positionColor, lightDir);
   vec3 finalLighting = _Light.color * ((_Material.ambientK + lighting));

   fragColor3 = vec4(finalLighting + prevLightPassColor, 1.0);
}