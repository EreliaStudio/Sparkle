#include "rendering/command/draw_voxel_bucket_command.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "exception.hpp"
#include "graphics/internal/resource.hpp"
#include "graphics/opengl/state.hpp"
#include "graphics/opengl/uniform.hpp"

namespace spk
{
	static_assert(sizeof(DrawVoxelBucketCommand::InstanceData) == 64);
	static_assert(alignof(DrawVoxelBucketCommand::InstanceData) == 16);
	static_assert(offsetof(DrawVoxelBucketCommand::InstanceData, model) == 0);
	static_assert(sizeof(DrawVoxelBucketCommand::MaterialData) == 16);
	static_assert(alignof(DrawVoxelBucketCommand::MaterialData) == 8);
	static_assert(offsetof(DrawVoxelBucketCommand::MaterialData, uvAnchor) == 0);
	static_assert(offsetof(DrawVoxelBucketCommand::MaterialData, uvSize) == 8);
	static_assert(std::is_standard_layout_v<DrawVoxelBucketCommand::MaterialData>);

	Program &DrawVoxelBucketCommand::_sharedProgram()
	{
		static Program result = [] {
			Program program(
				std::string(resources::text("shaders/voxel_opaque.vert.glsl")),
				std::string(resources::text("shaders/voxel_opaque.frag.glsl")));
			program.bindShaderStorageBlock("Instances", InstanceBinding);
			program.bindShaderStorageBlock("Materials", MaterialBinding);
			program.bindShaderStorageBlock("MaterialRemap", MaterialRemapBinding);
			program.bindSampler("uAlbedo", AlbedoUnit);
			return program;
		}();
		return result;
	}

	std::vector<DrawVoxelBucketCommand::MaterialData> DrawVoxelBucketCommand::_resolveMaterials(
		const Palette &palette, Texture::Handle &atlas)
	{
		std::vector<MaterialData> result;
		result.reserve(palette.materials().size());
		for (const auto &[name, material] : palette.materials())
		{
			const auto *binding = material.find("albedo");
			if (binding == nullptr || !binding->texture)
			{
				throw spk::Exception("Voxel draw: missing albedo for '" + name + "'");
			}
			if (atlas && atlas.identifier() != binding->texture.identifier())
			{
				throw spk::Exception("Voxel draw: inconsistent albedo atlases");
			}
			atlas = binding->texture;
			result.push_back({binding->section.anchor, binding->section.size});
		}
		return result;
	}

	DrawVoxelBucketCommand::DrawVoxelBucketCommand(
		Voxel::Geometry geometry,
		const Palette &palette,
		std::vector<std::uint32_t> materialRemap,
		std::vector<Matrix4x4> matrices,
		Matrix4x4 viewProjection) :
		_geometry(std::move(geometry)),
		_program(_sharedProgram()),
		_instances(InstanceBinding, 0, sizeof(InstanceData)),
		_materials(MaterialBinding, 0, sizeof(MaterialData)),
		_materialRemap(MaterialRemapBinding, 0, sizeof(std::uint32_t)),
		_albedo(AlbedoUnit, Sampler::Filtering::Nearest, Sampler::Wrap::ClampToEdge, Sampler::MipmapFiltering::Disabled),
		_viewProjection(viewProjection),
		_instanceCount(matrices.size())
	{
		if (_instanceCount == 0)
		{
			throw spk::Exception("Voxel draw: empty instance bucket");
		}
		std::vector<InstanceData> packed;
		packed.reserve(matrices.size());
		for (const Matrix4x4 &matrix : matrices)
		{
			packed.push_back({matrix});
		}
		_instances.resize(packed.size());
		_instances.setDynamicData(std::span<const InstanceData>(packed));
		_instances.validate();
		Texture::Handle atlas;
		const auto materials = _resolveMaterials(palette, atlas);
		if (materialRemap.size() != _geometry.materialSlots().size())
		{
			throw spk::Exception("Voxel draw: material remap has an unexpected size");
		}
		for (const std::uint32_t index : materialRemap)
		{
			if (index >= materials.size())
			{
				throw spk::Exception("Voxel draw: material remap index out of range");
			}
		}
		_materialRemap.resize(materialRemap.size());
		_materialRemap.setDynamicData(std::span<const std::uint32_t>(materialRemap));
		_materialRemap.validate();
		_materials.resize(materials.size());
		_materials.setDynamicData(std::span<const MaterialData>(materials));
		_materials.validate();
		if (!atlas)
		{
			throw spk::Exception("Voxel draw: no albedo atlas");
		}
		_albedo.setTexture(std::move(atlas));
		_albedo.validate();
	}

	std::size_t DrawVoxelBucketCommand::instanceCount() const noexcept
	{
		return _instanceCount;
	}

	void DrawVoxelBucketCommand::execute(RenderContext &context) const
	{
		if (_geometry.opaqueMesh().empty())
		{
			return;
		}
		OpenGL::State::DepthTest{}.apply();
		OpenGL::State::Blend::disabled().apply();
		OpenGL::State::Rasterization{}.apply();
		_program.activate(context);
		Uniform<Matrix4x4>("uViewProjection", _viewProjection).activate(context);
		_instances.activate(context);
		_materials.activate(context);
		_materialRemap.activate(context);
		_albedo.activate(context);
		const auto &mesh = _geometry.opaqueMesh();
		mesh.layout().activate(context);
		_program.renderInstanced(Program::Primitive::Triangles, mesh.indexType(), 0, mesh.indexCount(), _instanceCount);
	}
}
