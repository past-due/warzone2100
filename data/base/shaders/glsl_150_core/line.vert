#version 150 core

uniform vec2 from;
uniform vec2 to;
uniform mat4 ModelViewProjectionMatrix;

in vec4 vertex;


void main()
{
	vec4 pos = vec4(from + (to - from)*vertex.y, 0.0, 1.0);
	gl_Position = ModelViewProjectionMatrix * pos;
}
