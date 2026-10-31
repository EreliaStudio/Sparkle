#include "voxel/definition.hpp"

#include <string>
#include <utility>

#include "exception.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	Definition::Registry::Registry() :
		_data(std::make_shared<const Data>())
	{
	}

	Definition::Registry::Registry(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	const Definition &Definition::Registry::at(ID id) const
	{
		if (id == 0)
		{
			return Definition::air();
		}
		const auto found = _data->definitions.find(id);
		if (found == _data->definitions.end())
		{
			throw spk::Exception("unknown Voxel Definition Registry ID: " + std::to_string(id));
		}
		return found->second;
	}

	const Definition &Definition::Registry::operator[](ID id) const
	{
		return at(id);
	}

	const Definition *Definition::Registry::tryGet(ID id) const noexcept
	{
		if (id == 0)
		{
			return &Definition::air();
		}
		const auto found = _data->definitions.find(id);
		return found == _data->definitions.end() ? nullptr : &found->second;
	}

	bool Definition::Registry::contains(ID id) const noexcept
	{
		return id == 0 || _data->definitions.contains(id);
	}

	std::size_t Definition::Registry::size() const noexcept
	{
		return _data->definitions.size() + 1;
	}

	void Definition::Registry::Builder::bind(const Shape::Registry &shapes)
	{
		_shapes = shapes;
	}

	void Definition::Registry::Builder::add(ID id, Definition definition)
	{
		if (id == 0 || id > Volume::PackedCell::MaxDefinitionId || definition.isAir())
		{
			throw spk::Exception("Voxel Definition Registry requires a non-Air Definition and a representable nonzero ID");
		}
		_insert(id, std::move(definition));
	}

	Definition::Registry Definition::Registry::Builder::build() const
	{
		return Registry(std::make_shared<const Data>(Data{_entries()}));
	}

	Definition::Registry::Builder::ID Definition::Registry::Builder::_parseKey(const spk::JSON::Reader &reader) const
	{
		const auto id = reader.require<ID>("id");
		if (id == 0 || id > Volume::PackedCell::MaxDefinitionId)
		{
			throw spk::Exception("Voxel Definition Registry JSON ID must be nonzero and fit 29 bits");
		}
		return id;
	}

	Definition Definition::Registry::Builder::_parseElement(const spk::JSON::Reader &reader) const
	{
		Definition::Builder builder;
		builder.bind(_shapes);
		builder.addFromJSON(reader.value());
		return builder.build();
	}
}
