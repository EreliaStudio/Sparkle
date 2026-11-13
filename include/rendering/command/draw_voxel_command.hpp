#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include "graphics/opengl/program.hpp"
#include "graphics/opengl/sampler.hpp"
#include "graphics/opengl/shader_storage_buffer.hpp"
#include "lighting/system.hpp"
#include "math/matrix.hpp"
#include "rendering/command/camera_uniform_render_command.hpp"
#include "rendering/render_command.hpp"
#include "voxel/geometry.hpp"

namespace spk
{
	class DrawVoxelCommand final : public RenderCommand
	{
	private:
		struct alignas(8) MaterialData
		{
			Vector2 uvAnchor;
			Vector2 uvSize;
		};

		static_assert(sizeof(MaterialData) == 16);
		static_assert(alignof(MaterialData) == 8);
		static_assert(offsetof(MaterialData, uvAnchor) == 0);
		static_assert(offsetof(MaterialData, uvSize) == 8);
		static_assert(std::is_standard_layout_v<MaterialData>);

		static constexpr std::size_t InstanceBinding = 0;
		static constexpr std::size_t MaterialBinding = 1;
		static constexpr std::size_t MaterialRemapBinding = 2;
		static constexpr std::size_t AlbedoUnit = 0;

		Voxel::Geometry _geometry;
		Program _program;
		ShaderStorageBuffer _instances;
		ShaderStorageBuffer _materials;
		ShaderStorageBuffer _materialRemap;
		Sampler _albedo;
		std::shared_ptr<const Lighting::System> _lighting;
		std::shared_ptr<const CameraUniformRenderCommand> _camera;

		[[nodiscard]] static Program &_sharedProgram();
		[[nodiscard]] static std::vector<MaterialData> _resolveMaterials(
			const Palette &palette, Texture::Handle &atlas);

	public:
		DrawVoxelCommand(
			Voxel::Geometry geometry,
			const Palette &palette,
			std::vector<std::uint32_t> materialRemap,
			std::vector<Matrix4x4> models,
			std::shared_ptr<const Lighting::System> lighting,
			std::shared_ptr<const CameraUniformRenderCommand> camera = {});
		DrawVoxelCommand(
			Voxel::Geometry geometry,
			const Palette &palette,
			std::vector<std::uint32_t> materialRemap,
			Matrix4x4 model,
			std::shared_ptr<const Lighting::System> lighting,
			std::shared_ptr<const CameraUniformRenderCommand> camera = {});

		void execute(RenderContext &context) const override;
	};
}
