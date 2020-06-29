#pragma once

#include <base/Main.hpp>
#include <io/StateDump.hpp>

#include <base/Math.hpp>
#include <base/Random.hpp>
#include <3d/Mesh.hpp>

#include <vector>

namespace FW
{

class GLContext;

// This is a technical thing that holds the off-screen buffers related
// to shadow map rendering.
class ShadowMapContext
{
public:
	ShadowMapContext() :
	  m_depthRenderbuffer(0),
	  m_framebuffer(0),
	  m_resolution(256, 256)
		{ }

	~ShadowMapContext() { free(); }

	bool setup(Vec2i resolution);
	void free() {};

	GLuint allocateDepthTexture();
	void attach(GLuint texture);
	bool ok() const { 
		return m_framebuffer != 0; 
	}

protected:
	GLuint m_depthRenderbuffer;
	GLuint m_framebuffer;
	Vec2i m_resolution;
};

class LightSource
{
public:
	LightSource() : 
	  m_fov(45.0f), 
	  m_near(0.01f), 
	  m_far(100.0f),
	  m_shadowMapTexture(0),
	  m_enabled(true)
	  { }

	void			setStaticPosition(const Vec3f& p) { m_staticPosition = p; setPosition(p); }
	Vec3f			getStaticPosition(void)			{ return m_staticPosition; }

	Vec3f			getPosition(void) const			{ return Vec4f(m_xform.getCol(3)).getXYZ(); }
	void			setPosition(const Vec3f& p)		{ m_xform.setCol(3, Vec4f(p, 1.0f)); }

	Mat3f			getOrientation(void) const		{ return m_xform.getXYZ(); }
	void			setOrientation(const Mat3f& R)	{ m_xform.setCol(0,Vec4f(R.getCol(0),0.0f)); m_xform.setCol(1,Vec4f(R.getCol(1),0.0f)); m_xform.setCol(2,Vec4f(R.getCol(2),0.0f)); }

	Vec3f			getNormal(void) const			{ return -Vec4f(m_xform.getCol(2)).getXYZ(); }

	float			getFOV(void) const				{ return m_fov; }
	float			getFOVRad(void) const			{ return m_fov * 3.1415926f / 180.0f; }
	void			setFOV(float fov)				{ m_fov = fov; }

	float			getFar(void) const				{ return m_far; }
	void			setFar(float f)					{ m_far = f; }

	float			getNear(void) const				{ return m_near; }
	void			setNear(float n)				{ m_near = n; }

	void			renderShadowMap(FW::GLContext *gl, MeshBase *scene, ShadowMapContext *sm);

	Mat4f			getPosToLightClip() const;

	void			setEnabled(bool e) { m_enabled = e; };
	bool			isEnabled() const { return m_enabled; }

	Vec3f			m_color = 1;

	// OpenGL stuff:
	GLuint			getShadowTextureHandle() const { return m_shadowMapTexture; }
	void			freeShadowMap() { glDeleteTextures(1, &m_shadowMapTexture); m_shadowMapTexture = 0; }
protected:
	static void		renderSceneRaw(FW::GLContext *gl, MeshBase *scene, GLContext::Program *prog);

	Vec3f   m_staticPosition; // position dictated by scene file, stored separately so that animation doesn't overwrite position data
	Mat4f	m_xform;	// my position and orientation in world space
	float	m_fov;		// field of view in degrees
	float	m_near;
	float	m_far;

	bool	m_enabled;
	GLuint	m_shadowMapTexture;

};

}
