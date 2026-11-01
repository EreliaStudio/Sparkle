#include "voxel/palette.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	Palette::Palette(spk::Palette palette) :
		spk::Palette(std::move(palette))
	{
	}

	Palette::Builder::Builder(const Palette &palette) :
		spk::Palette::Builder(palette)
	{
		if (!palette.materials().empty())
		{
			_atlas = palette.materials().begin()->second.find("albedo")->texture.identifier();
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
		spk::Palette::Builder::setMaterial(std::move(name), std::move(material));
		_atlas = identity;
	}

	Palette Palette::Builder::build() const
	{
		return Palette(spk::Palette::Builder::build());
	}
}
