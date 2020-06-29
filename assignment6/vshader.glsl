#version 330
uniform mat4 posToClip;
uniform mat4 posToCamera;
in vec3 positionAttrib;
in vec3 normalAttrib;
in vec4 vcolorAttrib; // Workaround. "colorAttrib" appears to confuse certain ATI drivers.
in vec2 texCoordAttrib;
out vec3 positionVarying;
out vec3 normalVarying;
out vec4 colorVarying;
out vec3 tangentVarying;
out vec2 texCoordVarying;

// Shadow maps
// Three lights max, change if needed
out float lightDist[3]; // Distance to light
out vec2 shadowUV[3]; // Coordinate in the lamp's framebuffer
uniform mat4 posToLightClip[3]; // World -> light clip transform
uniform int renderFromLight; // Debug hax

void main()
{
	vec4 pos = vec4(positionAttrib, 1.0);
	gl_Position = posToClip * pos;
	positionVarying = (posToCamera * pos).xyz;
	normalVarying = normalAttrib;
	colorVarying = vcolorAttrib;
	texCoordVarying = texCoordAttrib;
	tangentVarying = vec3(1.0,0.0,0.0) - normalAttrib.x*normalAttrib;

	for (int i = 0; i < 3; i++) {
		// YOUR SHADOWS HERE: fill these two
		lightDist[i] = 99.0;
		shadowUV[i] = vec2(.0, .0);
	}

	// Quick 'n dirty debug view
	if (renderFromLight > 0)
		gl_Position = posToLightClip[renderFromLight - 1] * pos;
}
