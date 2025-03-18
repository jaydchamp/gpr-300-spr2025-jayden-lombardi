#version 450

out vec4 FragColor; 

in Surface
{
	vec3 WorldPosition;
	vec3 WorldNormal;
	vec2 TextCoord;
}fs_in;

uniform sampler2D _MainTexture; //2d texture sampler
uniform vec3 _LightDirection = vec3(0.0, -1.0f, 0.0);	//light pointing straight down
uniform vec3 _LightColor	 = vec3(1.0);				//a white light
uniform vec3 _EyePos;
uniform vec3 _AmbientColor = vec3(0.3,0.4,0.46);

struct Material
{
	float Ka;		 //Ambient	(0-1)
	float Kd;		 //Diffuse  (0-1)
	float Ks;		 //Specular (0-1)
	float Shininess; //specular highlight
}; 
uniform Material _Material;

void main()
{
	vec3 normal = normalize(fs_in.WorldNormal);	
	vec3 toLight = -_LightDirection;
	float diffuseFactor = max(dot(normal, toLight), 0.0); 

	vec3 toEye = normalize(_EyePos - fs_in.WorldPosition); //direction towards player's eyes
	vec3 h = normalize(toLight + toEye); //blinn phong uses half angle
	float specularFactor = pow(max(dot(normal, h), 0.0), _Material.Shininess);
	vec3 lightColor = (_Material.Kd * diffuseFactor + _Material.Ks * specularFactor) * _LightColor;

	lightColor += _AmbientColor * _Material.Ka;

	vec3 objectColor = texture(_MainTexture, fs_in.TextCoord).rgb;
	FragColor = vec4(objectColor * lightColor, 1.0);
}
