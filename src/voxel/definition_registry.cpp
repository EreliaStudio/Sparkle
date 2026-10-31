#include "voxel/definition.hpp"

#include <utility>

#include "exception.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	Definition::Registry::Registry(const Shape::Registry &shapes)
	{
		bind(shapes);
	}

	void Definition::Registry::bind(const Shape::Registry &shapes)
	{
		_shapes = std::make_shared<const Shape::Registry>(shapes);
	}

	void Definition::Registry::add(ID id, Definition definition)
	{
		if (id == 0 || id > Volume::PackedCell::MaxDefinitionIndex || definition.isAir())
		{
			throw spk::Exception("Voxel Definition Registry requires a non-Air Definition and a representable nonzero ID");
		}
		_insert(id, std::move(definition));
	}

	const Definition &Definition::Registry::at(const ID &id) const
	{
		return id == 0 ? Definition::air() : spk::JSON::Catalog<Definition, ID>::at(id);
	}

	const Definition &Definition::Registry::operator[](const ID &id) const
	{
		return at(id);
	}

	bool Definition::Registry::contains(const ID &id) const noexcept
	{
		return id == 0 || spk::JSON::Catalog<Definition, ID>::contains(id);
	}

	std::size_t Definition::Registry::size() const noexcept
	{
		return spk::JSON::Catalog<Definition, ID>::size() + 1;
	}

	Definition::Registry::ID Definition::Registry::_parseKey(const spk::JSON::Reader &reader) const
	{
		const auto id = reader.require<ID>("id");
		if (id == 0 || id > Volume::PackedCell::MaxDefinitionIndex)
		{
			throw spk::Exception("Voxel Definition Registry JSON ID must be nonzero and fit 29 bits");
		}
		return id;
	}

	Definition Definition::Registry::_parseElement(const spk::JSON::Reader &reader) const
	{
		if (!_shapes)
		{
			throw spk::Exception("Voxel Definition Registry requires a bound Shape Registry to load JSON");
		}
		return Definition::Builder(*_shapes, reader.value()).build();
	}
}
