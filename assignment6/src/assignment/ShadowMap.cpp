#include "ShadowMap.hpp"

namespace FW 
{

Mat4f LightSource::getPosToLightClip() const 
{
	// YOUR SHADOWS HERE
	// You'll need to construct the matrix that sends world space points to
	// points in the light source's clip space. You'll need to somehow use
	// m_xform, which describes the light source pose, and Mat4f::perspective().
	return Mat4f(); // placeholder
}

void LightSource::renderShadowMap(FW::GLContext *gl, MeshBase *scene, ShadowMapContext *sm)
{
	// Get or build the shader that outputs depth values
    static const char* progId = "ShadowMap::renderDepthTexture";
    GLContext::Program* prog = gl->getProgram(progId);
	if (!prog)
	{
		printf("Creating shadow map program\n");

		prog = new GLContext::Program(
			FW_GL_SHADER_SOURCE(
			
				// Have a look at the comments in the main shader in the renderShadowedScene() function in case
				// you are unfamiliar with GLSL shaders.

				// This is the shader that will render the scene from the light source's point of view, and
				// output the depth value at every pixel.

				// VERTEX SHADER

				// The posToLightClip uniform will be used to transform the world coordinate points of the scene.
				// They should end up in the clip coordinates of the light source "camera". If you look below,
				// you'll see that its value is set based on the function getWorldToLightClip(), which you'll
				// need to implement.
				uniform mat4	posToLightClip;
				uniform mat4	posToLight;
				attribute vec3	positionAttrib;
				varying float depthVarying;

				void main()
				{
					// Here we perform the transformation from world to light clip space.
					// If the posToLightClip is set correctly (see the corresponding setUniform()
					// call below), the scene will be viewed from the light.
					vec4 pos = vec4(positionAttrib, 1.0);
					gl_Position = posToLightClip * pos;
					// YOUR SHADOWS HERE: compute the depth of this vertex and assign it to depthVarying. Note that
					// there are various alternative "depths" you can store: there is at least (A) the z-coordinate in camera
					// space, (B) z-coordinate in clip space divided by the w-coordinate.
					// All of these work, but you must use the same convention also in the main shader that actually
					// reads from the depth map. We used the option B in the example solution.


					depthVarying = gl_Position.z / gl_Position.w;

				}
			),
			FW_GL_SHADER_SOURCE(
				// FRAGMENT SHADER
				varying float depthVarying;
				void main()
				{
					// YOUR SHADOWS HERE
					// Just output the depth value you computed in the vertex shader. It'll go into the shadow
					// map texture.
					gl_FragColor = vec4(depthVarying);
				}
			));

		gl->setProgram(progId, prog);
	}
    prog->use();

	// If this light source does not yet have an allocated OpenGL texture, then get one
	if (m_shadowMapTexture == 0)
		m_shadowMapTexture = sm->allocateDepthTexture();

	// Attach the shadow rendering state (off-screen buffers, render target texture)
	sm->attach(m_shadowMapTexture);
	// Here's a trick: we can cull the front faces instead of the backfaces; this
	// moves the shadow map bias nastiness problems to the dark side of the objects
	// where they might not be visible.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	
	// Clear the texture to maximum distance (1.0 in NDC coordinates)
	glClearColor(1.0f, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);	GLContext::checkErrors();


	// Set the transformation matrix uniform
	gl->setUniform(prog->getUniformLoc("posToLightClip"), getPosToLightClip());
	gl->setUniform(prog->getUniformLoc("posToLight"), m_xform.inverted());
	GLContext::checkErrors();
	renderSceneRaw(gl, scene, prog);

	GLContext::checkErrors();

	// Detach off-screen buffers
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



/*************************************************************************************
 * Nasty OpenGL stuff begins here, you should not need to touch the remainder of this
 * file for the basic requirements.
 *
 */

bool ShadowMapContext::setup(Vec2i resolution)
{
	m_resolution = resolution;

	printf("Setting up ShadowMapContext... ");
	free();	// Delete the existing stuff, if any
	
	glGenRenderbuffers(1, &m_depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_resolution.x, m_resolution.y);

	glGenFramebuffers(1, &m_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
	
	GLContext::checkErrors();

	// We now have a framebuffer object with a renderbuffer attachment for depth.
	// We'll render the actual depth maps into separate textures, which are attached
	// to the color buffers upon rendering.

	// Don't attach yet.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	printf("done\n");

	return true;
}

GLuint ShadowMapContext::allocateDepthTexture()
{
	printf("Allocating depth texture... ");

	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE32F_ARB, m_resolution.x, m_resolution.y,	0, GL_LUMINANCE, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_resolution.x, m_resolution.y,	0, GL_LUMINANCE, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	printf("done, %i\n", tex);
	return tex;
}


void ShadowMapContext::attach(GLuint texture)
{
	if (m_framebuffer == 0)
	{
		printf("Error: ShadowMapContext not initialized\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glViewport(0, 0, m_resolution.x, m_resolution.y);
}



void LightSource::renderSceneRaw(FW::GLContext *gl, MeshBase *scene, GLContext::Program *prog)
{
	// Just draw the triangles without doing anything fancy; anything else should be done on the caller's side

	int posAttrib = scene->findAttrib(MeshBase::AttribType_Position);
	if (posAttrib == -1)
		return;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->getVBO().getGLBuffer());
	scene->setGLAttrib(gl, posAttrib, prog->getAttribLoc("positionAttrib"));

	for (int i = 0; i < scene->numSubmeshes(); i++)
	{
		glDrawElements(GL_TRIANGLES, scene->vboIndexSize(i), GL_UNSIGNED_INT, (void*)(UPTR)scene->vboIndexOffset(i));
	}

	gl->resetAttribs();
}

}
