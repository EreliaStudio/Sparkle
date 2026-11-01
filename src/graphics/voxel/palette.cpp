#include "voxel/palette.hpp"

#include <utility>

#include "exception.hpp"
#include "voxel/model.hpp"

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

	Palette::Builder::Builder(const Model &model)
	{
		addModel(model);
	}

	Palette::Builder::Builder(const Palette &palette) :
		_materials(palette.materials())
	{
		for (const auto &[name, material] : palette.materials().materials())
		{
			(void)material;
			_required.insert(name);
		}
	}

	void Palette::Builder::addModel(const Model &model)
	{
		auto required = _required;
		for (const auto &slot : model.materialSlots())
		{
			required.insert(slot.name);
		}
		_required.swap(required);
	}

	void Palette::Builder::setMaterial(std::string_view name, spk::Material material)
	{
		if (!_required.contains(name))
		{
			throw spk::Exception("Voxel Palette: undeclared slot '" + std::string(name) + "'");
		}
		const auto *albedo = material.find("albedo");
		if (albedo == nullptr || !albedo->texture.valid())
		{
			throw spk::Exception("Voxel Palette: slot '" + std::string(name) + "' requires a valid albedo");
		}
		_materials.setMaterial(std::string(name), std::move(material));
	}

	Palette Palette::Builder::build() const
	{
		const auto materials = _materials.build();
		for (const auto &name : _required)
		{
			const auto *material = materials.find(name);
			if (material == nullptr || material->find("albedo") == nullptr ||
				!material->find("albedo")->texture.valid())
			{
				throw spk::Exception("Voxel Palette: missing valid albedo Material for slot '" + name + "'");
			}
		}
		return Palette(std::move(materials));
	}
}
