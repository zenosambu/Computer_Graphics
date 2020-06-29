#include "App.hpp"
#include "base/Main.hpp"
#include "gpu/GLContext.hpp"
#include "3d/Mesh.hpp"
#include "3d/Texture.hpp"
#include "io/File.hpp"
#include "io/StateDump.hpp"
#include "base/Random.hpp"

#include <stdio.h>
#include <conio.h>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace FW;

//------------------------------------------------------------------------

static Mat3f getOrientation(Vec3f m_forward, Vec3f m_up) {
    Mat3f r;
    r.col(2) = -m_forward.normalized();
    r.col(0) = m_up.cross(r.col(2)).normalized();
    r.col(1) = ((Vec3f)r.col(2)).cross(r.col(0)).normalized();
    return r;
}

App::App(void)
:   m_commonCtrl    (CommonControls::Feature_Default & ~CommonControls::Feature_RepaintOnF5),
    m_cameraCtrl    (&m_commonCtrl, CameraControls::Feature_Default | CameraControls::Feature_StereoControls),
    m_action        (Action_None),
    m_mesh          (NULL),
    m_cullMode      (CullMode_None),
    m_wireframe     (false),
    m_numRays       (0),
	m_shaderProgram (0),
	m_renderMode	(0),
	m_useNormalMap  (true),
	m_useDiffuseTexture (true),
	m_setDiffuseToZero (false),
	m_setSpecularToZero (false),
	m_normalMapScale(0.15f),
	m_viewpoint(0),
	m_shadowMode(true),
	m_shadowEps(0.0f)
{
    m_commonCtrl.showFPS(true);
    m_commonCtrl.addStateObject(this);
    m_cameraCtrl.setKeepAligned(true);

    m_commonCtrl.addButton((S32*)&m_action, Action_LoadMesh,                FW_KEY_M,       "Load mesh... (M)");
    m_commonCtrl.addButton((S32*)&m_action, Action_ReloadMesh,              FW_KEY_F5,      "Reload mesh (F5)");
    m_commonCtrl.addButton((S32*)&m_action, Action_SaveMesh,                FW_KEY_O,       "Save mesh... (O)");
    m_commonCtrl.addSeparator();

    m_commonCtrl.addButton((S32*)&m_action, Action_ResetCamera,             FW_KEY_NONE,    "Reset camera");
    m_commonCtrl.addButton((S32*)&m_action, Action_EncodeCameraSignature,   FW_KEY_NONE,    "Encode camera signature");
    m_commonCtrl.addButton((S32*)&m_action, Action_DecodeCameraSignature,   FW_KEY_NONE,    "Decode camera signature...");
    m_window.addListener(&m_cameraCtrl);
    m_commonCtrl.addSeparator();

	m_commonCtrl.addButton((S32*)&m_action, Action_ReloadShaders,			FW_KEY_ENTER,   "Recompile Shaders (ENTER)");
    m_commonCtrl.addSeparator();

	m_commonCtrl.addToggle((S32*)&m_renderMode, 0,							FW_KEY_NONE,	"Final render" );
	m_commonCtrl.addToggle((S32*)&m_renderMode, 1,							FW_KEY_NONE,	"Debug render: diffuse texture" );
	m_commonCtrl.addToggle((S32*)&m_renderMode, 2,							FW_KEY_NONE,	"Debug render: normal map texture" );
	m_commonCtrl.addToggle((S32*)&m_renderMode, 3,							FW_KEY_NONE,	"Debug render: final normal" );
	m_commonCtrl.addToggle((S32*)&m_renderMode, 4,							FW_KEY_NONE,	"Debug render: distribution term D");
	m_commonCtrl.addToggle((S32*)&m_renderMode, 5,							FW_KEY_NONE,	"Debug render: geometry term G");
	m_commonCtrl.addToggle((S32*)&m_renderMode, 6,							FW_KEY_NONE,	"Debug render: Fresnel term F");
    m_commonCtrl.addSeparator();

	m_commonCtrl.addToggle((S32*)&m_shadowMode,	0,							FW_KEY_NONE,	"No shadow maps" );
	m_commonCtrl.addToggle((S32*)&m_shadowMode,	1,							FW_KEY_NONE,	"Use shadow maps" );
	m_commonCtrl.addToggle((S32*)&m_viewpoint, 0,							FW_KEY_NONE,	"Camera viewpoint" );
	m_commonCtrl.addToggle((S32*)&m_viewpoint, 1,							FW_KEY_NONE,	"Light 1 viewpoint" );
	m_commonCtrl.addToggle((S32*)&m_viewpoint, 2,							FW_KEY_NONE,	"Light 2 viewpoint" );
	m_commonCtrl.addSlider((F32*)&m_shadowEps, -0.01f, 0.01f, false, FW_KEY_NONE, FW_KEY_NONE, "shadow epsilon %f");
    m_commonCtrl.addSeparator();

	m_commonCtrl.addToggle(&m_useNormalMap,									FW_KEY_NONE,	"Use normal map" );
	m_commonCtrl.addToggle(&m_useDiffuseTexture,							FW_KEY_NONE,	"Use albedo textures" );
	m_commonCtrl.addToggle(&m_setDiffuseToZero,								FW_KEY_NONE,	"Zero diffuse (for debugging specular)" );
	m_commonCtrl.addToggle(&m_setSpecularToZero,							FW_KEY_NONE,	"Zero specular (for debugging diffuse)" );
    m_commonCtrl.addSeparator();

    m_commonCtrl.addToggle(&m_wireframe,                                    FW_KEY_TAB,     "Wireframe");
    m_commonCtrl.addToggle((S32*)&m_cullMode, CullMode_None,                FW_KEY_NONE,    "Disable backface culling");
    m_commonCtrl.addToggle((S32*)&m_cullMode, CullMode_CW,                  FW_KEY_NONE,    "Cull clockwise faces");
    m_commonCtrl.addToggle((S32*)&m_cullMode, CullMode_CCW,                 FW_KEY_NONE,    "Cull counter-clockwise faces");
    m_commonCtrl.addSeparator();

    m_window.setTitle("Assignment 6");
    m_window.addListener(this);
    m_window.addListener(&m_commonCtrl);

	m_commonCtrl.setStateFilePrefix( "Assignment6" );

    if (!m_commonCtrl.loadState(m_commonCtrl.getStateFileName(1)))
        firstTimeInit();

	m_window.getGL(); // grab the appropriate gl context to be able to setup()

	initShader();


	m_lights.push_back(std::pair<LightSource, ShadowMapContext>());
	m_lights[0].first.setStaticPosition(Vec3f(1, 0, 1));
	Mat3f o;
	o.setCol(0, Vec3f(1, 0, -1).normalized());
	o.setCol(1, Vec3f(0, 1, 0));
	o.setCol(2, Vec3f(1, 0, 1).normalized());
	m_lights[0].first.setOrientation(o);

	m_lights.push_back(std::pair<LightSource, ShadowMapContext>());
	m_lights[1].first.setStaticPosition(Vec3f(-1, 1, 1));
	m_lights[1].first.m_color = Vec3f(.5f);


	//Allocate buffers
	for (auto& lpair: m_lights)
		lpair.second.setup(Vec2i(1024, 1024)*4);

	// you might want to place the lights further away
	// cannot see them with the default far setting
	m_cameraCtrl.setFar(10.0f);
}

