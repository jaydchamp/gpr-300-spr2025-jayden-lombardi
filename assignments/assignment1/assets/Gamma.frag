#version 450

in vec2 vs_texcoord;
out vec4 FragColor;

uniform sampler2D texture0; 

const float gamma = 2.2;

void main()
{
    vec3 color = texture(texture0, vs_texcoord).rgb;

    //applying ONLY gamma correction
    //in HDR FRAGMENT, tone mapping AND gamma correction added
    color = pow(color, vec3(1.0 / gamma)); 

    FragColor = vec4(color, 1.0);
}