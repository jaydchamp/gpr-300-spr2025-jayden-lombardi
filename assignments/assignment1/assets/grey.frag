#version 450

//what is coming out of the shader
out vec4 FragColor;
in vec2 vs_texcoord;

uniform sampler2D texture0; 

void main()
{
	vec3 albedo = texture(texture0, vs_texcoord).rgb;

	//convert color to greyscale with luminance
	//float average = ((0.2126 * albedo.r) + (0.7512 * albedo.g) + (0.0722 * albedo.b)) / 3;
	float average = dot(albedo, vec3(0.2126, 0.7512, 0.0722));

	FragColor = vec4(average, average, average, 1.0); //rgba	
}