//------------------------------------------------------------------------

App::~App(void)
{
    delete m_mesh;
	delete m_shaderProgram;
}

//------------------------------------------------------------------------

bool App::handleEvent(const Window::Event& ev)
{
    if (ev.type == Window::EventType_Close)
    {
        m_window.showModalMessage("Exiting...");
        delete this;
        return true;
    }

    Action action = m_action;
    m_action = Action_None;
    String name;
    Mat4f mat;

    switch (action)
    {
    case Action_None:
        break;

    case Action_LoadMesh:
        name = m_window.showFileLoadDialog("Load mesh", getMeshImportFilter());
        if (name.getLength())
            loadScene(name);
        break;

    case Action_ReloadMesh:
        if (m_meshFileName.getLength())
            loadMesh(m_meshFileName);
        break;

    case Action_SaveMesh:
        name = m_window.showFileSaveDialog("Save mesh", getMeshExportFilter());
        if (name.getLength())
            saveMesh(name);
        break;

    case Action_ResetCamera:
        if (m_mesh)
        {
            m_cameraCtrl.initForMesh(m_mesh);
            m_commonCtrl.message("Camera reset");
        }
        break;

	case Action_ReloadShaders:
		initShader();
		break;

    case Action_EncodeCameraSignature:
        m_window.setVisible(false);
        printf("\nCamera signature:\n");
        printf("%s\n", m_cameraCtrl.encodeSignature().getPtr());
		{
			printf("In other words:\n");
			Vec3f vp = m_cameraCtrl.getPosition();
			Vec3f vd = m_cameraCtrl.getForward();
			Vec3f vu = m_cameraCtrl.getUp();
			printf("-vp %f %f %f\n", vp.x, vp.y, vp.z);
			printf("-vd %f %f %f\n", vd.x, vd.y, vd.z);
			printf("-vu %f %f %f\n", vu.x, vu.y, vu.z);
			printf("-fov %f\n", m_cameraCtrl.getFOV());
		}
        waitKey();
        break;

    case Action_DecodeCameraSignature:
        {
            m_window.setVisible(false);
            printf("\nEnter camera signature:\n");

            char buf[1024];
            if (scanf_s("%s", buf, FW_ARRAY_SIZE(buf)))
                m_cameraCtrl.decodeSignature(buf);
            else
                setError("Signature too long!");

            if (!hasError())
                printf("Done.\n\n");
            else
            {
                printf("Error: %s\n", getError().getPtr());
                clearError();
                waitKey();
            }
        }
        break;

    default:
        FW_ASSERT(false);
        break;
    }

    m_window.setVisible(true);

    if (ev.type == Window::EventType_Paint)
        renderFrame(m_window.getGL());
    m_window.repaint();
    return false;
}

