#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "voxel/model_element.hpp"
#include "voxel/render_batch.hpp"

namespace spk::Voxel
{
	class ModelElementView final
	{
	public:
		using ElementType = ModelElement;

	private:
		const ModelElement *_element;
		std::optional<Volume> _replacement;
		std::optional<Geometry> _replacementGeometry;
		std::optional<spk::Palette> _palette;
		std::vector<std::uint32_t> _materialRemap;

		[[nodiscard]] const Geometry &_geometry() const;

	public:
		explicit ModelElementView(const ModelElement &element);

		void bind(Volume volume);
		void bind(spk::Palette palette);
		void clearVolumeOverride();
		void clearPalette() noexcept;

		[[nodiscard]] const Geometry &geometry() const;
		[[nodiscard]] const spk::Palette *palette() const noexcept;
		[[nodiscard]] const std::vector<std::uint32_t> &materialRemap() const noexcept;

		void buildRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::Matrix4x4 &model);
		void buildRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::Matrix4x4 &model,
			RenderBatch &batch);
	};
}
