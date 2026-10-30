#include "voxel/definition.hpp"

#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	struct Definition::Data
	{
		Shape shape;
		SlotMappings slots;
	};

	Definition::Definition() :
		_data(air()._data)
	{
	}

	Definition::Definition(Shape shape, SlotMappings mapping)
	{
		if (mapping.size() != shape.slots().size())
		{
			throw spk::Exception("Voxel Definition must map every Shape slot exactly once");
		}

		for (const auto &slot : shape.slots())
		{
			if (!mapping.contains(slot.name))
			{
				throw spk::Exception("Voxel Definition has a missing or repeated Shape slot mapping");
			}
		}
		for (const auto &[key, value] : mapping)
		{
			if (value.empty())
			{
				throw spk::Exception("Voxel Definition semantic slot value cannot be empty");
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

	const Definition::SlotMappings &Definition::slots() const noexcept
	{
		return _data->slots;
	}

	const std::string &Definition::semanticSlot(std::string_view geometric) const
	{
		const auto iterator = _data->slots.find(std::string(geometric));
		if (iterator != _data->slots.end())
		{
			return iterator->second;
		}
		throw spk::Exception("Voxel Definition references an unmapped geometric slot");
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