//------------------------------------------------------------------------

void App::readState(StateDump& d)
{
    String meshFileName, rayDumpFileName;

    d.pushOwner("App");
    d.get(meshFileName,     "m_meshFileName");
    d.get((S32&)m_cullMode, "m_cullMode");
    d.get(m_wireframe,      "m_wireframe");
    d.get(rayDumpFileName,  "m_rayDumpFileName");
    d.popOwner();

    if (m_meshFileName != meshFileName && meshFileName.getLength())
        loadMesh(meshFileName);
}

//------------------------------------------------------------------------

void App::writeState(StateDump& d) const
{
    d.pushOwner("App");
    d.set(m_meshFileName,       "m_meshFileName");
    d.set((S32)m_cullMode,      "m_cullMode");
    d.set(m_wireframe,          "m_wireframe");
    d.set(m_rayDumpFileName,    "m_rayDumpFileName");
    d.popOwner();
}

//------------------------------------------------------------------------

void App::waitKey(void)
{
    printf("Press any key to continue . . . ");
    _getch();
    printf("\n\n");
}

void App::renderFrame(GLContext* gl)
{
    // No mesh => skip.

    if (!m_mesh)
    {
        gl->drawModalMessage("No mesh loaded!");
        return;
    }
	// YOUR SHADOWS HERE: this is a good place to e.g. move the lights.
	LightSource& a = m_lights[0].first;
	LightSource& b = m_lights[1].first;

	// For now set positions to ones defined in scene file and lights to point toward scene origin
	a.setPosition(a.getStaticPosition());
	a.setOrientation(getOrientation(-a.getPosition(), Vec3f(0.0f, 1.0f, 0.0f)));

	b.setPosition(b.getStaticPosition());
	b.setOrientation(getOrientation(-b.getPosition(), Vec3f(0.0f, 1.0f, 0.0f)));


	if (m_shadowMode) {
		for (auto& lpair : m_lights)
		{
			auto& light = lpair.first;
			auto& shadowCtx = lpair.second;

			// Copy near and far distances from camera to light source
			light.setFar(m_cameraCtrl.getFar());
			light.setNear(m_cameraCtrl.getNear());

			// Render light's shadow map
			light.renderShadowMap(gl, m_mesh, &shadowCtx);
		}

		// shadowmap sets its own viewport, so reset this back
		glViewport(0, 0, gl->getViewSize().x, gl->getViewSize().y);
	}
    // Setup transformations.

    Mat4f worldToCamera = m_cameraCtrl.getWorldToCamera();
    Mat4f projection = gl->xformFitToView(Vec2f(-1.0f, -1.0f), Vec2f(2.0f, 2.0f)) * m_cameraCtrl.getCameraToClip();

    // Initialize GL state.

    glClearColor(0.2f, 0.4f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (m_cullMode == CullMode_None)
        glDisable(GL_CULL_FACE);
    else
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace((m_cullMode == CullMode_CW) ? GL_CCW : GL_CW);
    }

    glPolygonMode(GL_FRONT_AND_BACK, (m_wireframe) ? GL_LINE : GL_FILL);

    // Render.

	renderWithNormalMap(gl, worldToCamera, projection);
	
	// Visualize the lights

	glUseProgram(0);
	// ??
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&projection(0, 0));
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(&worldToCamera(0, 0));

	glEnable(GL_POINT_SMOOTH);
	glPointSize(20);

	glBegin(GL_POINTS);
	for (auto& lpair: m_lights) {
		glColor3f(lpair.first.m_color.x, lpair.first.m_color.y, lpair.first.m_color.z);
		Vec3f p(lpair.first.getPosition());
		glVertex3f(p.x, p.y, p.z);
	}
	glEnd();


    // Clean up.

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Display status line.

    m_commonCtrl.message(sprintf("Triangles = %d, vertices = %d, materials = %d",
        m_mesh->numTriangles(),
        m_mesh->numVertices(),
        m_mesh->numSubmeshes()),
        "meshStats");
}

