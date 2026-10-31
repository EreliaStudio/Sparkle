#include "voxel/shape.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	void Shape::Registry::add(ID id, Shape shape)
	{
		if (id.empty())
		{
			throw spk::Exception("Voxel Shape Registry requires a nonempty ID");
		}
		_insert(std::move(id), std::move(shape));
	}

	Shape::Registry::ID Shape::Registry::_parseKey(const spk::JSON::Reader &reader) const
	{
		const auto id = reader.require<std::string>("id");
		if (id.empty())
		{
			throw spk::Exception("Voxel Shape Registry requires a nonempty ID");
		}
		return id;
	}

	Shape Shape::Registry::_parseElement(const spk::JSON::Reader &reader) const
	{
		return Shape::Builder(reader.value()).build();
	}
}
