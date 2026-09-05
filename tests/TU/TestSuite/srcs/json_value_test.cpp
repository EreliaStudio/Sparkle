#include <gtest/gtest.h>

#include "container/json/object.hpp"
#include "sparkle_test.hpp"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace json_value_test
{
	struct Point
	{
		int x = 0;
		int y = 0;
		bool operator==(const Point &) const = default;
	};

	spk::JSON::Value toJSON(const Point &point)
	{
		spk::JSON::Value result = spk::JSON::Value::object();
		result["x"] = point.x;
		result["y"] = point.y;
		return result;
	}

	void fromJSON(const spk::JSON::Value &value, Point &point)
	{
		point.x = value.at("x").as<int>();
		point.y = value.at("y").as<int>();
	}
}

namespace
{
	[[nodiscard]] bool contains(std::string_view text, std::string_view fragment)
	{
		return text.find(fragment) != std::string_view::npos;
	}
}

TEST(JSONValueTest, StandardUsageRoundTripsMixedDocumentAndCustomType)
{
	spk::JSON::Value root = spk::JSON::Value::object();
	root["null"] = nullptr;
	root["boolean"] = true;
	root["integer"] = -42;
	root["floating"] = 3.5;
	root["string"] = "hello";
	root["custom"] = json_value_test::Point{4, 7};
	root["array"] = spk::JSON::Value::array();
	root["array"].pushBack(1);
	root["array"].pushBack("two");
	root["array"].pushBack(false);

	const std::string serialized = root.toString({.pretty = false});
	const spk::JSON::Value parsed = spk::JSON::Value::fromString(serialized);

	EXPECT_TRUE(parsed.at("null").isNull());
	EXPECT_TRUE(parsed.at("boolean").as<bool>());
	EXPECT_EQ(parsed.at("integer").as<int>(), -42);
	EXPECT_DOUBLE_EQ(parsed.at("floating").as<double>(), 3.5);
	EXPECT_EQ(parsed.at("string").as<std::string>(), "hello");
	EXPECT_EQ(parsed.at("custom").as<json_value_test::Point>(), (json_value_test::Point{4, 7}));
	ASSERT_EQ(parsed.at("array").size(), 3u);
	EXPECT_EQ(parsed.at("array").at(0).as<int>(), 1);
	EXPECT_EQ(parsed.at("array").at(1).as<std::string>(), "two");
	EXPECT_FALSE(parsed.at("array").at(2).as<bool>());
}