void App::loadScene(const String& fileName)
{
	if (fileName.endsWith(".obj"))
	{
		// load obj files as normal
		loadMesh(fileName);
		return;
	}
	else if (fileName.endsWith(".txt"))
	{
		// Get file path to use relative paths for obj files in scene file
		auto path = fileName.getDirName();

		// Clear lights list since the scene file will contain a new one
		m_lights.clear();

		// Open file for read
		std::ifstream file(fileName.getPtr());
		std::string line, keyword;

		// Read file line by line
		while (std::getline(file, line))
		{
			std::stringstream stream(line);	// Stream for parsing line word by word
			stream >> keyword;

			if (keyword == "obj")
			{
				std::string name;
				stream >> name; // Read relative path of obj file
				loadMesh(path + "\\" + String(name.c_str())); // Combine name to full path and load mesh
			}
			else if (keyword == "light")
			{
				// Add new entry to lights list and store reference to it so we can set its parameters
				m_lights.push_back(std::pair<LightSource, ShadowMapContext>());
				auto& light = m_lights.back().first;
				m_lights.back().second.setup(Vec2i(1024, 1024));

				// New loop to parse light data
				while (std::getline(file, line))
				{
					std::stringstream line_stream(line);
					line_stream >> keyword;

					if (keyword == "}")
						break;
					else if (keyword == "pos")
					{
						Vec3f p;
						line_stream >> p.x >> p.y >> p.z;
						light.setStaticPosition(p);
					}
					else if (keyword == "col")
					{
						line_stream >> light.m_color.x >> light.m_color.y >> light.m_color.z;
					}
				}
			}
		}
		return;
	}
	else
	{
		std::cout << "Unknown file suffix in scene: " << fileName.getPtr() << std::endl;
	}


}

void App::loadMesh(const String& fileName)
{
    m_window.showModalMessage(sprintf("Loading mesh from '%s'...", fileName.getPtr()));
    String oldError = clearError();
    MeshBase* mesh = importMesh(fileName);
    String newError = getError();

    if (restoreError(oldError))
    {
        delete mesh;
        m_commonCtrl.message(sprintf("Error while loading '%s': %s", fileName.getPtr(), newError.getPtr()));
        return;
    }

    delete m_mesh;
    m_meshFileName = fileName;
    m_mesh = mesh;
    m_commonCtrl.message(sprintf("Loaded mesh from '%s'", fileName.getPtr()));

	// convert normal map
	//{
	//	FILE* pF = fopen( "head/nm.raw", "rb" );
	//	fseek( pF, 0, SEEK_END );
	//	S64 size = ftell(pF);
	//	char* b = (char*)malloc( size );
	//	fseek( pF, 0, SEEK_SET );
	//	fread( b, size, 1, pF );
	//	fclose( pF );
	//	Image bump( Vec2i(6000,6000), ImageFormat::RGBA_Vec4f );
	//	for ( int i = 0; i < 6000*6000; ++i )
	//		bump.setVec4f( Vec2i(i%6000,i/6000), Vec4f(((unsigned short*)b)[i]/65535.f) );
	//	delete[] b;
	//	Image img = convertBumpToObjectSpaceNormal( bump, m_mesh, 0.0025f );
	//	exportImage( "nmap.png", &img );
	//	exit(0);
	//}

}

