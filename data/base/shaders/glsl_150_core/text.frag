#version 150 core

uniform vec4 color;
uniform sampler2D theTexture;

in vec2 uv;

out vec4 FragColor;

void main()
{
	vec4 texColour = texture(theTexture, uv) * color.a;
	FragColor = texColour * color;

	// gl_FragData[1] apparently fails to compile for some people, see #4584.
	// GL::SC(Error:High) : 0:12(2): error: array index must be < 1
	//gl_FragData[0] = texColour * color;
	//gl_FragData[1] = texColour;
}
