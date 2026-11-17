#include "rendering/command/draw_voxel_command.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "graphics/internal/resource.hpp"
#include "graphics/opengl/state.hpp"

namespace spk
{
	Program &DrawVoxelCommand::_sharedProgram(Mode mode)
	{
		const auto build = [](Mode variant) {
			std::string fragment(resources::text("shaders/voxel_opaque.frag.glsl"));
			if (variant == Mode::Translucent)
			{
				const auto versionEnd = fragment.find('\n');
				if (versionEnd == std::string::npos)
				{
					throw spk::Exception("Voxel draw: fragment shader is missing its GLSL version");
				}
				fragment.insert(versionEnd + 1, "#define SPK_VOXEL_TRANSLUCENT 1\n");
			}
			Program program(
				std::string(resources::text("shaders/voxel_opaque.vert.glsl")),
				std::move(fragment));
			program.bindShaderStorageBlock("Instances", InstanceBinding);
			program.bindShaderStorageBlock("Materials", MaterialBinding);
			program.bindShaderStorageBlock("MaterialRemap", MaterialRemapBinding);
			if (variant == Mode::Translucent)
			{
				program.bindShaderStorageBlock("SlotOpacities", SlotOpacityBinding);
			}
			program.bindShaderStorageBlock("ForwardLighting", Lighting::System::BindingPoint);
			program.bindShaderStorageBlock("ForwardShadows", Lighting::System::ShadowBindingPoint);
			for (std::size_t i = 0; i < Lighting::System::MaxShadowSamplers; ++i)
			{
				program.bindSampler("uShadowMaps[" + std::to_string(i) + "]", 1 + i);
			}
			for (std::size_t i = 0; i < Lighting::System::MaxPointShadowSamplers; ++i)
			{
				program.bindSampler(
					"uPointShadowMaps[" + std::to_string(i) + "]",
					Lighting::System::PointShadowBindingBase + i);
			}
			program.bindSampler("uAlbedo", AlbedoUnit);
			return program;
		};
		static Program opaque = build(Mode::Opaque);
		static Program translucent = build(Mode::Translucent);
		return mode == Mode::Translucent ? translucent : opaque;
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
		std::shared_ptr<const Lighting::System> lighting,
		std::shared_ptr<const CameraUniformRenderCommand> camera,
		Mode mode) :
		DrawVoxelCommand(
			std::move(geometry), palette, std::move(materialRemap), std::vector<Matrix4x4>{model}, std::move(lighting), std::move(camera), mode)
	{
	}

	DrawVoxelCommand::DrawVoxelCommand(
		Voxel::Geometry geometry,
		const Palette &palette,
		std::vector<std::uint32_t> materialRemap,
		std::vector<Matrix4x4> models,
		std::shared_ptr<const Lighting::System> lighting,
		std::shared_ptr<const CameraUniformRenderCommand> camera,
		Mode mode) :
		_mode(mode),
		_geometry(std::move(geometry)),
		_program(_sharedProgram(mode)),
		_instances(InstanceBinding, 0, sizeof(Matrix4x4)),
		_materials(MaterialBinding, 0, sizeof(MaterialData)),
		_materialRemap(MaterialRemapBinding, 0, sizeof(std::uint32_t)),
		_slotOpacities(SlotOpacityBinding, 0, sizeof(float)),
		_albedo(AlbedoUnit, Sampler::Filtering::Nearest, Sampler::Wrap::ClampToEdge, Sampler::MipmapFiltering::Disabled),
		_lighting(std::move(lighting)),
		_camera(std::move(camera))
	{
		if (!_lighting)
		{
			throw spk::Exception("Voxel draw: lighting system is required");
		}
		if (models.empty())
		{
			throw spk::Exception("Voxel draw: at least one instance is required");
		}
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
		if (_mode == Mode::Translucent)
		{
			std::vector<float> opacities;
			opacities.reserve(_geometry.materialSlots().size());
			for (const auto &slot : _geometry.materialSlots())
			{
				opacities.push_back(slot.opacity);
			}
			_slotOpacities.resize(opacities.size());
			_slotOpacities.setDynamicData(std::span<const float>(opacities));
			_slotOpacities.validate();
		}
		_materials.resize(materials.size());
		_materials.setDynamicData(std::span<const MaterialData>(materials));
		_materials.validate();
		_instances.resize(models.size());
		_instances.setDynamicData(std::span<const Matrix4x4>(models));
		_instances.validate();
		if (!atlas)
		{
			throw spk::Exception("Voxel draw: no albedo atlas");
		}
		_albedo.setTexture(std::move(atlas));
		_albedo.validate();
	}

	void DrawVoxelCommand::execute(RenderContext &context) const
	{
		const auto &mesh = _mode == Mode::Opaque ? _geometry.opaqueMesh() : _geometry.transparentMesh();
		if (mesh.empty())
		{
			return;
		}
		if (_mode == Mode::Opaque)
		{
			OpenGL::State::DepthTest{}.apply(context);
			OpenGL::State::Blend::disabled(context).apply(context);
		}
		else
		{
			OpenGL::State::DepthTest{.write = false}.apply(context);
			auto blend = OpenGL::State::Blend::disabled(context);
			blend.attachments[0] = {.enabled = true, .sourceRGB = OpenGL::State::BlendFactor::One, .destinationRGB = OpenGL::State::BlendFactor::One, .sourceAlpha = OpenGL::State::BlendFactor::One, .destinationAlpha = OpenGL::State::BlendFactor::One};
			blend.attachments[1] = {.enabled = true, .sourceRGB = OpenGL::State::BlendFactor::Zero, .destinationRGB = OpenGL::State::BlendFactor::OneMinusSourceColor, .sourceAlpha = OpenGL::State::BlendFactor::Zero, .destinationAlpha = OpenGL::State::BlendFactor::OneMinusSourceColor};
			blend.apply(context);
		}
		OpenGL::State::Rasterization{}.apply(context);
		if (_camera)
		{
			_camera->execute(context);
		}
		_program.activate(context);
		_instances.activate(context);
		_materials.activate(context);
		_materialRemap.activate(context);
		if (_mode == Mode::Translucent)
		{
			_slotOpacities.activate(context);
		}
		_lighting->activate(context);
		const auto shadows = _lighting->activateShadows(context);
		_albedo.activate(context);
		mesh.layout().activate(context);
		_program.renderInstanced(
			Program::Primitive::Triangles, mesh.indexType(), 0, mesh.indexCount(), _instances.dynamicElementCount());
	}
}