TEST(JSONValueTest, ScalarTypesBoundariesAndPredicatesAreConsistent)
{
	spk::JSON::Value value;
	EXPECT_TRUE(value.isNull());
	EXPECT_EQ(value.type(), spk::JSON::Value::Type::Null);

	value = true;
	EXPECT_TRUE(value.isBoolean());
	EXPECT_TRUE(value.holds<bool>());

	value = std::numeric_limits<std::int64_t>::min();
	EXPECT_TRUE(value.isInteger());
	EXPECT_TRUE(value.isNumber());
	EXPECT_EQ(value.as<std::int64_t>(), std::numeric_limits<std::int64_t>::min());

	value = std::numeric_limits<std::int64_t>::max();
	EXPECT_EQ(value.as<std::uint64_t>(), static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
	EXPECT_FALSE(value.canAs<std::int32_t>());

	const std::uint64_t largestSupportedUnsigned = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
	value = largestSupportedUnsigned;
	EXPECT_EQ(value.as<std::uint64_t>(), largestSupportedUnsigned);

	value = 1.25;
	EXPECT_TRUE(value.isFloating());
	EXPECT_TRUE(value.isNumber());
	EXPECT_FLOAT_EQ(value.as<float>(), 1.25f);

	value = "text";
	EXPECT_TRUE(value.isString());
	EXPECT_EQ(value.as<std::string>(), "text");
}

TEST(JSONValueTest, StringsEscapesUnicodeWhitespaceAndBomAreParsed)
{
	const auto value = spk::JSON::Value::fromString(
		" \t\r\n{\"text\":\"quote: \\\" slash: \\\\ solidus: \\/ backspace: \\b formfeed: \\f newline: \\n return: \\r tab: \\t unicode: \\u20AC\"} \n");
	const std::string text = value.at("text").as<std::string>();
	EXPECT_TRUE(contains(text, "quote: \""));
	EXPECT_TRUE(contains(text, "slash: \\"));
	EXPECT_TRUE(contains(text, "solidus: /"));
	EXPECT_TRUE(contains(text, "unicode: €"));

	const std::string bomDocument = std::string("\xEF\xBB\xBF") + "{\"ok\":true}";
	EXPECT_TRUE(spk::JSON::Value::fromString(bomDocument).at("ok").as<bool>());

	spk::JSON::ParseOptions options;
	options.allowUtf8Bom = false;
	EXPECT_THROW((void)spk::JSON::Value::fromString(bomDocument, options), std::runtime_error);
}

TEST(JSONValueTest, EmptyNestedContainersLookupAppendResizeAndResetBehaveAsContainers)
{
	spk::JSON::Value root = spk::JSON::Value::object();
	EXPECT_TRUE(root.empty());
	EXPECT_FALSE(root.contains("missing"));
	EXPECT_EQ(root.count("missing"), 0u);
	EXPECT_EQ(root.find("missing"), nullptr);

	root["nested"] = spk::JSON::Value::array();
	root["nested"].append() = 1;
	root["nested"].pushBack(2);
	root["nested"].resize(4);
	ASSERT_EQ(root["nested"].size(), 4u);
	EXPECT_EQ(root["nested"].at(0).as<int>(), 1);
	EXPECT_EQ(root["nested"].at(1).as<int>(), 2);
	EXPECT_TRUE(root["nested"].at(2).isNull());
	EXPECT_TRUE(root["nested"].at(3).isNull());

	root.reset();
	EXPECT_TRUE(root.isNull());
	root.resetToObject();
	EXPECT_TRUE(root.isObject());
	root.resetToArray();
	EXPECT_TRUE(root.isArray());
	root.resetToNull();
	EXPECT_TRUE(root.isNull());
}

TEST(JSONValueTest, EqualityAndCompactPrettyFormattingAreObservable)
{
	spk::JSON::Value left = spk::JSON::Value::object();
	left["a"] = 1;
	left["b"] = spk::JSON::Value::array();
	left["b"].pushBack(true);
	left["b"].pushBack(nullptr);
	spk::JSON::Value right = left;
	EXPECT_EQ(left, right);

	const std::string compact = left.toString({.pretty = false});
	EXPECT_EQ(compact.find('\n'), std::string::npos);
	const std::string pretty = left.toString({.pretty = true, .indentationSize = 2});
	EXPECT_NE(pretty.find('\n'), std::string::npos);
	EXPECT_TRUE(contains(pretty, "  \"a\""));
}

TEST(JSONValueTest, NumericFormsAndWhitespaceParseCorrectly)
{
	const auto value = spk::JSON::Value::fromString("[0,-1,12,1.5,-2.5,1e3,-2E-2]");
	ASSERT_EQ(value.size(), 7u);
	EXPECT_EQ(value.at(0).as<int>(), 0);
	EXPECT_EQ(value.at(1).as<int>(), -1);
	EXPECT_EQ(value.at(2).as<int>(), 12);
	EXPECT_DOUBLE_EQ(value.at(3).as<double>(), 1.5);
	EXPECT_DOUBLE_EQ(value.at(4).as<double>(), -2.5);
	EXPECT_DOUBLE_EQ(value.at(5).as<double>(), 1000.0);
	EXPECT_DOUBLE_EQ(value.at(6).as<double>(), -0.02);
}

TEST(JSONValueTest, MalformedJsonInputsAreRejected)
{
	const std::vector<std::string> invalidDocuments = {
		"true false",
		"tru",
		"nul",
		"[1,]",
		"{\"a\":1,}",
		"01",
		"1.",
		"1e",
		"\"\\x\"",
		"\"\\uZZZZ\"",
		std::string("\"") + char(0xC3) + char(0x28) + "\""};

	for (const std::string &document : invalidDocuments)
	{
		SCOPED_TRACE(document);
		EXPECT_THROW((void)spk::JSON::Value::fromString(document), std::runtime_error);
	}
}

TEST(JSONValueTest, DuplicateKeysCanBeRejectedOrAllowed)
{
	const std::string document = "{\"key\":1,\"key\":2}";
	EXPECT_THROW((void)spk::JSON::Value::fromString(document), std::runtime_error);

	spk::JSON::ParseOptions options;
	options.rejectDuplicateKeys = false;
	const auto parsed = spk::JSON::Value::fromString(document, options);
	EXPECT_EQ(parsed.count("key"), 1u);
}

TEST(JSONValueTest, NestingBeyondConfiguredDepthIsRejected)
{
	spk::JSON::ParseOptions options;
	options.maxDepth = 1;
	EXPECT_THROW((void)spk::JSON::Value::fromString("[[0]]", options), std::runtime_error);
}

TEST(JSONValueTest, InvalidStorageAndConversionsThrowRuntimeError)
{
	EXPECT_THROW(
		(void)spk::JSON::Value(std::numeric_limits<std::uint64_t>::max()),
		std::runtime_error);
	EXPECT_THROW((void)spk::JSON::Value(std::numeric_limits<double>::infinity()), std::runtime_error);
	EXPECT_THROW((void)spk::JSON::Value(std::numeric_limits<double>::quiet_NaN()), std::runtime_error);

	spk::JSON::Value stringValue("42");
	EXPECT_THROW((void)stringValue.as<int>(), std::runtime_error);
	spk::JSON::Value negative(-1);
	EXPECT_THROW((void)negative.as<unsigned int>(), std::runtime_error);
	spk::JSON::Value large(std::numeric_limits<std::int64_t>::max());
	EXPECT_THROW((void)large.as<std::int32_t>(), std::runtime_error);
}

TEST(JSONValueTest, WrongContainerOperationsMissingKeysAndIndicesThrowRuntimeError)
{
	spk::JSON::Value scalar(42);
	EXPECT_THROW((void)scalar.asObject(), std::runtime_error);
	EXPECT_THROW((void)scalar.asArray(), std::runtime_error);
	EXPECT_THROW((void)scalar.size(), std::runtime_error);
	EXPECT_THROW((void)scalar.at("key"), std::runtime_error);
	EXPECT_THROW((void)scalar.at(0), std::runtime_error);

	spk::JSON::Value object = spk::JSON::Value::object();
	EXPECT_THROW((void)object.at("missing"), std::runtime_error);
	EXPECT_THROW(object.pushBack(1), std::runtime_error);
	spk::JSON::Value array = spk::JSON::Value::array();
	EXPECT_THROW((void)array.at(0), std::runtime_error);
	EXPECT_THROW((void)array.contains("key"), std::runtime_error);
}

TEST(JSONValueTest, FileLoadSaveRoundTripAndMissingPaths)
{
	sparkle_test::TemporaryDirectory directory;
	spk::JSON::Value value = spk::JSON::Value::object();
	value["name"] = "sparkle";
	value["version"] = 1;
	const auto path = directory.file("value.json");
	value.saveToFile(path, {.pretty = true});

	EXPECT_EQ(spk::JSON::Value::loadFromFile(path), value);
	EXPECT_THROW((void)spk::JSON::Value::loadFromFile(directory.file("missing.json")), std::runtime_error);
	EXPECT_THROW(value.saveToFile(directory.path()), std::runtime_error);
}

TEST(JSONValueTest, ParseErrorsExposeDiagnosticText)
{
	try
	{
		(void)spk::JSON::Value::fromString("{\"a\": [1, }");
		FAIL() << "Expected parse failure";
	}
	catch (const std::runtime_error &exception)
	{
		EXPECT_FALSE(std::string(exception.what()).empty());
	}
}

TEST(JSONValueTest, DISABLED_ExactParserOffsetContextContractRequiresParserImplementationSnapshot)
{
	GTEST_SKIP() << "The backlog requires useful parse offsets/context, but the supplied Value header does not define the parser diagnostic format. The enabled parse-error test still verifies a non-empty diagnostic.";
}

TEST(JSONValueTest, DISABLED_SerializingInjectedNonFiniteValueHasNoPublicConstructionPath)
{
	GTEST_SKIP() << "All public floating-point setters reject NaN/infinity before storage, and the supplied API exposes no lower-level test seam for injecting a non-finite stored value.";
}
