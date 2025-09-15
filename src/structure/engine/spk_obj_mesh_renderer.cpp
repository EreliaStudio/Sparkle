#include "structure/engine/spk_obj_mesh_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

#include "spk_debug_macro.hpp"
#include "spk_generated_resources.hpp"
#include "structure/engine/spk_entity.hpp"

namespace spk
{
	spk::Lumina::Shader ObjMeshRenderer::Painter::_createShader()
	{
		if (spk::Lumina::ShaderObjectFactory::instance() == nullptr)
		{
			spk::Lumina::ShaderObjectFactory::instanciate();
		}
		spk::Lumina::ShaderObjectFactory::instance()->add(
			spk::JSON::Object::fromString(SPARKLE_GET_RESOURCE_AS_STRING("resources/json/mesh_renderer_shader_object.json")));

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
		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3});
		shader.addAttribute({1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});
		shader.addAttribute({2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3});

		shader.addSampler(
			L"diffuseTexture", spk::Lumina::ShaderObjectFactory::instance()->sampler(L"diffuseTexture"), spk::Lumina::Shader::Mode::Constant);

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);

		shader.addUBO(L"TransformUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO"), spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}

	spk::Lumina::Shader ObjMeshRenderer::Painter::_shader = ObjMeshRenderer::Painter::_createShader();

	ObjMeshRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_diffuseSampler(_shader.sampler(L"diffuseTexture")),
		_transformUBO(_object.ubo(L"TransformUBO"))
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

	void ObjMeshRenderer::Painter::setTransform(const spk::Transform &p_transform)
	{
		_transformUBO.layout()[L"modelMatrix"] = p_transform.model();
		_transformUBO.validate();
	}

	void ObjMeshRenderer::Painter::setTexture(const spk::SafePointer<const spk::Texture> &p_texture)
	{
		_diffuseSampler.bind(p_texture);
	}

	const spk::SafePointer<const spk::Texture> &ObjMeshRenderer::Painter::texture() const
	{
		return (_diffuseSampler.texture());
	}

	ObjMeshRenderer::ObjMeshRenderer(const std::wstring &p_name) :
		spk::Component(p_name),
		_painter()
	{
	}

	void ObjMeshRenderer::setTexture(spk::SafePointer<const spk::Texture> p_texture)
	{
		_painter.setTexture(p_texture);
	}

	const spk::SafePointer<const spk::Texture> &ObjMeshRenderer::texture() const
	{
		return (_painter.texture());
	}

	void ObjMeshRenderer::setMesh(const spk::SafePointer<const spk::ObjMesh> &p_mesh)
	{
		_mesh = p_mesh;
		_onMeshMaterialChangeContract = spk::ObjMesh::MaterialChangeContract();

		_painter.clear();
		if (_mesh != nullptr)
		{
			_painter.prepare(*_mesh);
		}
		_painter.validate();
	}

	const spk::SafePointer<const spk::ObjMesh> &ObjMeshRenderer::mesh() const
	{
		return (_mesh);
	}

	void ObjMeshRenderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		_painter.render();
	}

	void ObjMeshRenderer::start()
	{
		_onOwnerTransformEditionContract = owner()->transform().addOnEditionCallback([this]() { _painter.setTransform(owner()->transform()); });
		_onOwnerTransformEditionContract.trigger();
	}
}
