#include "voxel/palette_builder.hpp"

#include <string>
#include <string_view>

#include "exception.hpp"

namespace spk::Voxel
{
	namespace
	{
		void validateMaterial(
			std::string_view name,
			const spk::Material &material,
			const spk::Palette::Materials &existing)
		{
			const auto *albedo = material.find("albedo");
			if (albedo == nullptr || !albedo->texture.valid())
			{
				throw spk::Exception("Voxel Palette: slot '" + std::string(name) + "' requires a valid albedo");
			}
			for (const auto &[otherName, other] : existing)
			{
				if (otherName == name)
				{
					continue;
				}
				const auto *otherAlbedo = other.find("albedo");
				if (otherAlbedo == nullptr || !otherAlbedo->texture.valid() ||
					otherAlbedo->texture.identifier() != albedo->texture.identifier())
				{
					throw spk::Exception("Voxel Palette: slot '" + std::string(name) + "' uses a different albedo atlas");
				}
			}
		}
	}

	PaletteBuilder::PaletteBuilder() :
		spk::Palette::Builder(validateMaterial)
	{
	}

	PaletteBuilder::PaletteBuilder(const spk::Palette &palette) :
		spk::Palette::Builder(palette, validateMaterial)
	{
	}
}
