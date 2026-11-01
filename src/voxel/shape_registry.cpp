#include "voxel/shape.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	Shape::Registry::Registry() :
		_data(std::make_shared<const Data>())
	{
	}

	Shape::Registry::Registry(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	const Shape &Shape::Registry::at(const ID &id) const
	{
		const auto found = _data->shapes.find(id);
		if (found == _data->shapes.end())
		{
			throw spk::Exception("unknown Voxel Shape Registry ID: " + id);
		}
		return found->second;
	}

	const Shape &Shape::Registry::operator[](const ID &id) const
	{
		return at(id);
	}

	const Shape *Shape::Registry::tryGet(const ID &id) const noexcept
	{
		const auto found = _data->shapes.find(id);
		return found == _data->shapes.end() ? nullptr : &found->second;
	}

	bool Shape::Registry::contains(const ID &id) const noexcept
	{
		return _data->shapes.contains(id);
	}

	std::size_t Shape::Registry::size() const noexcept
	{
		return _data->shapes.size();
	}

	void Shape::Registry::Builder::add(ID id, Shape shape)
	{
		if (id.empty())
		{
			throw spk::Exception("Voxel Shape Registry requires a nonempty ID");
		}
		_insert(std::move(id), std::move(shape));
	}

	Shape::Registry Shape::Registry::Builder::build() const
	{
		return Registry(std::make_shared<const Data>(Data{_entries()}));
	}

	Shape::Registry::Builder::ID Shape::Registry::Builder::_parseKey(const spk::JSON::Reader &reader) const
	{
		return reader.require<std::string>(
			"id",
			[](const std::string &id) { return !id.empty(); },
			"Voxel Shape Registry requires a nonempty ID");
	}

	Shape Shape::Registry::Builder::_parseElement(const spk::JSON::Reader &reader) const
	{
		Shape::Builder builder;
		builder.addFromJSON(reader.value());
		return builder.build();
	}
}
