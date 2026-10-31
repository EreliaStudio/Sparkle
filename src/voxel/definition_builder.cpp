#include "voxel/definition.hpp"

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
	}

	void Definition::Builder::addFromJSON(const spk::JSON::Value &value)
	{
		spk::JSON::Reader reader(value, "<definition>");
		reader.forbidUnknown({"shape", "slots"});
		Shape shape = _shapes.at(reader.require<std::string>("shape"));
		SlotMapping slots;
		for (const auto &[key, value] : reader.child("slots").value().asObject())
		{
			if (value.isString())
			{
				slots.emplace(key, Slot{value.as<std::string>()});
				continue;
			}
			spk::JSON::Reader slotReader(value, reader.file(), reader.pathFor("slots") + "." + key);
			slotReader.forbidUnknown({"name", "opacity"});
			slots.emplace(key, Slot{
				slotReader.require<std::string>("name"),
				slotReader.optional<float>("opacity", 1.0f)});
		}
		(void)Definition(shape, slots);
		_shape = std::move(shape);
		_slots.swap(slots);
	}

	Definition Definition::Builder::build() const
	{
		if (!_shape)
		{
			throw spk::Exception("Voxel Definition Builder requires a Shape");
		}
		return Definition(*_shape, _slots);
	}
}
