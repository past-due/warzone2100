#version 150 core

// gl_VertexID seems to not be supported on 120, despite documentation to the contrary.

// Old comment:
// This shader uses vertex id to generate
// vertex position. This allows to save
// a vertex buffer binding and thus
// simplifies C++ code.

uniform mat4 transformationMatrix;
uniform vec2 tuv_offset;
uniform vec2 tuv_scale;

in vec4 vertex;

out vec2 uv;


void main()
{
	gl_Position = transformationMatrix * vertex;
	uv = tuv_scale * vertex.xy + tuv_offset;
}
