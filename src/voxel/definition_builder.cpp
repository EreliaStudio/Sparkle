#include "voxel/definition.hpp"

#include <cmath>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel
{
	void Definition::Builder::bind(const Shape::Registry &shapes)
	{
		_shape.reset();
		_shapes = shapes;
	}

	void Definition::Builder::setShape(std::string_view id)
	{
		_shape = _shapes.at(std::string(id));
	}

	void Definition::Builder::setSlots(SlotMapping slots)
	{
		_slots.swap(slots);
		_opacity.swap(opacity);
	}

	void Definition::Builder::setOpacity(std::string_view geometricKey, float opacity)
	{
		if (!std::isfinite(opacity) || opacity < 0.0f || opacity > 1.0f)
		{
			throw spk::Exception("Voxel Definition opacity must be finite and in [0,1]");
		}
		_opacity[std::string(geometricKey)] = opacity;
	}

	void Definition::Builder::addFromJSON(const spk::JSON::Value &value)
	{
		spk::JSON::Reader reader(value, "<definition>");
		reader.forbidUnknown({"shape", "slots", "opacity"});
		Shape shape = _shapes.at(reader.require<std::string>("shape"));
		SlotMapping slots;
		for (const auto &[name, target] : reader.child("slots").value().asObject())
		{
			slots.emplace(name, target.as<std::string>());
		}
		OpacityMapping opacity;
		if (reader.value().contains("opacity"))
		{
			for (const auto &[name, value] : reader.child("opacity").value().asObject())
			{
				opacity.emplace(name, value.as<float>());
			}
		}
		(void)Definition(shape, slots, opacity);
		_shape = std::move(shape);
		_slots.swap(slots);
	}

	Definition Definition::Builder::build() const
	{
		if (!_shape)
		{
			throw spk::Exception("Voxel Definition Builder requires a Shape");
		}
		return Definition(*_shape, _slots, _opacity);
	}
}
