#include "rendering/command/draw_voxel_command.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "exception.hpp"
#include "graphics/internal/resource.hpp"
#include "graphics/opengl/state.hpp"
#include "graphics/opengl/uniform.hpp"

namespace spk
{
	Program &DrawVoxelCommand::_sharedProgram()
	{
		static Program result = [] {
			Program program(
				std::string(resources::text("shaders/voxel_opaque.vert.glsl")),
				std::string(resources::text("shaders/voxel_opaque.frag.glsl")));
			program.bindShaderStorageBlock("Materials", MaterialBinding);
			program.bindShaderStorageBlock("MaterialRemap", MaterialRemapBinding);
			program.bindSampler("uAlbedo", AlbedoUnit);
			return program;
		}();
		return result;
	}

	std::vector<DrawVoxelCommand::MaterialData> DrawVoxelCommand::_resolveMaterials(
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

	DrawVoxelCommand::DrawVoxelCommand(
		Voxel::Geometry geometry,
		const Palette &palette,
		std::vector<std::uint32_t> materialRemap,
		Matrix4x4 model,
		Matrix4x4 viewProjection) :
		_geometry(std::move(geometry)),
		_program(_sharedProgram()),
		_materials(MaterialBinding, 0, sizeof(MaterialData)),
		_materialRemap(MaterialRemapBinding, 0, sizeof(std::uint32_t)),
		_albedo(AlbedoUnit, Sampler::Filtering::Nearest, Sampler::Wrap::ClampToEdge, Sampler::MipmapFiltering::Disabled),
		_viewProjection(viewProjection),
		_model(model)
	{
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

	void DrawVoxelCommand::execute(RenderContext &context) const
	{
		if (_geometry.opaqueMesh().empty())
		{
			return;
		}
		OpenGL::State::DepthTest{}.apply(context);
		OpenGL::State::Blend::disabled(context).apply(context);
		OpenGL::State::Rasterization{}.apply(context);
		_program.activate(context);
		Uniform<Matrix4x4>("uViewProjection", _viewProjection).activate(context);
		Uniform<Matrix4x4>("uModel", _model).activate(context);
		_materials.activate(context);
		_materialRemap.activate(context);
		_albedo.activate(context);
		const auto &mesh = _geometry.opaqueMesh();
		mesh.layout().activate(context);
		_program.render(Program::Primitive::Triangles, mesh.indexType(), 0, mesh.indexCount());
	}
}
