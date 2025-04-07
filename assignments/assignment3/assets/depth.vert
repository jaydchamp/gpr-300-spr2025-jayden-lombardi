#version 450

layout(location = 0) in vec3 vPosition;

uniform mat4 LightSpaceMatrix;
uniform mat4 Model;

void main()
{
    // Transform vertex to light space
    gl_Position = LightSpaceMatrix * Model * vec4(vPosition, 1.0);
}