//------------------------------------------------------------------------

void App::saveMesh(const String& fileName)
{
    if (!m_mesh)
    {
        m_commonCtrl.message("No mesh to save!");
        return;
    }

    m_window.showModalMessage(sprintf("Saving mesh to '%s'...", fileName.getPtr()));
    String oldError = clearError();
    exportMesh(fileName, m_mesh);
    String newError = getError();

    if (restoreError(oldError))
    {
        m_commonCtrl.message(sprintf("Error while saving '%s': %s", fileName.getPtr(), newError.getPtr()));
        return;
    }

    m_meshFileName = fileName;
    m_commonCtrl.message(sprintf("Saved mesh to '%s'", fileName.getPtr()));
}

//------------------------------------------------------------------------

void App::firstTimeInit(void)
{
    loadMesh("head/head.OBJ");
    m_cameraCtrl.decodeSignature("KC3a00RYQE00bUcU/04VMity13gGCz190//Uz////X108Qx7w/6//m100");
    m_commonCtrl.saveState(m_commonCtrl.getStateFileName(1));
    failIfError();
}

//------------------------------------------------------------------------

void FW::init(void)
{
    new App;
}

//------------------------------------------------------------------------

//#undef SOLUTION
void App::initShader()
{
	File vsf( "vshader.glsl", File::Read );
	char* vsa = new char[ vsf.getSize()+1 ];
	vsf.readFully( vsa, (S32)vsf.getSize() );
	vsa[ vsf.getSize() ] = 0;
	String vs( vsa );

	File psf( "pshader.glsl", File::Read );
	char* psa = new char[ psf.getSize()+1 ];
	psf.readFully( psa, (S32)psf.getSize() );
	psa[ psf.getSize() ] = 0;
	String ps( psa );

	delete[] vsa;
	delete[] psa;

	GLContext::Program* pProgram = 0;
	try
	{
		pProgram = new GLContext::Program( vs, ps, true );
	}
	catch ( GLContext::Program::ShaderCompilationException& e )
	{
		printf( "Could not compile shader:\n%s\n", e.msg_.getPtr() );

		if ( m_shaderProgram == 0 && pProgram == 0 )
			fail( "Fatal, need a shader to draw!" );

		return;
	}

	delete m_shaderProgram;
	m_shaderProgram = pProgram;
}

