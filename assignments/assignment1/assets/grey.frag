#version 450

//what is coming out of the shader
out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0; 

void main()
{
	vec3 albedo = texture(1.0 - texture0, vs_texcoord).rgb;
	float average = (albedo.r + albedo.g + albedo.b) / 3;
	FragColor = vec4(average, average, average, 1.0); //rgba


	(0.2126 * albedo.r)
	(0.7512 * albedo.g)
	(0.0722 * albedo.b)
}