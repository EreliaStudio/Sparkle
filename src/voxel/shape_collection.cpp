#include "voxel/shape.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	Shape::Collection::Collection() :
		_data(std::make_shared<const Data>())
	{
	}

	Shape::Collection::Collection(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	const Shape &Shape::Collection::at(const ID &id) const
	{
		const auto found = _data->shapes.find(id);
		if (found == _data->shapes.end())
		{
			throw spk::Exception("unknown Voxel Shape Collection ID: " + id);
		}
		return found->second;
	}

	const Shape &Shape::Collection::operator[](const ID &id) const
	{
		return at(id);
	}

	const Shape *Shape::Collection::tryGet(const ID &id) const noexcept
	{
		const auto found = _data->shapes.find(id);
		return found == _data->shapes.end() ? nullptr : &found->second;
	}

	bool Shape::Collection::contains(const ID &id) const noexcept
	{
		return _data->shapes.contains(id);
	}

	std::size_t Shape::Collection::size() const noexcept
	{
		return _data->shapes.size();
	}

	void Shape::Collection::Builder::add(ID id, Shape shape)
	{
		if (id.empty())
		{
			throw spk::Exception("Voxel Shape Collection requires a nonempty ID");
		}
		_insert(std::move(id), std::move(shape));
	}

	Shape::Collection Shape::Collection::Builder::build() const
	{
		return Collection(std::make_shared<const Data>(Data{_entries()}));
	}

	Shape::Collection::Builder::ID Shape::Collection::Builder::_parseKey(const spk::JSON::Reader &reader) const
	{
		return reader.require<std::string>(
			"id",
			[](const std::string &id) {
				return !id.empty();
			},
			"Voxel Shape Collection requires a nonempty ID");
	}

	Shape Shape::Collection::Builder::_parseElement(const spk::JSON::Reader &reader) const
	{
		Shape::Builder builder;
		builder.addFromJSON(reader.value());
		return builder.build();
	}
}
