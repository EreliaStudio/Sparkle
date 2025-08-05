#include "structure/engine/spk_obj_mesh_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

#include "spk_generated_resources.hpp"

namespace spk
{
	spk::Lumina::Shader ObjMeshRenderer::Painter::_createShader()
	{
		spk::Lumina::ShaderObjectFactory::instance()->add(spk::JSON::Object::fromString(
			SPARKLE_GET_RESOURCE_AS_STRING("resources/json/mesh_renderer_shader_object.json")
		));

		const char *vertexShaderSrc = R"(#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

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
gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * transformUBO.modelMatrix * vec4(inPosition, 1.0);
}
)";

		const char *fragmentShaderSrc = R"(#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outputColor;
uniform sampler2D diffuseTexture;
const float epsilon = 0.0001;
void main()
{
vec4 texColor = texture(diffuseTexture, fragUV);
if (texColor.a <= epsilon)
discard;
outputColor = texColor;
}
)";

		spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);
		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3});
		shader.addAttribute({1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});
		shader.addAttribute({2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3});

	DEBUG_LINE();
		shader.addSampler(
				L"diffuseTexture",
				spk::Lumina::ShaderObjectFactory::instance()->sampler(L"diffuseTexture"),
				spk::Lumina::Shader::Mode::Attribute
			);
	DEBUG_LINE();

		shader.addUBO(
				L"CameraUBO", 
				spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), 
				spk::Lumina::Shader::Mode::Constant
			);
	DEBUG_LINE();

		shader.addUBO(
				L"TransformUBO", 
				spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO"), 
				spk::Lumina::Shader::Mode::Attribute
			);
	DEBUG_LINE();

		return (shader);
	}

	spk::Lumina::Shader ObjMeshRenderer::Painter::_shader = ObjMeshRenderer::Painter::_createShader();

	ObjMeshRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_diffuseSampler(_shader.sampler(L"diffuseTexture"))
	{
	}

	void ObjMeshRenderer::Painter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void ObjMeshRenderer::Painter::prepare(const spk::ObjMesh &p_mesh)
	{
		const auto &buffer = p_mesh.buffer();
		_bufferSet.layout() << buffer.vertices;
		_bufferSet.indexes() << buffer.indexes;
	}

	void ObjMeshRenderer::Painter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void ObjMeshRenderer::Painter::render()
	{
		_object.render();
	}

	spk::Lumina::Shader &ObjMeshRenderer::Painter::shader()
	{
		return (_shader);
	}

	spk::OpenGL::SamplerObject &ObjMeshRenderer::Painter::diffuseSampler()
	{
		return (_diffuseSampler);
	}

	ObjMeshRenderer::ObjMeshRenderer() :
		_painter()
	{
		_painter.diffuseSampler().bind(_texture);
	}

	void ObjMeshRenderer::setTexture(spk::SafePointer<spk::Image> p_texture)
	{
		_texture = p_texture;
		_painter.diffuseSampler().bind(_texture);
	}

	spk::SafePointer<spk::Image> &ObjMeshRenderer::texture()
	{
		return (_texture);
	}

	const spk::SafePointer<spk::Image> &ObjMeshRenderer::texture() const
	{
		return (_texture);
	}

	spk::Lumina::Shader &ObjMeshRenderer::shader()
	{
		return (_painter.shader());
	}

	ObjMeshRenderer::Painter &ObjMeshRenderer::painter()
	{
		return (_painter);
	}
}
