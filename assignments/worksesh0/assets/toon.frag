#version 450

//what is coming out of the shader
out vec4 FragColor; //The color of this fragment

//in vec3 Normal; //Interpolated of this fragment  WE SWITHCED THIS FOR::
in Surface
{
	//vec3 Normal; ADDED FOR DIFFUSE
	vec3 WorldPosition;
	vec3 WorldNormal;
	vec2 TextCoord;
}fs_in;

//then we added this:
uniform sampler2D _MainTexture; //2d texture sampler
uniform sampler2D albedo; //2d texture sampler
uniform sampler2D zatoon; //2d texture sampler

//ADDED FOR DIFFUSE::
uniform vec3 _LightDirection = vec3(0.0, -1.0f, 0.0);	//light pointing straight down
uniform vec3 _LightColor	 = vec3(1.0);				//a white light

//ADDED FOR MATERIAL CONTROLS:
struct Material
{
	float Ka;		 //Ambient	(0-1)
	float Kd;		 //Diffuse  (0-1)
	float Ks;		 //Specular (0-1)
	float Shininess; //specular highlight
}; 
uniform Material _Material;

struct Palette
{
	vec3 highlight;
	vec3 shadow;
};
uniform Palette _Palette;

vec3 toonLighting(vec3 normal, vec3 light_direction)
{
	//this num needs to be 0-1:
	float diff = (dot(normal, light_direction) + 1.0) * 0.5;
	//vec3 light_color = vec3(1.0) * diff;

	//determining shadow step function for zatoon
	float step = texture(zatoon, vec2(diff)).r;

	vec3 light_color = mix(_Palette.shadow, _Palette.highlight, step);
	return light_color * step;
}

void main()
{
	//ADDED FOR DIFFUSE
	vec3 normal = normalize(fs_in.WorldNormal);	//normalize normal bc of different screen sizes/ratios

	vec3 toLight = -_LightDirection;
	vec3 tolightNormal = normalize(toLight);

	vec3 light_color = toonLighting(normal, tolightNormal);
	vec3 obj_color = texture(albedo, fs_in.TextCoord).rgb;
	FragColor = vec4(obj_color * light_color, 1.0);
}
