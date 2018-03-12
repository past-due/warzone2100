#version 120

uniform vec4 color;
uniform sampler2D theTexture;

varying vec2 uv;

void main()
{
	gl_FragColor = texture2D(theTexture, uv) * color;
}
