#include "rendering/command/draw_voxel_command.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

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
			program.bindShaderStorageBlock("Instances", InstanceBinding);
			program.bindShaderStorageBlock("Materials", MaterialBinding);
			program.bindShaderStorageBlock("MaterialRemap", MaterialRemapBinding);
			program.bindSampler("uAlbedo", AlbedoUnit);
			return program;
		}();
		return result;
	}

	std::vector<DrawVoxelCommand::MaterialData> DrawVoxelCommand::_resolveMaterials(
		const Palette &palette,
		Texture::Handle &atlas,
		std::optional<EngineView::Source> &source,
		const EngineView::CaptureContext *context)
	{
		std::vector<MaterialData> result;
		result.reserve(palette.materials().size());
		const Material::Binding *first = nullptr;
		for (const auto &[name, material] : palette.materials())
		{
			const auto *binding = material.find("albedo");
			if (binding == nullptr || !binding->valid())
			{
				throw spk::Exception("Voxel draw: missing albedo for '" + name + "'");
			}
			if (first != nullptr && !first->sameSource(*binding))
			{
				throw spk::Exception("Voxel draw: inconsistent albedo atlases");
			}
			first = binding;
			result.push_back({binding->section.anchor, binding->section.size});
		}
		if (first != nullptr && first->isViewSource())
		{
			const auto output = first->viewOutput.lock();
			if (!output || context == nullptr || !context->resolveSource)
			{
				throw spk::Exception("Voxel draw: EngineView source requires a valid capture");
			}
			source = context->resolveSource(output, first->viewIdentity);
		}
		else if (first != nullptr)
		{
			atlas = first->texture;
		}
		return result;
	}

	DrawVoxelCommand::DrawVoxelCommand(
		Voxel::Geometry geometry,
		const Palette &palette,
		std::vector<std::uint32_t> materialRemap,
		Matrix4x4 model,
		Matrix4x4 viewProjection,
		const EngineView::CaptureContext *context) :
		DrawVoxelCommand(
			std::move(geometry), palette, std::move(materialRemap), std::vector<Matrix4x4>{model}, viewProjection, context)
	{
	}

	DrawVoxelCommand::DrawVoxelCommand(
		Voxel::Geometry geometry,
		const Palette &palette,
		std::vector<std::uint32_t> materialRemap,
		std::vector<Matrix4x4> models,
		Matrix4x4 viewProjection,
		const EngineView::CaptureContext *context) :
		_geometry(std::move(geometry)),
		_program(_sharedProgram()),
		_instances(InstanceBinding, 0, sizeof(Matrix4x4)),
		_materials(MaterialBinding, 0, sizeof(MaterialData)),
		_materialRemap(MaterialRemapBinding, 0, sizeof(std::uint32_t)),
		_albedo(AlbedoUnit, Sampler::Filtering::Nearest, Sampler::Wrap::ClampToEdge, Sampler::MipmapFiltering::Disabled),
		_viewProjection(viewProjection)
	{
		if (models.empty())
		{
			throw spk::Exception("Voxel draw: at least one instance is required");
		}
		Texture::Handle atlas;
		const auto materials = _resolveMaterials(palette, atlas, _viewAlbedo, context);
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
		_instances.resize(models.size());
		_instances.setDynamicData(std::span<const Matrix4x4>(models));
		_instances.validate();
		if (!atlas && !_viewAlbedo)
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
		_instances.activate(context);
		_materials.activate(context);
		_materialRemap.activate(context);
		if (_viewAlbedo)
		{
			if (context.targetSurface == nullptr)
			{
				throw spk::Exception("Voxel draw: EngineView albedo requires a Surface");
			}
			Sampler resolved(AlbedoUnit, Sampler::Filtering::Nearest, Sampler::Wrap::ClampToEdge, Sampler::MipmapFiltering::Disabled);
			resolved.setTexture(_viewAlbedo->onSurface(*context.targetSurface));
			resolved.validate();
			resolved.activate(context);
		}
		else
		{
			_albedo.activate(context);
		}
		const auto &mesh = _geometry.opaqueMesh();
		mesh.layout().activate(context);
		_program.renderInstanced(
			Program::Primitive::Triangles, mesh.indexType(), 0, mesh.indexCount(), _instances.dynamicElementCount());
	}
}
