#include "voxel/renderer.hpp"

#include <string>
#include <utility>

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

	void Renderer::bind(Geometry geometry)
	{
		if (_palette)
		{
			_validate(geometry, *_palette);
		}
		_geometry = std::move(geometry);
	}

	void Renderer::bind(spk::Palette palette)
	{
		if (_geometry)
		{
			_validate(*_geometry, palette);
		}
		_palette = std::move(palette);
	}

	void Renderer::clearGeometry() noexcept
	{
		_geometry.reset();
	}

	void Renderer::clearPalette() noexcept
	{
		_palette.reset();
	}

	const Geometry *Renderer::geometry() const noexcept
	{
		return _geometry ? &*_geometry : nullptr;
	}

	const spk::Palette *Renderer::palette() const noexcept
	{
		return _palette ? &*_palette : nullptr;
	}
}