void App::renderWithNormalMap(GLContext* gl, Mat4f worldToCamera, Mat4f projection)
{

    // Find mesh attributes.

	int posAttrib = m_mesh->findAttrib(MeshBase::AttribType_Position);
    int normalAttrib = m_mesh->findAttrib(MeshBase::AttribType_Normal);
    int vcolorAttrib = m_mesh->findAttrib(MeshBase::AttribType_Color);
    int texCoordAttrib = m_mesh->findAttrib(MeshBase::AttribType_TexCoord);
    if (posAttrib == -1)
        return;

    // Setup uniforms.

    m_shaderProgram->use();
	gl->setUniform(m_shaderProgram->getUniformLoc("posToClip"), projection * worldToCamera);
	gl->setUniform(m_shaderProgram->getUniformLoc("posToCamera"), worldToCamera);
    gl->setUniform(m_shaderProgram->getUniformLoc("normalToCamera"), worldToCamera.getXYZ().inverted().transposed());
    gl->setUniform(m_shaderProgram->getUniformLoc("hasNormals"), (normalAttrib != -1));
    gl->setUniform(m_shaderProgram->getUniformLoc("diffuseSampler"), 0);
	gl->setUniform(m_shaderProgram->getUniformLoc("specularSampler"), 0); // we don't actually have a specular albedo texture for the head so this will have to do
    gl->setUniform(m_shaderProgram->getUniformLoc("normalSampler"), 1);

	// You can add lights here. Remember to set their intensities.
	std::vector<Vec3f> lightDir, I;
	for (auto& lpair : m_lights)
	{
		lightDir.push_back(lpair.first.getPosition());
		I.push_back(lpair.first.m_color);
	}
	int nlights = m_lights.size();

	for ( int i = 0; i < nlights; ++i )
	{
		lightDir[i] = (worldToCamera * Vec4f( lightDir[i].normalized(), 0.0f )).getXYZ();
	}

	gl->setUniformArray(m_shaderProgram->getUniformLoc("lightDirections"), nlights, lightDir.data() );
	gl->setUniformArray(m_shaderProgram->getUniformLoc("lightIntensities"), nlights, I.data() );
	gl->setUniform(m_shaderProgram->getUniformLoc("numLights"), nlights);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh->getVBO().getGLBuffer());
    m_mesh->setGLAttrib(gl, posAttrib, m_shaderProgram->getAttribLoc("positionAttrib"));

    if (normalAttrib != -1)
        m_mesh->setGLAttrib(gl, normalAttrib, m_shaderProgram->getAttribLoc("normalAttrib"));
    else if ( m_shaderProgram->getAttribLoc("normalAttrib") != -1 )
        glVertexAttrib3f(m_shaderProgram->getAttribLoc("normalAttrib"), 0.0f, 0.0f, 0.0f);

    if (vcolorAttrib != -1)
        m_mesh->setGLAttrib(gl, vcolorAttrib, m_shaderProgram->getAttribLoc("vcolorAttrib"));
    else if ( m_shaderProgram->getAttribLoc("vcolorAttrib") != -1 )
        glVertexAttrib4f(m_shaderProgram->getAttribLoc("vcolorAttrib"), 1.0f, 1.0f, 1.0f, 1.0f);

    if (texCoordAttrib != -1)
        m_mesh->setGLAttrib(gl, texCoordAttrib, m_shaderProgram->getAttribLoc("texCoordAttrib"));
    else if ( m_shaderProgram->getAttribLoc("texCoordAttrib") != -1 )
        glVertexAttrib2f(m_shaderProgram->getAttribLoc("texCoordAttrib"), 0.0f, 0.0f);

    // Render each submesh.

	gl->setUniform(m_shaderProgram->getUniformLoc("renderMode"), m_renderMode );
	gl->setUniform(m_shaderProgram->getUniformLoc("normalMapScale"), m_normalMapScale );
	gl->setUniform(m_shaderProgram->getUniformLoc("setDiffuseToZero"), m_setDiffuseToZero );
	gl->setUniform(m_shaderProgram->getUniformLoc("setSpecularToZero"), m_setSpecularToZero );

	// Shadow maps
	for (int i = 0; i < int(m_lights.size()); i++) {
		LightSource& ls = m_lights[i].first;
		// Each array item is also a single uniform in opengl
		String ss = "shadowSampler["; ss += ('0' + (char)i); ss += ']';
		String ptlc = "posToLightClip["; ptlc += ('0' + (char)i); ptlc += ']';
		glActiveTexture(GL_TEXTURE2 + i);
		glBindTexture(GL_TEXTURE_2D, ls.getShadowTextureHandle());
		gl->setUniform(m_shaderProgram->getUniformLoc(ss.getPtr()), 2 + i);
		gl->setUniform(m_shaderProgram->getUniformLoc(ptlc.getPtr()), ls.getPosToLightClip());
	}
	gl->setUniform(m_shaderProgram->getUniformLoc("renderFromLight"), m_viewpoint);
	gl->setUniform(m_shaderProgram->getUniformLoc("shadowMaps"), m_shadowMode != 0);
	gl->setUniform(m_shaderProgram->getUniformLoc("shadowEps"), m_shadowEps);

	for (int i = 0; i < m_mesh->numSubmeshes(); i++)
    {
        const MeshBase::Material& mat = m_mesh->material(i);
        gl->setUniform(m_shaderProgram->getUniformLoc("diffuseUniform"), mat.diffuse);
        gl->setUniform(m_shaderProgram->getUniformLoc("roughness"), sqrt(2.0f / (mat.glossiness + 2.0f)));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mat.textures[MeshBase::TextureType_Diffuse].getGLTexture());
        gl->setUniform(m_shaderProgram->getUniformLoc("useTextures"), mat.textures[MeshBase::TextureType_Diffuse].exists() && m_useDiffuseTexture && m_renderMode<4);

        glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mat.textures[MeshBase::TextureType_Normal].getGLTexture());
		gl->setUniform(m_shaderProgram->getUniformLoc("useNormalMap"), mat.textures[MeshBase::TextureType_Normal].exists() && m_useNormalMap);

        glDrawElements(GL_TRIANGLES, m_mesh->vboIndexSize(i), GL_UNSIGNED_INT, (void*)(UPTR)m_mesh->vboIndexOffset(i));
    }

    gl->resetAttribs();
}

