#include "voxel/definition.hpp"

#include <algorithm>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	struct Definition::Data
	{
		Shape shape;
		std::vector<SlotMapping> slots;
	};

	Definition::Definition() :
		_data(air()._data)
	{
	}

	Definition::Definition(Shape shape, std::vector<SlotMapping> mapping)
	{
		if (mapping.size() != shape.slots().size())
		{
			throw spk::Exception("Voxel Definition must map every Shape slot exactly once");
		}

		for (const auto &slot : shape.slots())
		{
			const auto count = std::count_if(mapping.begin(), mapping.end(), [&](const SlotMapping &entry) {
				return entry.geometric == slot.name;
			});
			if (count != 1)
			{
				throw spk::Exception("Voxel Definition has a missing or repeated Shape slot mapping");
			}
		}
		for (const auto &entry : mapping)
		{
			if (entry.semantic.empty())
			{
				throw spk::Exception("Voxel Definition semantic slot name cannot be empty");
			}
			(void)shape.slot(entry.geometric);
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

	const std::vector<Definition::SlotMapping> &Definition::slots() const noexcept
	{
		return _data->slots;
	}

	const std::string &Definition::semanticSlot(std::string_view geometric) const
	{
		for (const auto &entry : _data->slots)
		{
			if (entry.geometric == geometric)
			{
				return entry.semantic;
			}
		}
		throw spk::Exception("Voxel Definition references an unmapped geometric slot");
	}

	bool Definition::sameIdentity(const Definition &other) const noexcept
	{
		return _data == other._data;
	}

	bool Definition::isAir() const noexcept
	{
		return sameIdentity(air());
	}
}
