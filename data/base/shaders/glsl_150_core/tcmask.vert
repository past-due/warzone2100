#version 150 core
//#pragma debug(on)

uniform float stretch;
uniform mat4 ModelViewMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform mat4 NormalMatrix;

uniform vec4 lightPosition;

in vec4 vertex;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

out float vertexDistance;
out vec3 normal, lightDir, eyeVec;
out vec2 texCoord;

void main()
{
	vec3 vVertex = (ModelViewMatrix * vertex).xyz;
	vec4 position = vertex;

	// Pass texture coordinates to fragment shader
	texCoord = vertexTexCoord;

	// Lighting -- we pass these to the fragment shader
	normal = (NormalMatrix * vec4(vertexNormal, 0.0)).xyz;
	lightDir = lightPosition.xyz - vVertex;
	eyeVec = -vVertex;

	// Implement building stretching to accommodate terrain
	if (vertex.y <= 0.0) // use vertex here directly to help shader compiler optimization
	{
		position.y -= stretch;
	}

	// Translate every vertex according to the Model View and Projection Matrix
	vec4 gposition = ModelViewProjectionMatrix * position;
	gl_Position = gposition;

	// Remember vertex distance
	vertexDistance = gposition.z;
}
