#include "voxel/palette.hpp"

#include <optional>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	Palette::Palette(spk::Palette materials) :
		_materials(std::move(materials))
	{
	}

	const spk::Palette &Palette::materials() const noexcept
	{
		return _materials;
	}

	const spk::Material *Palette::find(std::string_view name) const noexcept
	{
		return _materials.find(name);
	}

	Palette::Builder::Builder(const Palette &palette) :
		_materials(palette.materials())
	{
	}

	void Palette::Builder::setMaterial(std::string name, spk::Material material)
	{
		const auto *albedo = material.find("albedo");
		if (albedo == nullptr || !albedo->texture.valid())
		{
			throw spk::Exception("Voxel Palette: slot '" + name + "' requires a valid albedo");
		}
		_materials.setMaterial(std::move(name), std::move(material));
	}

	Palette Palette::Builder::build() const
	{
		spk::Palette materials = _materials.build();
		std::optional<spk::GPUResource::Identifier> atlas;
		for (const auto &[name, material] : materials.materials())
		{
			const auto identity = material.find("albedo")->texture.identifier();
			if (atlas && *atlas != identity)
			{
				throw spk::Exception("Voxel Palette: slot '" + name + "' uses a different albedo atlas");
			}
			atlas = identity;
		}
		return Palette(std::move(materials));
	}
}
