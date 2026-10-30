#include "voxel/definition.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	struct Definition::Data
	{
		Shape shape;
		SlotMapping slots;
	};

	Definition::Definition() :
		_data(air()._data)
	{
	}

	Definition::Definition(Shape shape, SlotMapping mapping)
	{
		if (mapping.size() != shape.slots().size())
		{
			throw spk::Exception("Voxel Definition must map every Shape slot exactly once");
		}
		for (const auto &slot : shape.slots())
		{
			if (!mapping.contains(slot.name))
			{
				throw spk::Exception("Voxel Definition has a missing Shape slot mapping");
			}
		}
		for (const auto &[key, value] : mapping)
		{
			if (value.empty())
			{
				throw spk::Exception("Voxel Definition semantic slot name cannot be empty");
			}
			(void)shape.slot(key);
		}

		auto data = std::make_shared<Data>();
		data->shape = std::move(shape);
		data->slots = std::move(mapping);
		_data = std::move(data);
	}

	const Definition &Definition::air()
	{
		static const Definition result(Shape::Builder{}.build(), {});
		return result;
	}

	const Shape &Definition::shape() const noexcept
	{
		return _data->shape;
	}

	const Definition::SlotMapping &Definition::slots() const noexcept
	{
		return _data->slots;
	}

	const std::string &Definition::semanticSlot(std::string_view geometricKey) const
	{
		const auto found = _data->slots.find(std::string(geometricKey));
		if (found == _data->slots.end())
		{
			throw spk::Exception("Voxel Definition references an unmapped geometric slot");
		}
		return found->second;
	}

	bool Definition::operator==(const Definition &other) const noexcept
	{
		return _data == other._data;
	}

	bool Definition::isAir() const noexcept
	{
		return *this == air();
	}
}
