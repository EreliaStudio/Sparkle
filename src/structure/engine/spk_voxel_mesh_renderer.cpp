#include "structure/engine/spk_voxel_mesh_renderer.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/math/spk_matrix.hpp"

#include "spk_generated_resources.hpp"
#include "structure/engine/spk_entity.hpp"

namespace spk
{
	spk::Lumina::Shader VoxelMeshRenderer::Painter::_createShader()
	{
		spk::Lumina::ShaderObjectFactory::instance()->add(
			spk::JSON::Object::fromString(SPARKLE_GET_RESOURCE_AS_STRING("resources/json/mesh_renderer_shader_object.json")));

		const char *vertexShaderSrc = R"(#version 450
layout(location = 0) in vec3 inPosition;
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

		// shader.setVerboseMode(true);

		shader.addAttribute({0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector3});
		shader.addAttribute({1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2});

		shader.addSampler(
			L"diffuseTexture", spk::Lumina::ShaderObjectFactory::instance()->sampler(L"diffuseTexture"), spk::Lumina::Shader::Mode::Constant);

		shader.addUBO(L"CameraUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);

		shader.addUBO(L"TransformUBO", spk::Lumina::ShaderObjectFactory::instance()->ubo(L"TransformUBO"), spk::Lumina::Shader::Mode::Attribute);

		return (shader);
	}

	spk::Lumina::Shader VoxelMeshRenderer::Painter::_shader = VoxelMeshRenderer::Painter::_createShader();

	VoxelMeshRenderer::Painter::Painter() :
		_object(_shader.createObject()),
		_bufferSet(_object.bufferSet()),
		_diffuseSampler(_shader.sampler(L"diffuseTexture")),
		_transformUBO(_object.UBO(L"TransformUBO"))
	{
	}

	void VoxelMeshRenderer::Painter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void VoxelMeshRenderer::Painter::prepare(const VoxelMesh &p_mesh)
	{
		const auto &buffer = p_mesh.buffer();
		_bufferSet.layout() << buffer.vertices;
		_bufferSet.indexes() << buffer.indexes;
	}

	void VoxelMeshRenderer::Painter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void VoxelMeshRenderer::Painter::render()
	{
		_object.render();
	}

	void VoxelMeshRenderer::Painter::setTransform(const spk::Transform &p_transform)
	{
		_transformUBO.layout()[L"modelMatrix"] = p_transform.model();
		_transformUBO.validate();
	}

	void VoxelMeshRenderer::Painter::setTexture(const spk::SafePointer<const spk::Texture> &p_texture)
	{
		_diffuseSampler.bind(p_texture);
	}

	const spk::SafePointer<const spk::Texture> &VoxelMeshRenderer::Painter::texture() const
	{
		return (_diffuseSampler.texture());
	}

	VoxelMeshRenderer::VoxelMeshRenderer(const std::wstring &p_name) :
		spk::Component(p_name),
		_painter()
	{
	}

	void VoxelMeshRenderer::setTexture(spk::SafePointer<const spk::Texture> p_texture)
	{
		_painter.setTexture(p_texture);
	}

	const spk::SafePointer<const spk::Texture> &VoxelMeshRenderer::texture() const
	{
		return (_painter.texture());
	}

	void VoxelMeshRenderer::setMesh(const spk::SafePointer<const VoxelMesh> &p_mesh)
	{
		_mesh = p_mesh;

		_painter.clear();
		if (_mesh != nullptr)
		{
			_painter.prepare(*_mesh);
		}
		_painter.validate();
	}

	const spk::SafePointer<const VoxelMesh> &VoxelMeshRenderer::mesh() const
	{
		return (_mesh);
	}

	void VoxelMeshRenderer::onPaintEvent(spk::PaintEvent &p_event)
	{
		_painter.render();
	}

	void VoxelMeshRenderer::start()
	{
		_onOwnerTransformEditionContract = owner()->transform().addOnEditionCallback([this]() {
			_painter.setTransform(owner()->transform());
		});
		_onOwnerTransformEditionContract.trigger();
	}
}
