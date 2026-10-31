#include "voxel/definition_registry.hpp"

#include <utility>

#include "exception.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	void DefinitionRegistry::add(std::uint32_t id, Definition definition)
	{
		if (id == 0 || id > Volume::PackedCell::MaxDefinitionIndex || definition.isAir())
		{
			throw spk::Exception("Voxel DefinitionRegistry requires a non-Air Definition and a representable nonzero ID");
		}
		if (!_definitions.emplace(id, std::move(definition)).second)
		{
			throw spk::Exception("Voxel DefinitionRegistry ID is already registered");
		}
	}

	const Definition &DefinitionRegistry::at(std::uint32_t id) const
	{
		if (id == 0)
		{
			return Definition::air();
		}
		const auto entry = _definitions.find(id);
		if (entry == _definitions.end())
		{
			throw spk::Exception("Voxel DefinitionRegistry ID is not registered");
		}
		return entry->second;
	}

	const Definition &DefinitionRegistry::operator[](std::uint32_t id) const
	{
		return at(id);
	}

	bool DefinitionRegistry::contains(std::uint32_t id) const noexcept
	{
		return id == 0 || _definitions.contains(id);
	}

	std::size_t DefinitionRegistry::size() const noexcept
	{
		return _definitions.size() + 1;
	}
}