Image App::convertBumpToObjectSpaceNormal( const Image& bump, MeshBase* pMeshIn, float alpha )
{
	Vec2i s = bump.getSize();
	S32 W = s.x;
	S32 H = s.y;

	// result normal map
	Image nmap( s, ImageFormat::RGB_Vec3f );

	// convert into suitable format 
	Mesh<VertexPNT>* pMesh = new Mesh<VertexPNT>( *pMeshIn );

	// loop over triangles, rasterize into normal map texture
	for ( int m = 0; m < pMesh->numSubmeshes(); ++m )
		for ( int t = 0; t < pMesh->indices(m).getSize(); ++t )
		{
			// get vertices' UV coordinates
			Vec2f v[3];
			v[0] = pMesh->vertex(pMesh->indices(m)[t].x).t;
			v[1] = pMesh->vertex(pMesh->indices(m)[t].y).t;
			v[2] = pMesh->vertex(pMesh->indices(m)[t].z).t;
			v[0].x *= W; v[0].y *= H;
			v[1].x *= W; v[1].y *= H;
			v[2].x *= W; v[2].y *= H;

			// for getting barys from UVs, as per rasterization lecture slides
			Mat3f interpolation;
			interpolation.setCol( 0, Vec3f( v[0], 1.0f ) );
			interpolation.setCol( 1, Vec3f( v[1], 1.0f ) );
			interpolation.setCol( 2, Vec3f( v[2], 1.0f ) );
			interpolation.invert();

			Mat3f PM;
			PM.setCol( 0, pMesh->vertex(pMesh->indices(m)[t].x).p );
			PM.setCol( 1, pMesh->vertex(pMesh->indices(m)[t].y).p );
			PM.setCol( 2, pMesh->vertex(pMesh->indices(m)[t].z).p );
			Mat3f NM;
			NM.setCol( 0, pMesh->vertex(pMesh->indices(m)[t].x).n );
			NM.setCol( 1, pMesh->vertex(pMesh->indices(m)[t].y).n );
			NM.setCol( 2, pMesh->vertex(pMesh->indices(m)[t].z).n );

			Mat3f Mp = PM * interpolation;	// Mp = vertex matrix * inv(uv matrix), gives point on mesh as function of x, y
			Mat3f Mn = NM * interpolation;	// Mn = normal matrix * inv(uv matrix), gives normal as function of x, y

			// rasterize in fixed point, 8 bits of subpixel precision
			// first convert vertices to FP
			S64 v_[6]; 
			v_[0] = (S64)(v[0].x*256); v_[1] = (S64)(v[0].y*256);
			v_[2] = (S64)(v[1].x*256); v_[3] = (S64)(v[1].y*256);
			v_[4] = (S64)(v[2].x*256); v_[5] = (S64)(v[2].y*256);

			// compute fixed-point edge functions
			S64 edges[3][3];
			edges[0][0] = v_[3]-v_[1]; edges[0][1] = -(v_[2]-v_[0]); edges[0][2] = -(edges[0][0]*v_[0] + edges[0][1]*v_[1]);
			edges[1][0] = v_[5]-v_[3]; edges[1][1] = -(v_[4]-v_[2]); edges[1][2] = -(edges[1][0]*v_[2] + edges[1][1]*v_[3]);
			edges[2][0] = v_[1]-v_[5]; edges[2][1] = -(v_[0]-v_[4]); edges[2][2] = -(edges[2][0]*v_[4] + edges[2][1]*v_[5]);

			Vec2i bbmin, bbmax;
			// bounding box minimum: just chop off fractional part
			bbmin = Vec2i( S32(min(v_[0], v_[2], v_[4])) >> 8, S32(min(v_[1], v_[3], v_[5])) >> 8 );
			// maximum: have to round up
			bbmax = Vec2i( (S32(max(v_[0], v_[2], v_[4])) & 0xFF) ? 1 : 0, (S32(max(v_[1], v_[3], v_[5])) & 0xFF) ? 1 : 0 );
			bbmax += Vec2i(S32(max(v_[0], v_[2], v_[4])) >> 8, S32(max(v_[1], v_[3], v_[5])) >> 8 );

			for ( int y = bbmin.y; y <= bbmax.y; ++y )
				for ( int x = bbmin.x; x <= bbmax.x; ++x )
				{
					// center of pixel
					S64 px = (x << 8) + 127;
					S64 py = (y << 8) + 127;

					// evaluate edge functions
					S64 e[3];
					e[0] = px*edges[0][0] + py*edges[0][1] + edges[0][2];
					e[1] = px*edges[1][0] + py*edges[1][1] + edges[1][2];
					e[2] = px*edges[2][0] + py*edges[2][1] + edges[2][2];

					// test for containment
					if ( (e[0] >= 0 && e[1] >= 0 && e[2] >= 0) ||
						 (e[0] <= 0 && e[1] <= 0 && e[2] <= 0) )
					{
						// we are in, bet barys
						Vec3f b = interpolation * Vec3f( x+0.5f, y+0.5f, 1.0f );
						b = b * (1.0f / b.sum());	// should already sum to one but what the heck

						// interpolate normal on mesh surface, NOT normalized
						Vec3f N = b[0] * pMesh->vertex(pMesh->indices(m)[t].x).n + 
							      b[1] * pMesh->vertex(pMesh->indices(m)[t].y).n +
								  b[2] * pMesh->vertex(pMesh->indices(m)[t].z).n;
						N.normalize();

						// compute finite difference derivative of height
						//float f00 = bump.getVec4f( Vec2i(x,   y )).x;
						float f01 = bump.getVec4f( Vec2i(x-1, y) ).x;
						float f02 = bump.getVec4f( Vec2i(x+1, y )).x;
						float f03 = bump.getVec4f( Vec2i(x,   y-1 )).x;
						float f04 = bump.getVec4f( Vec2i(x,   y+1 )).x;

						// central differencing to get dh/du, dh/dv
						float dhdx = (f02-f01) / 2.0f;
						float dhdy = (f04-f03) / 2.0f;

						//     P' = P(u,v) + n(u,v) + \alpha * n(u,v) * h(u,v)
						// =>  dP'/du = dP/du + dn/du + \alpha*[ h*dn/du + n*dh/du ]
						// dP/du = Mp(:,0), dP/dv = Mp(:,1) (Matlab notation)
						// similar for dn/du,v

						Vec3f dPdx = Mp.getCol(0);
						Vec3f dPdy = Mp.getCol(1);
						float l1 = dPdx.length();
						float l2 = dPdy.length();
						dPdx -= N*dot(dPdx,N);
						dPdy -= N*dot(dPdy,N);
						dPdx = dPdx.normalized()*l1;
						dPdy = dPdy.normalized()*l2;

						Vec3f dndx = Mn.getCol(0);
						Vec3f dndy = Mn.getCol(1);

						//Vec3f dndu = Mn.getCol(0) / N.length();
						//dndu -= dot(Mn.getCol(0), N) * (1.0f / pow(dot(N,N),1.5f));
						//Vec3f dndv = Mn.getCol(1) / N.length();
						//dndv -= dot(Mn.getCol(1), N) * (1.0f / pow(dot(N,N),1.5f));
						//dndu *= (1.0f / W);
						//dndv *= (1.0f / H);
						//dPdu *= (1.0f / W);
						//dPdv *= (1.0f / H);

						//Vec3f dPpdx = dPdx + alpha * (dndx*f00 + dhdx*N);
						//Vec3f dPpdy = dPdy + alpha * (dndy*f00 + dhdy*N);
						Vec3f dPpdx = dPdx + alpha * (dhdx*N);
						Vec3f dPpdy = dPdy + alpha * (dhdy*N);

						// ha!
						Vec3f bumpedNormal = cross( dPpdx, dPpdy ).normalized();

						if ( dot( bumpedNormal, N ) < 0.0f )
							bumpedNormal = -bumpedNormal;

						//bumpedNormal = 10.0f * Vec3f( dhdx, dhdy, 0.0f );

						nmap.setVec4f( Vec2i(x,y), Vec4f( bumpedNormal*0.5+0.5, 0.0f ) );
						//nmap.setVec4f( Vec2i(x,y), bump.getVec4f(Vec2i(x,y)) );						
						//nmap.setVec4f( Vec2i(x,y), Vec4f( N.normalized()*0.5+0.5, 1.0f ) );
					}
				}
		}

	delete pMesh;

	return nmap;
}
