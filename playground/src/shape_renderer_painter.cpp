#include "shape.hpp"

namespace taag
{
	spk::Lumina::Shader Shape::Renderer::Painter::_createShader()
	{
		spk::Lumina::ShaderObjectFactory::instance()->add(spk::JSON::Object::fromString(LR"({
	"UBO": [
		{
			"name": "CameraUBO",
			"data": {
				"BlockName": "CameraUBO",
				"BindingPoint": 0,
				"Size": 128,
				"Elements": [
					{
						"Name": "viewMatrix",
						"Offset": 0,
						"Size": 64
					},
					{
						"Name": "projectionMatrix",
						"Offset": 64,
						"Size": 64
					}
				]
			}
		}
	],
	"SSBO": [
	]
})"));
		const char *vertexShaderSrc = R"(#version 450

layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform CameraUBO
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
} cameraUBO;

struct Info
{
    mat4 model;
    vec4 color;
};

layout(std430, binding = 3) buffer InfoSSBO
{
    Info object[];
};

void main()
{
    gl_Position = cameraUBO.projectionMatrix * cameraUBO.viewMatrix * object[gl_InstanceID].model * vec4(inPosition, 0.0, 1.0);

    fragColor = object[gl_InstanceID].color;
})";

		const char *fragmentShaderSrc = R"(#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = fragColor;
})";

		spk::Lumina::Shader shader(vertexShaderSrc, fragmentShaderSrc);

		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);

		spk::OpenGL::ShaderStorageBufferObject infoSSBO = spk::OpenGL::ShaderStorageBufferObject(L"InfoSSBO", 3, 0, 0, 80, 0);
		infoSSBO.dynamicArray().addElement(L"model", 0, 64);
		infoSSBO.dynamicArray().addElement(L"color", 64, 16);

		shader.addSSBO(L"InfoSSBO", infoSSBO, spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}

	void Shape::Renderer::Painter::_prepare(const Mesh &p_mesh)
	{
		const auto &buffer = p_mesh.buffer();

		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();

		_bufferSet.layout() << buffer.vertices;
		_bufferSet.indexes() << buffer.indexes;

		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	Shape::Renderer::Painter::Painter(const Mesh &p_mesh) :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_infoSSBO(_object.ssbo(L"InfoSSBO"))
	{
		_prepare(p_mesh);
	}

	void Shape::Renderer::Painter::render()
	{
		size_t nbInstance = _infoSSBO.dynamicArray().nbElement();

		_object.setNbInstance(nbInstance);
		_object.render();
	}

	void Shape::Renderer::Painter::setShapeList(const spk::SafePointer<const InfoContainer> p_infoContainer)
	{
		_infoContainer = p_infoContainer;
	}

	void Shape::Renderer::Painter::validate()
	{
		if (_infoContainer == nullptr)
		{
			return;
		}

		auto &array = _infoSSBO.dynamicArray();

		array.resize(_infoContainer->size());

		size_t index = 0;
		for (const auto &info : *_infoContainer)
		{
			array[index] = info;
			index++;
		}

		_infoSSBO.validate();
	}
}