#pragma once
#include "gui/Window.hpp"
#include "gui/CommonControls.hpp"
#include "3d/CameraControls.hpp"
#include "3d/Texture.hpp"
#include "gpu/Buffer.hpp"
#include "gui/Image.hpp"

#include "ShadowMap.hpp"

namespace FW
{
//------------------------------------------------------------------------

class App : public Window::Listener, public CommonControls::StateObject
{
private:
    enum Action
    {
        Action_None,

        Action_LoadMesh,
        Action_ReloadMesh,
        Action_SaveMesh,

		Action_ReloadShaders,

        Action_ResetCamera,
        Action_EncodeCameraSignature,
        Action_DecodeCameraSignature,
    };

    enum CullMode
    {
        CullMode_None = 0,
        CullMode_CW,
        CullMode_CCW,
    };

    struct RayVertex
    {
        Vec3f       pos;
        U32         color;
    };

public:
                    App             (void);
    virtual         ~App            (void);

    virtual bool    handleEvent     (const Window::Event& ev);
    virtual void    readState       (StateDump& d);
    virtual void    writeState      (StateDump& d) const;

private:
    void            waitKey         (void);
    void            renderFrame     (GLContext* gl);
    void            renderScene     (GLContext* gl, const Mat4f& worldToCamera, const Mat4f& projection);
	void			loadScene		(const String& fileName);
    void            loadMesh        (const String& fileName);
    void            saveMesh        (const String& fileName);

    void            firstTimeInit   (void);

	void			initShader						(void);
	void			renderWithNormalMap				(GLContext* gl, Mat4f cameraToWorld, Mat4f projection);
	static Image	convertBumpToObjectSpaceNormal	(const Image& bump, MeshBase* pMesh, float alpha);

private:
                    App             (const App&); // forbidden
    App&            operator=       (const App&); // forbidden

private:
    Window          m_window;
    CommonControls  m_commonCtrl;
    CameraControls  m_cameraCtrl;

    Action          m_action;
    String          m_meshFileName;
    MeshBase*       m_mesh;
    CullMode        m_cullMode;
    bool            m_wireframe;

	GLContext::Program*	m_shaderProgram;
	S32					m_renderMode;
	bool				m_useNormalMap;
	bool				m_useDiffuseTexture;
	F32					m_normalMapScale;
	bool				m_setDiffuseToZero;
	bool				m_setSpecularToZero;

    String          m_rayDumpFileName;
    S32             m_numRays;
    Buffer          m_rayVertices;

	std::vector<std::pair<LightSource, ShadowMapContext>> m_lights;
	S32					m_viewpoint;
	Timer				m_lamptimer; // You can use this to move the lights
	int					m_shadowMode;
	float				m_shadowEps;
};

//------------------------------------------------------------------------
}
