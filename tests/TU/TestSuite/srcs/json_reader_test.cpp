#include <gtest/gtest.h>

#include "container/json/reader.hpp"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace json_reader_test
{
	struct Point
	{
		int x = 0;
		int y = 0;
		bool operator==(const Point &) const = default;
	};

	void fromJSON(const spk::JSON::Value &value, Point &point)
	{
		point.x = value.at("x").as<int>();
		point.y = value.at("y").as<int>();
	}
}

namespace
{
	enum class Mode
	{
		Windowed,
		Fullscreen
	};

	[[nodiscard]] spk::JSON::Value makeDocument()
	{
		spk::JSON::Value root = spk::JSON::Value::object();
		root["name"] = "Sparkle";
		root["enabled"] = true;
		root["count"] = 4;
		root["ratio"] = 1.5;
		root["numbers"] = spk::JSON::Value::array();
		root["numbers"].pushBack(1);
		root["numbers"].pushBack(2);
		root["numbers"].pushBack(3);
		root["fixed"] = spk::JSON::Value::array();
		root["fixed"].pushBack(8);
		root["fixed"].pushBack(9);
		root["mode"] = "windowed";
		root["point"] = spk::JSON::Value::object();
		root["point"]["x"] = 5;
		root["point"]["y"] = 6;
		root["child"] = spk::JSON::Value::object();
		root["child"]["value"] = 12;
		root["children"] = spk::JSON::Value::array();
		spk::JSON::Value first = spk::JSON::Value::object();
		first["id"] = 1;
		spk::JSON::Value second = spk::JSON::Value::object();
		second["id"] = 2;
		root["children"].pushBack(std::move(first));
		root["children"].pushBack(std::move(second));
		return root;
	}
}

TEST(JSONReaderTest, StandardUsageReadsScalarsSequencesEnumsChildrenAndCustomTypes)
{
	const spk::JSON::Value document = makeDocument();
	const spk::JSON::Reader reader(document, "config.json");

	EXPECT_EQ(reader.require<std::string>("name"), "Sparkle");
	EXPECT_TRUE(reader.require<bool>("enabled"));
	EXPECT_EQ(reader.require<int>("count"), 4);
	EXPECT_DOUBLE_EQ(reader.require<double>("ratio"), 1.5);
	EXPECT_EQ(reader.require<std::vector<int>>("numbers"), (std::vector<int>{1, 2, 3}));
	EXPECT_EQ((reader.require<std::array<int, 2>>("fixed")), (std::array<int, 2>{8, 9}));
	EXPECT_EQ(reader.require<json_reader_test::Point>("point"), (json_reader_test::Point{5, 6}));

	const std::map<std::string, Mode> modes = {
		{"fullscreen", Mode::Fullscreen},
		{"windowed", Mode::Windowed}};
	EXPECT_EQ(reader.requireEnum<Mode>("mode", modes), Mode::Windowed);

	const spk::JSON::Reader child = reader.child("child");
	EXPECT_EQ(child.require<int>("value"), 12);
	const std::vector<spk::JSON::Reader> children = reader.childArray("children");
	ASSERT_EQ(children.size(), 2u);
	EXPECT_EQ(children[0].require<int>("id"), 1);
	EXPECT_EQ(children[1].require<int>("id"), 2);
}

TEST(JSONReaderTest, OptionalDefaultsApplyOnlyWhenFieldIsAbsent)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["present"] = 3;
	document["wrong"] = "not an integer";
	const spk::JSON::Reader reader(document, "config.json");

	EXPECT_EQ(reader.optional<int>("missing", 99), 99);
	EXPECT_EQ(reader.optional<int>("present", 99), 3);
	EXPECT_THROW((void)reader.optional<int>("wrong", 99), spk::JSON::Error);
}

TEST(JSONReaderTest, ExactFixedArrayArityIsRequired)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["short"] = spk::JSON::Value::array();
	document["short"].pushBack(1);
	document["long"] = spk::JSON::Value::array();
	document["long"].pushBack(1);
	document["long"].pushBack(2);
	document["long"].pushBack(3);
	const spk::JSON::Reader reader(document, "config.json");

	EXPECT_THROW(((void)reader.require<std::array<int, 2>>("short")), spk::JSON::Error);
	EXPECT_THROW(((void)reader.require<std::array<int, 2>>("long")), spk::JSON::Error);
}

