#include "voxel/definition.hpp"

#include <string>
#include <utility>

#include "exception.hpp"
#include "voxel/volume.hpp"

namespace spk::Voxel
{
	Definition::Collection::Collection() :
		_data(std::make_shared<const Data>())
	{
	}

	Definition::Collection::Collection(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	const Definition &Definition::Collection::at(ID id) const
	{
		if (id == 0)
		{
			return Definition::air();
		}
		const auto found = _data->definitions.find(id);
		if (found == _data->definitions.end())
		{
			throw spk::Exception("unknown Voxel Definition Collection ID: " + std::to_string(id));
		}
		return found->second;
	}

	const Definition &Definition::Collection::operator[](ID id) const
	{
		return at(id);
	}

	const Definition *Definition::Collection::tryGet(ID id) const noexcept
	{
		if (id == 0)
		{
			return &Definition::air();
		}
		const auto found = _data->definitions.find(id);
		return found == _data->definitions.end() ? nullptr : &found->second;
	}

	bool Definition::Collection::contains(ID id) const noexcept
	{
		return id == 0 || _data->definitions.contains(id);
	}

	std::size_t Definition::Collection::size() const noexcept
	{
		return _data->definitions.size() + 1;
	}

	void Definition::Collection::Builder::bind(const Shape::Collection &shapes)
	{
		_shapes = shapes;
	}

	void Definition::Collection::Builder::add(ID id, Definition definition)
	{
		if (id == 0 || id > Volume::PackedCell::MaxDefinitionId || definition.isAir())
		{
			throw spk::Exception("Voxel Definition Collection requires a non-Air Definition and a representable nonzero ID");
		}
		_insert(id, std::move(definition));
	}

	Definition::Collection Definition::Collection::Builder::build() const
	{
		return Collection(std::make_shared<const Data>(Data{_entries()}));
	}

	Definition::Collection::Builder::ID Definition::Collection::Builder::_parseKey(const spk::JSON::Reader &reader) const
	{
		return reader.require<ID>(
			"id",
			[](ID id) {
				return id > 0 && id <= Volume::PackedCell::MaxDefinitionId;
			},
			"Voxel Definition Collection JSON ID must be nonzero and fit 29 bits");
	}

	Definition Definition::Collection::Builder::_parseElement(const spk::JSON::Reader &reader) const
	{
		Definition::Builder builder;
		builder.bind(_shapes);
		builder.loadFromJSON(reader.value());
		return builder.build();
	}
}
