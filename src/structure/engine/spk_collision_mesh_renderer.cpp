#include "structure/engine/spk_collision_mesh_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

#include "spk_generated_resources.hpp"
#include "structure/engine/spk_entity.hpp"

#include <GL/glew.h>

namespace spk
{
	spk::Lumina::Shader CollisionMeshRenderer::Painter::_createShader()
	{
		spk::Lumina::ShaderObjectFactory::instance()->add(
			spk::JSON::Object::fromString(SPARKLE_GET_RESOURCE_AS_STRING("resources/json/mesh_renderer_shader_object.json")));

		const char *vertexShaderSrc = R"(#version 450
layout(location = 0) in vec3 inPosition;

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
        gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * transformUBO.modelMatrix * vec4(inPosition, 1.0);
}
)";

		const char *fragmentShaderSrc = R"(#version 450
layout(location = 0) out vec4 outputColor;
void main()
{
    const vec3 colors[16] = vec3[16](
        vec3(0.0, 0.0, 1.0),   // Blue
        vec3(0.0, 1.0, 0.0),   // Green
        vec3(1.0, 0.0, 0.0),   // Red
        vec3(1.0, 1.0, 0.0),   // Yellow
        vec3(1.0, 0.0, 1.0),   // Magenta
        vec3(0.0, 1.0, 1.0),   // Cyan
        vec3(1.0, 1.0, 1.0),   // White
        vec3(0.5, 0.5, 0.5),   // Gray
        vec3(0.3, 0.0, 0.5),   // Purple
        vec3(0.8, 0.3, 0.0),   // Orange
        vec3(0.0, 0.4, 0.8),   // Light Blue
        vec3(0.5, 0.8, 0.1),   // Lime
        vec3(0.8, 0.8, 0.3),   // Sand
        vec3(0.8, 0.5, 0.8),   // Pinkish Purple
        vec3(0.2, 0.8, 0.7),   // Aqua
        vec3(0.9, 0.9, 0.9)    // Almost White
    );

    uint id = uint(gl_PrimitiveID);
    vec3 color = colors[id % 16];
    outputColor = vec4(color, 1.0);
}
)";

		spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);
		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3});

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);
		shader.addUBO(L"TransformUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO"), spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}

	spk::Lumina::Shader CollisionMeshRenderer::Painter::_shader = CollisionMeshRenderer::Painter::_createShader();

	CollisionMeshRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_transformUBO(_object.UBO(L"TransformUBO"))
	{
	}

	void CollisionMeshRenderer::Painter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void CollisionMeshRenderer::Painter::prepare(const spk::CollisionMesh &p_mesh)
	{
		unsigned int offset = 0;
		for (const auto &poly : p_mesh.polygons())
		{
			auto tris = poly.triangulize();
			for (const auto &tri : tris)
			{
				_bufferSet.layout() << tri.points[0] << tri.points[1] << tri.points[2];
				_bufferSet.indexes() << offset << (offset + 1) << (offset + 2);
				offset += 3;
			}
		}
	}

	void CollisionMeshRenderer::Painter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void CollisionMeshRenderer::Painter::render(bool p_wireframe)
	{
		if (p_wireframe == true)
		{
			_object.render();
		}
		else
		{
			_object.render();
		}
	}

	void CollisionMeshRenderer::Painter::setTransform(const spk::Transform &p_transform)
	{
		_transformUBO.layout()[L"modelMatrix"] = p_transform.model();
		_transformUBO.validate();
	}

	CollisionMeshRenderer::CollisionMeshRenderer(const std::wstring &p_name) :
		spk::Component(p_name),
		_painter()
	{
	}

	void CollisionMeshRenderer::setMesh(const spk::SafePointer<const spk::CollisionMesh> &p_mesh)
	{
		_mesh = p_mesh;

		_painter.clear();
		if (_mesh != nullptr)
		{
			_painter.prepare(*_mesh);
		}
		_painter.validate();
	}

	const spk::SafePointer<const spk::CollisionMesh> &CollisionMeshRenderer::mesh() const
	{
		return (_mesh);
	}

	void CollisionMeshRenderer::setWireframe(bool p_wireframe)
	{
		_wireframe = p_wireframe;
	}

	bool CollisionMeshRenderer::wireframe() const
	{
		return (_wireframe);
	}

	void CollisionMeshRenderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		_painter.render(_wireframe);
	}

	void CollisionMeshRenderer::start()
	{
		_onOwnerTransformEditionContract = owner()->transform().addOnEditionCallback([this]() { _painter.setTransform(owner()->transform()); });
		_onOwnerTransformEditionContract.trigger();
	}
}
