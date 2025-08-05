#include "structure/graphics/renderer/spk_mesh_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

namespace spk
{
	spk::Lumina::ShaderObjectFactory::Instanciator MeshRenderer::Painter::_initializeObjectFactory()
	{
		const wchar_t *uboString = LR"(
{
"UBO": [
{
"name": "CameraUBO",
"data": {
"BlockName": "CameraUBO",
"BindingPoint": 0,
"Size": 128,
"Elements": [
{"Name": "viewMatrix", "Offset": 0, "Size": 64},
{"Name": "projectionMatrix", "Offset": 64, "Size": 64}
]
}
},
{
"name": "TransformUBO",
"data": {
"BlockName": "TransformUBO",
"BindingPoint": 1,
"Size": 64,
"Elements": [
{"Name": "modelMatrix", "Offset": 0, "Size": 64}
]
}
}
]
})";

		const wchar_t *samplerString = LR"(
{
"Sampler": [
{
"name": "diffuseTexture",
"data": {"Name": "diffuseTexture", "Type": "Texture2D", "BindingPoint": 0}
}
]
})";

		const wchar_t *ssboString = LR"(
{
"SSBO": [
{
"name": "DummySSBO",
"data": {
"BlockName": "DummySSBO",
"BindingPoint": 0,
"FixedSize": 16,
"PaddingFixedToDynamic": 16,
"DynamicElementSize": 32,
"DynamicElementPadding": 0,
"FixedElements": [
{"Name": "header", "Offset": 0, "Size": 16}
],
"DynamicElementComposition": [
{"Name": "position", "Offset": 0, "Size": 12},
{"Name": "color", "Offset": 16, "Size": 16}
],
"InitialDynamicCount": 0
}
}
]
})";

		spk::Lumina::ShaderObjectFactory::Instanciator instanciator;
		instanciator->add(spk::JSON::Object::fromString(uboString));
		instanciator->add(spk::JSON::Object::fromString(samplerString));
		instanciator->add(spk::JSON::Object::fromString(ssboString));

		return (instanciator);
	}

	spk::Lumina::Shader MeshRenderer::Painter::_createShader()
	{
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

		spk::OpenGL::SamplerObject &sampler = spk::Lumina::ShaderObjectFactory::instance()->sampler(L"diffuseTexture");
		shader.addSampler(L"diffuseTexture", sampler, spk::Lumina::Shader::Mode::Attribute);

		spk::OpenGL::UniformBufferObject &cameraUBO = spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO");
		shader.addUBO(L"CameraUBO", cameraUBO, spk::Lumina::Shader::Mode::Constant);

		spk::OpenGL::UniformBufferObject &transformUBO = spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO");
		shader.addUBO(L"TransformUBO", transformUBO, spk::Lumina::Shader::Mode::Attribute);

		shader.UBO(L"CameraUBO")[L"viewMatrix"] = spk::Matrix4x4();
		shader.UBO(L"CameraUBO")[L"projectionMatrix"] = spk::Matrix4x4();
		shader.UBO(L"CameraUBO").validate();

		shader.UBO(L"TransformUBO")[L"modelMatrix"] = spk::Matrix4x4();
		shader.UBO(L"TransformUBO").validate();

		return (shader);
	}

	MeshRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_diffuseSampler(_shader.sampler(L"diffuseTexture"))
	{
	}

	void MeshRenderer::Painter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void MeshRenderer::Painter::prepare(const spk::Mesh &p_mesh)
	{
		const auto &buffer = p_mesh.buffer();
		_bufferSet.layout() << buffer.vertices;
		_bufferSet.indexes() << buffer.indexes;
	}

	void MeshRenderer::Painter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void MeshRenderer::Painter::render()
	{
		_object.render();
	}

	spk::Lumina::Shader &MeshRenderer::Painter::shader()
	{
		return (_shader);
	}

	spk::Lumina::Shader::Sampler &MeshRenderer::Painter::diffuseSampler()
	{
		return (_diffuseSampler);
	}

	MeshRenderer::MeshRenderer() :
		_painter()
	{
		_painter.diffuseSampler().bind(_texture);
	}

	void MeshRenderer::setTexture(spk::SafePointer<spk::Image> p_texture)
	{
		_texture = p_texture;
		_painter.diffuseSampler().bind(_texture);
	}

	spk::SafePointer<spk::Image> &MeshRenderer::texture()
	{
		return (_texture);
	}

	const spk::SafePointer<spk::Image> &MeshRenderer::texture() const
	{
		return (_texture);
	}

	spk::Lumina::Shader &MeshRenderer::shader()
	{
		return (_painter.shader());
	}

	MeshRenderer::Painter &MeshRenderer::painter()
	{
		return (_painter);
	}
}
