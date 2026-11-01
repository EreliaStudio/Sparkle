#include "graphics/voxel_palette.hpp"

#include <cmath>
#include <utility>

#include "exception.hpp"

namespace spk
{
	namespace
	{
		[[nodiscard]] bool hasAlbedo(const Material &material) noexcept
		{
			const auto *binding = material.find("albedo");
			return binding != nullptr && binding->texture.valid();
		}
	}

	VoxelPalette::VoxelPalette(Slots slots) :
		_slots(std::make_shared<const Slots>(std::move(slots)))
	{
	}

	VoxelPalette::VoxelPalette() :
		VoxelPalette(Slots{})
	{
	}

	const VoxelPalette::Slots &VoxelPalette::slots() const noexcept
	{
		return *_slots;
	}

	const VoxelPalette::Slot *VoxelPalette::find(std::string_view name) const noexcept
	{
		const auto found = _slots->find(name);
		return found == _slots->end() ? nullptr : &found->second;
	}

	VoxelPalette::Builder::Builder(const Voxel::Model &model)
	{
		addModel(model);
	}

	VoxelPalette::Builder::Builder(const VoxelPalette &palette)
	{
		for (const auto &[name, slot] : palette.slots())
		{
			_slots.emplace(name, Entry{slot.opacity, slot.material});
		}
	}

	void VoxelPalette::Builder::declareSlot(std::string name, float opacity)
	{
		if (name.empty())
		{
			throw Exception("VoxelPalette: semantic slot name cannot be empty");
		}
		if (!std::isfinite(opacity) || opacity < 0.0f || opacity > 1.0f)
		{
			throw Exception("VoxelPalette: slot '" + name + "' opacity must be finite and in [0,1]");
		}

		const auto [found, inserted] = _slots.try_emplace(std::move(name), Entry{opacity, std::nullopt});
		if (!inserted && found->second.opacity != opacity)
		{
			throw Exception("VoxelPalette: conflicting opacity for slot '" + found->first + "'");
		}
	}

	void VoxelPalette::Builder::addModel(const Voxel::Model &model)
	{
		Builder candidate(*this);
		for (const auto &slot : model.materialSlots())
		{
			candidate.declareSlot(slot.name, slot.opacity);
		}
		_slots.swap(candidate._slots);
	}

	void VoxelPalette::Builder::setMaterial(std::string_view name, Material material)
	{
		const auto found = _slots.find(name);
		if (found == _slots.end())
		{
			throw Exception("VoxelPalette: undeclared slot '" + std::string(name) + "'");
		}
		if (!hasAlbedo(material))
		{
			throw Exception("VoxelPalette: slot '" + found->first + "' requires a valid albedo");
		}
		found->second.material = std::move(material);
	}

	VoxelPalette VoxelPalette::Builder::build() const
	{
		Slots slots;
		for (const auto &[name, entry] : _slots)
		{
			if (!entry.material || !hasAlbedo(*entry.material))
			{
				throw Exception("VoxelPalette: missing valid albedo Material for slot '" + name + "'");
			}
			slots.emplace(name, Slot{entry.opacity, *entry.material});
		}
		return VoxelPalette(std::move(slots));
	}
}
