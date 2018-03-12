#version 150 core

uniform mat4 ModelViewProjectionMatrix;

in vec4 vertex;

void main()
{
	gl_Position = ModelViewProjectionMatrix * vertex;
}
