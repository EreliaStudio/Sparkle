#include "structure/engine/spk_color_mesh_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

#include "spk_generated_resources.hpp"
#include "structure/engine/spk_entity.hpp"

namespace spk
{
	spk::Lumina::Shader ColorMeshRenderer::Painter::_createShader()
	{
		if (spk::Lumina::ShaderObjectFactory::instance() == nullptr)
		{
			spk::Lumina::ShaderObjectFactory::instanciate();
		}
		
		spk::Lumina::ShaderObjectFactory::instance()->add(
			spk::JSON::Object::fromString(SPARKLE_GET_RESOURCE_AS_STRING("resources/json/mesh_renderer_shader_object.json")));

		const char *vertexShaderSrc = R"(#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

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
fragColor = inColor;
gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * transformUBO.modelMatrix * vec4(inPosition, 1.0);
}
)";

		const char *fragmentShaderSrc = R"(#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outputColor;
void main()
{
        outputColor = fragColor;
}
)";

		spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);

		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3});
		shader.addAttribute({1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector4});

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);

		shader.addUBO(L"TransformUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO"), spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}

	spk::Lumina::Shader ColorMeshRenderer::Painter::_shader = ColorMeshRenderer::Painter::_createShader();

	ColorMeshRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_transformUBO(_object.ubo(L"TransformUBO"))
	{
	}

	void ColorMeshRenderer::Painter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void ColorMeshRenderer::Painter::prepare(const spk::ColorMesh &p_mesh)
	{
		const auto &buffer = p_mesh.buffer();
		_bufferSet.layout() << buffer.vertices;
		_bufferSet.indexes() << buffer.indexes;
	}

	void ColorMeshRenderer::Painter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void ColorMeshRenderer::Painter::render()
	{
		_object.render();
	}

	void ColorMeshRenderer::Painter::setTransform(const spk::Transform &p_transform)
	{
		_transformUBO.layout()[L"modelMatrix"] = p_transform.model();
		_transformUBO.validate();
	}

	ColorMeshRenderer::ColorMeshRenderer(const std::wstring &p_name) :
		spk::Component(p_name),
		_painter()
	{
	}

	void ColorMeshRenderer::setMesh(const spk::SafePointer<const spk::ColorMesh> &p_mesh)
	{
		_mesh = p_mesh;

		_painter.clear();
		if (_mesh != nullptr)
		{
			_painter.prepare(*_mesh);
		}
		_painter.validate();
	}

	const spk::SafePointer<const spk::ColorMesh> &ColorMeshRenderer::mesh() const
	{
		return (_mesh);
	}

	void ColorMeshRenderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		_painter.render();
	}

	void ColorMeshRenderer::start()
	{
		_onOwnerTransformEditionContract = owner()->transform().addOnEditionCallback([this]() { _painter.setTransform(owner()->transform()); });
		_onOwnerTransformEditionContract.trigger();
	}
}
