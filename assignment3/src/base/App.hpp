#pragma once

#include "skeleton.hpp"

#include "gui/Window.hpp"
#include "gui/CommonControls.hpp"

#include <vector>

namespace FW {

struct Vertex
{
	Vec3f position;
	Vec3f normal;
	Vec3f color;
};

struct WeightedVertex
{
	Vec3f	position;
	Vec3f	normal;
	Vec3f	color;
	int		joints[WEIGHTS_PER_VERTEX];
	float	weights[WEIGHTS_PER_VERTEX];
};

struct glGeneratedIndices
{
	// Shader programs
	GLuint simple_shader, ssd_shader;

	// Vertex array objects
	GLuint simple_vao, ssd_vao;

	// Buffers
	GLuint simple_vertex_buffer, ssd_vertex_buffer;

	// simple_shader uniforms
	GLint simple_world_to_clip_uniform, simple_shading_mix_uniform;

	// ssd_shader uniforms
	GLint ssd_world_to_clip_uniform, ssd_shading_mix_uniform, ssd_transforms_uniform;
};

class App : public Window::Listener
{
private:
	enum DrawMode
	{
		MODE_SKELETON,
		MODE_MESH_CPU,
		MODE_MESH_GPU
	};

public:
					App             (void);
	virtual         ~App            (void) {}

	virtual bool    handleEvent     (const Window::Event& ev);

private:
	void			initRendering		(void);
	void			render				(void);
	void			renderSkeleton		(void);

	void			loadModel			(const String& filename);
	void			loadAnimation		(const String& filename);
	

	std::vector<WeightedVertex>	loadAnimatedMesh		(std::string namefile, std::string mesh_file, std::string attachment_file);
	std::vector<WeightedVertex>	loadWeightedMesh		(std::string mesh_file, std::string attachment_file);

	std::vector<Vertex>			computeSSD				(const std::vector<WeightedVertex>& source);
private:
					App             (const App&); // forbid copy
	App&            operator=       (const App&); // forbid assignment

private:
	Window			window_;
	CommonControls	common_ctrl_;

	DrawMode		drawmode_;
	String			filename_;
	bool			shading_toggle_;
	bool			shading_mode_changed_;
	std::vector<Vec3f> joint_colors_;

	glGeneratedIndices	gl_;

	std::vector<WeightedVertex> weighted_vertices_;
	
	float			camera_rotation_;
	float			scale_ = 1.f;

	Skeleton		skel_;

	unsigned		selected_joint_;

	bool			animationMode = false;
	DWORD			animationStart;
};

} // namespace FW
