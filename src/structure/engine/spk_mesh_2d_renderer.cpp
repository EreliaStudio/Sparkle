#include "structure/engine/spk_mesh_2d_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

#include "spk_debug_macro.hpp"
#include "spk_generated_resources.hpp"
#include "structure/engine/spk_entity.hpp"

namespace spk
{
	spk::Lumina::Shader Mesh2DRenderer::Painter::_createShader()
	{
		spk::Lumina::ShaderObjectFactory::instance()->add(
			spk::JSON::Object::fromString(SPARKLE_GET_RESOURCE_AS_STRING("resources/json/mesh_renderer_shader_object.json")));

		const char *vertexShaderSrc = R"(#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

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
fragUV = inUV;
vec3 position = vec3(inPosition, 0.0);
gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * transformUBO.modelMatrix * vec4(position, 1.0);
}
)";

		const char *fragmentShaderSrc = R"(#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outputColor;
uniform sampler2D diffuseTexture;
const float epsilon = 0.0001;
void main()
{
if (fragUV.x < 0 || fragUV.x > 1 || fragUV.y < 0 || fragUV.y > 1)
{
outputColor = vec4(1, 0, 1, 1);
}
else
{
vec4 texColor = texture(diffuseTexture, fragUV);
if (texColor.a <= epsilon)
discard;
outputColor = texColor;
}
}
)";

		spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);
		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});
		shader.addAttribute({1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});

		shader.addSampler(
			L"diffuseTexture", spk::Lumina::ShaderObjectFactory::instance()->sampler(L"diffuseTexture"), spk::Lumina::Shader::Mode::Attribute);

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);

		shader.addUBO(L"TransformUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO"), spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}

	spk::Lumina::Shader Mesh2DRenderer::Painter::_shader = Mesh2DRenderer::Painter::_createShader();

	Mesh2DRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_diffuseSampler(_object.sampler(L"diffuseTexture")),
		_transformUBO(_object.ubo(L"TransformUBO"))
	{
	}

	void Mesh2DRenderer::Painter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void Mesh2DRenderer::Painter::prepare(const Mesh2D &p_mesh)
	{
		const auto &buffer = p_mesh.buffer();
		_bufferSet.layout() << buffer.vertices;
		_bufferSet.indexes() << buffer.indexes;
	}

	void Mesh2DRenderer::Painter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void Mesh2DRenderer::Painter::render()
	{
		_object.render();
	}

	void Mesh2DRenderer::Painter::setTransform(const spk::Transform &p_transform)
	{
		_transformUBO->layout()[L"modelMatrix"] = p_transform.model();
		_transformUBO->validate();
	}

	void Mesh2DRenderer::Painter::setTexture(const spk::SafePointer<const spk::Texture> &p_texture)
	{
		_diffuseSampler->bind(p_texture);
	}

	const spk::SafePointer<const spk::Texture> &Mesh2DRenderer::Painter::texture() const
	{
		return (_diffuseSampler->texture());
	}

	Mesh2DRenderer::Mesh2DRenderer(const std::wstring &p_name) :
		spk::Component(p_name),
		_painter()
	{
	}

	void Mesh2DRenderer::setTexture(spk::SafePointer<const spk::Texture> p_texture)
	{
		_painter.setTexture(p_texture);
	}

	const spk::SafePointer<const spk::Texture> &Mesh2DRenderer::texture() const
	{
		return (_painter.texture());
	}

	void Mesh2DRenderer::setMesh(const spk::SafePointer<const spk::Mesh2D> &p_mesh)
	{
		_mesh = p_mesh;
		_painter.clear();
		if (_mesh != nullptr)
		{
			_painter.prepare(*_mesh);
		}
		_painter.validate();
	}

	const spk::SafePointer<const spk::Mesh2D> &Mesh2DRenderer::mesh() const
	{
		return (_mesh);
	}

	void Mesh2DRenderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		_painter.render();
	}

	void Mesh2DRenderer::start()
	{
		_onOwnerTransformEditionContract = owner()->transform().addOnEditionCallback([this]() { _painter.setTransform(owner()->transform()); });
		_onOwnerTransformEditionContract.trigger();
	}
}
