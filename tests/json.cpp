#include "json_reader.hpp"

#include <array>
#include <cassert>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

int main()
{
	using spk::JSON::Reader;
	using spk::JSON::Value;

	const Value document = Value::fromString(R"({
		"enabled": true,
		"name": "Sparkle \u2728",
		"position": [12, -4],
		"items": [{"id": 1}, {"id": 2}]
	})");
	const Reader reader(document, "memory.json");
	assert(reader.require<bool>("enabled"));
	assert(reader.require<std::string>("name") == "Sparkle \xE2\x9C\xA8");
	assert((reader.require<std::array<int, 2>>("position") == std::array<int, 2>{12, -4}));
	assert(reader.optional<int>("missing", 42) == 42);
	assert(reader.childArray("items")[1].require<int>("id") == 2);

	const std::string compact = document.toString({.pretty = false});
	assert(Value::fromString(compact) == document);

	bool rejectedDuplicate = false;
	try
	{
		(void)Value::fromString(R"({"key": 1, "key": 2})");
	} catch (const std::runtime_error &)
	{
		rejectedDuplicate = true;
	}
	assert(rejectedDuplicate);

	bool rejectedUnknown = false;
	try
	{
		reader.forbidUnknown({"enabled", "name", "position"});
	} catch (const spk::JSON::Error &error)
	{
		rejectedUnknown = error.path() == "$.items";
	}
	assert(rejectedUnknown);
}
