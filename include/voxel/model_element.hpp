#pragma once

#include <optional>
#include <string>

#include "model3d/blueprint.hpp"
#include "voxel/geometry.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	class ModelElement final : public spk::Model3D::Blueprint::Element
	{
	private:
		Volume _volume;
		mutable std::optional<Geometry> _geometry;

	public:
		ModelElement(std::string name, Volume volume);
		ModelElement(ModelElement &&) noexcept = default;
		ModelElement &operator=(ModelElement &&) = delete;

		void setVolume(Volume volume);
		[[nodiscard]] const Volume &volume() const noexcept;
		[[nodiscard]] const Geometry &geometry() const;
	};
}
