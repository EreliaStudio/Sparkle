#include "voxel/definition.hpp"

#include <cmath>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	struct Definition::Data
	{
		Shape shape;
		SlotMapping slots;
		OpacityMapping opacity;
	};

	Definition::Definition() :
		_data(air()._data)
	{
	}

	Definition::Definition(Shape shape, SlotMapping mapping, OpacityMapping opacity)
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

		for (const auto &[key, value] : opacity)
		{
			(void)shape.slot(key);
			if (!std::isfinite(value) || value < 0.0f || value > 1.0f)
			{
				throw spk::Exception("Voxel Definition opacity must be finite and in [0,1]");
			}
		}

		auto data = std::make_shared<Data>();
		data->shape = std::move(shape);
		data->slots = std::move(mapping);
		data->opacity = std::move(opacity);
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

	float Definition::opacity(std::string_view geometricKey) const
	{
		(void)shape().slot(geometricKey);
		const auto found = _data->opacity.find(std::string(geometricKey));
		return found == _data->opacity.end() ? 1.0f : found->second;
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
