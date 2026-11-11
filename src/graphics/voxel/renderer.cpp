#include "voxel/renderer.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"

namespace spk::Voxel
{
	Renderer::Renderer(const std::string &name, spk::Entity3D *owner) :
		RenderingComponent3D(name, owner)
	{
	}

	void Renderer::_validate(const Geometry &geometry, const spk::Palette &palette)
	{
		std::string missing;
		spk::GPUResource::Identifier atlas = 0;
		for (const auto &slot : geometry.materialSlots())
		{
			const auto *material = palette.find(slot.name);
			if (material == nullptr)
			{
				if (!missing.empty())
				{
					missing += ", ";
				}
				missing += slot.name;
				continue;
			}
			const auto *albedo = material->find("albedo");
			if (albedo == nullptr || !albedo->texture)
			{
				throw spk::Exception("Voxel Renderer: slot '" + slot.name + "' has no valid albedo");
			}
			const auto id = albedo->texture.identifier();
			if (atlas != 0 && atlas != id)
			{
				throw spk::Exception("Voxel Renderer: incompatible albedo atlases");
			}
			atlas = id;
		}
		if (!missing.empty())
		{
			throw spk::Exception("Voxel Renderer: missing material slots: " + missing);
		}
	}

	std::vector<std::uint32_t> Renderer::resolveMaterialRemap(const Geometry &geometry, const spk::Palette &palette)
	{
		_validate(geometry, palette);
		std::vector<std::uint32_t> remap(geometry.materialSlots().size());
		std::uint32_t paletteIndex = 0;
		for (const auto &[name, material] : palette.materials())
		{
			(void)material;
			const auto found = geometry.materialIndices().find(name);
			if (found != geometry.materialIndices().end())
			{
				remap[found->second] = paletteIndex;
			}
			if (paletteIndex == std::numeric_limits<std::uint32_t>::max())
			{
				throw spk::Exception("Voxel Renderer: too many palette materials");
			}
			++paletteIndex;
		}
		return remap;
	}

	void Renderer::bind(Geometry geometry)
	{
		std::vector<std::uint32_t> remap;
		if (_palette)
		{
			remap = resolveMaterialRemap(geometry, *_palette);
		}
		_geometry = std::move(geometry);
		_materialRemap = std::move(remap);
	}

	void Renderer::bind(spk::Palette palette)
	{
		std::vector<std::uint32_t> remap;
		if (_geometry)
		{
			remap = resolveMaterialRemap(*_geometry, palette);
		}
		_palette = std::move(palette);
		_materialRemap = std::move(remap);
	}

	void Renderer::clearGeometry() noexcept
	{
		_geometry.reset();
		_materialRemap.clear();
	}

	void Renderer::clearPalette() noexcept
	{
		_palette.reset();
		_materialRemap.clear();
	}

	const Geometry *Renderer::geometry() const noexcept
	{
		return _geometry ? &*_geometry : nullptr;
	}

	const spk::Palette *Renderer::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}

	const std::vector<std::uint32_t> &Renderer::materialRemap() const noexcept
	{
		return _materialRemap;
	}
}