TEST(JSONReaderTest, FilePathValueContainsAndPathExtensionAreExposed)
{
	const spk::JSON::Value document = makeDocument();
	const spk::JSON::Reader reader(document, "assets/config.json", "$.settings");

	EXPECT_EQ(reader.file(), "assets/config.json");
	EXPECT_EQ(reader.path(), "$.settings");
	EXPECT_EQ(&reader.value(), &document);
	EXPECT_TRUE(reader.contains("name"));
	EXPECT_FALSE(reader.contains("missing"));
	EXPECT_EQ(reader.pathFor("name"), "$.settings.name");

	const auto child = reader.child("child");
	EXPECT_EQ(child.path(), "$.settings.child");
	const auto children = reader.childArray("children");
	ASSERT_EQ(children.size(), 2u);
	EXPECT_EQ(children[0].path(), "$.settings.children[0]");
	EXPECT_EQ(children[1].path(), "$.settings.children[1]");
}

TEST(JSONReaderTest, ForbidUnknownAcceptsExactAllowedSet)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["name"] = "Sparkle";
	document["count"] = 2;
	const spk::JSON::Reader reader(document, "config.json");
	EXPECT_NO_THROW(reader.forbidUnknown({"name", "count"}));
}

TEST(JSONReaderTest, MissingRequiredMemberReportsExactFailingPath)
{
	const spk::JSON::Value document = makeDocument();
	const spk::JSON::Reader reader(document, "config.json", "$.root");

	try
	{
		(void)reader.require<int>("missing");
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.file(), "config.json");
		EXPECT_EQ(error.path(), "$.root.missing");
		EXPECT_FALSE(error.message().empty());
	}
}

TEST(JSONReaderTest, WrongScalarContainerAndCustomTypesReportMemberPath)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["number"] = "wrong";
	document["vector"] = 1;
	document["point"] = 1;
	const spk::JSON::Reader reader(document, "config.json");

	for (const std::string key : {"number", "vector", "point"})
	{
		try
		{
			if (key == "number")
				(void)reader.require<int>(key);
			else if (key == "vector")
				(void)reader.require<std::vector<int>>(key);
			else
				(void)reader.require<json_reader_test::Point>(key);
			FAIL() << "Expected spk::JSON::Error for " << key;
		}
		catch (const spk::JSON::Error &error)
		{
			EXPECT_EQ(error.path(), std::string("$.") + key);
		}
	}
}

TEST(JSONReaderTest, FixedArrayWrongArityReportsExactMemberPath)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["fixed"] = spk::JSON::Value::array();
	document["fixed"].pushBack(1);
	const spk::JSON::Reader reader(document, "config.json", "$.root");

	try
	{
		(void)reader.require<std::array<int, 2>>("fixed");
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.path(), "$.root.fixed");
		EXPECT_NE(error.message().find("expected exactly 2 elements"), std::string::npos);
	}
}

TEST(JSONReaderTest, UnknownEnumListsKnownValuesAndReportsExactPath)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["mode"] = "borderless";
	const spk::JSON::Reader reader(document, "config.json");
	const std::map<std::string, Mode> modes = {
		{"fullscreen", Mode::Fullscreen},
		{"windowed", Mode::Windowed}};

	try
	{
		(void)reader.requireEnum<Mode>("mode", modes);
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.path(), "$.mode");
		EXPECT_NE(error.message().find("borderless"), std::string::npos);
		EXPECT_NE(error.message().find("fullscreen"), std::string::npos);
		EXPECT_NE(error.message().find("windowed"), std::string::npos);
	}
}

TEST(JSONReaderTest, ChildWithWrongShapeReportsChildPath)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["child"] = 12;
	const spk::JSON::Reader reader(document, "config.json");

	try
	{
		(void)reader.child("child");
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.path(), "$.child");
	}
}

TEST(JSONReaderTest, ChildArrayRejectsNonObjectElementAtIndexedPath)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["children"] = spk::JSON::Value::array();
	document["children"].pushBack(spk::JSON::Value::object());
	document["children"].pushBack(12);
	const spk::JSON::Reader reader(document, "config.json");

	try
	{
		(void)reader.childArray("children");
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.path(), "$.children[1]");
	}
}

TEST(JSONReaderTest, ObjectOperationsOnScalarThrowJsonError)
{
	const spk::JSON::Value scalar(42);
	const spk::JSON::Reader reader(scalar, "config.json", "$.scalar");

	EXPECT_THROW((void)reader.contains("key"), spk::JSON::Error);
	EXPECT_THROW((void)reader.optional<int>("key", 0), spk::JSON::Error);
	EXPECT_THROW(reader.forbidUnknown({}), spk::JSON::Error);
}

TEST(JSONReaderTest, UnknownKeyIsRejected)
{
	spk::JSON::Value document = spk::JSON::Value::object();
	document["known"] = 1;
	document["unknown"] = 2;
	const spk::JSON::Reader reader(document, "config.json", "$.root");

	try
	{
		reader.forbidUnknown({"known"});
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.path(), "$.root.unknown");
		EXPECT_NE(error.message().find("unknown"), std::string::npos);
	}
}
