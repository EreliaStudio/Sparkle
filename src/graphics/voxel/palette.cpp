#include "voxel/palette.hpp"

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
		if (!palette.materials().materials().empty())
		{
			_atlas = palette.materials().materials().begin()->second.find("albedo")->texture.identifier();
		}
	}

	void Palette::Builder::setMaterial(std::string name, spk::Material material)
	{
		const auto *albedo = material.find("albedo");
		if (albedo == nullptr || !albedo->texture.valid())
		{
			throw spk::Exception("Voxel Palette: slot '" + name + "' requires a valid albedo");
		}
		const auto identity = albedo->texture.identifier();
		if (_atlas && *_atlas != identity)
		{
			throw spk::Exception("Voxel Palette: slot '" + name + "' uses a different albedo atlas");
		}
		_materials.setMaterial(std::move(name), std::move(material));
		_atlas = identity;
	}

	Palette Palette::Builder::build() const
	{
		return Palette(_materials.build());
	}
}
