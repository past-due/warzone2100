#version 150 core
//#pragma debug(on)

uniform mat4 ModelViewProjectionMatrix;

in vec4 vertex;
in vec2 vertexTexCoord;

out vec2 texCoord;

void main()
{
	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Translate every vertex according to the Model, View and Projection matrices
	gl_Position = ModelViewProjectionMatrix * vertex;
}
