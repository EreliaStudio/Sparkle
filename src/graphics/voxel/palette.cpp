#include "voxel/palette.hpp"

#include <string>
#include <string_view>

#include "exception.hpp"
#include "voxel/geometry.hpp"

namespace spk::Voxel::Palette
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

	bool isCompatible(const Geometry &geometry, const spk::Palette &palette) noexcept
	{
		for (const auto &slot : geometry.materialSlots())
		{
			if (palette.find(slot.name) == nullptr)
			{
				return false;
			}
		}
		return true;
	}

	Builder::Builder() :
		spk::Palette::Builder(validateMaterial)
	{
	}

	Builder::Builder(const spk::Palette &palette) :
		spk::Palette::Builder(palette, validateMaterial)
	{
	}
}
