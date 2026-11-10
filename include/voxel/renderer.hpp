#pragma once

#include <optional>
#include <string>

#include "engine/registry.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "voxel/geometry.hpp"

namespace spk::Voxel
{
	class Renderer final : public spk::RenderingComponent3D,
						   public spk::Registry<Renderer, spk::EngineHeadless *>::Object
	{
	private:
		std::optional<Geometry> _geometry;
		std::optional<spk::Palette> _palette;

		static void _validate(const Geometry &geometry, const spk::Palette &palette);

	public:
		explicit Renderer(const std::string &name = "Voxel renderer", spk::Entity3D *owner = nullptr);

		void bind(Geometry geometry);
		void bind(spk::Palette palette);
		void clearGeometry() noexcept;
		void clearPalette() noexcept;

		[[nodiscard]] const Geometry *geometry() const noexcept;
		[[nodiscard]] const spk::Palette *palette() const noexcept;
	};
}
