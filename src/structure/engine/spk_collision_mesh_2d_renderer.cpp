#include "structure/engine/spk_collision_mesh_2d_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

#include "spk_generated_resources.hpp"
#include "structure/engine/spk_entity.hpp"

#include <GL/glew.h>

namespace spk
{
	spk::Lumina::Shader CollisionMesh2DRenderer::Painter::_createShader()
	{
		spk::Lumina::ShaderObjectFactory::instance()->add(
			spk::JSON::Object::fromString(SPARKLE_GET_RESOURCE_AS_STRING("resources/json/mesh_renderer_shader_object.json")));

		const char *vertexShaderSrc = R"(#version 450
layout(location = 0) in vec2 inPosition;

layout(std140, binding = 0) uniform CameraUBO
{
mat4 viewMatrix;
mat4 projectionMatrix;
} cameraUBO;

layout(std140, binding = 1) uniform TransformUBO
{
mat4 modelMatrix;
} transformUBO;

void main()
{
vec3 position = vec3(inPosition, 0.0);
gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * transformUBO.modelMatrix * vec4(position, 1.0);
}
)";

		const char *fragmentShaderSrc = R"(#version 450
layout(location = 0) out vec4 outputColor;
void main()
{
const vec3 colors[16] = vec3[16](
vec3(0.0, 0.0, 1.0),
vec3(0.0, 1.0, 0.0),
vec3(1.0, 0.0, 0.0),
vec3(1.0, 1.0, 0.0),
vec3(1.0, 0.0, 1.0),
vec3(0.0, 1.0, 1.0),
vec3(1.0, 1.0, 1.0),
vec3(0.5, 0.5, 0.5),
vec3(0.3, 0.0, 0.5),
vec3(0.8, 0.3, 0.0),
vec3(0.0, 0.4, 0.8),
vec3(0.5, 0.8, 0.1),
vec3(0.8, 0.8, 0.3),
vec3(0.8, 0.5, 0.8),
vec3(0.2, 0.8, 0.7),
vec3(0.9, 0.9, 0.9)
);

uint id = uint(gl_PrimitiveID);
vec3 color = colors[id % 16];
outputColor = vec4(color, 1.0);
}
)";

		spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);
		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);
		shader.addUBO(L"TransformUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO"), spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}

	spk::Lumina::Shader CollisionMesh2DRenderer::Painter::_shader = CollisionMesh2DRenderer::Painter::_createShader();

	CollisionMesh2DRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_transformUBO(_object.ubo(L"TransformUBO"))
	{
	}

	void CollisionMesh2DRenderer::Painter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void CollisionMesh2DRenderer::Painter::prepare(const spk::CollisionMesh2D &p_mesh)
	{
		unsigned int offset = 0;
		for (const auto &unit : p_mesh.units())
		{
			for (const auto &point : unit.points())
			{
				_bufferSet.layout() << point;
			}
			if (unit.points().size() >= 3)
			{
				for (size_t i = 1; i + 1 < unit.points().size(); ++i)
				{
					_bufferSet.indexes() << (offset + 0) << (offset + static_cast<unsigned int>(i)) << (offset + static_cast<unsigned int>(i + 1));
				}
			}
			offset += static_cast<unsigned int>(unit.points().size());
		}
	}

	void CollisionMesh2DRenderer::Painter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void CollisionMesh2DRenderer::Painter::render(bool p_wireframe)
	{
		if (p_wireframe == true)
		{
			glDisable(GL_CULL_FACE);
			_object.render();
			glEnable(GL_CULL_FACE);
		}
		else
		{
			_object.render();
		}
	}

	void CollisionMesh2DRenderer::Painter::setTransform(const spk::Transform &p_transform)
	{
		_transformUBO->layout()[L"modelMatrix"] = p_transform.model();
		_transformUBO->validate();
	}

	CollisionMesh2DRenderer::CollisionMesh2DRenderer(const std::wstring &p_name) :
		spk::Component(p_name),
		_painter()
	{
	}

	void CollisionMesh2DRenderer::setMesh(const spk::SafePointer<const spk::CollisionMesh2D> &p_mesh)
	{
		_mesh = p_mesh;
		_painter.clear();
		if (_mesh != nullptr)
		{
			_painter.prepare(*_mesh);
		}
		_painter.validate();
	}

	const spk::SafePointer<const spk::CollisionMesh2D> &CollisionMesh2DRenderer::mesh() const
	{
		return (_mesh);
	}

	void CollisionMesh2DRenderer::setWireframe(bool p_wireframe)
	{
		_wireframe = p_wireframe;
	}

	bool CollisionMesh2DRenderer::wireframe() const
	{
		return (_wireframe);
	}

	void CollisionMesh2DRenderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		_painter.render(_wireframe);
	}

	void CollisionMesh2DRenderer::start()
	{
		_onOwnerTransformEditionContract = owner()->transform().addOnEditionCallback([this]() { _painter.setTransform(owner()->transform()); });
		_onOwnerTransformEditionContract.trigger();
	}
}
