#include "structure/container/spk_json_object_tester.hpp"

#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	struct JSONableSample
	{
		int value = 0;

		spk::JSON::Object toJSON() const
		{
			spk::JSON::Object object(L"JSONableSample");
			object.setAsObject();
			object.addAttribute(L"value") = static_cast<long>(value);
			return object;
		}

		void fromJSON(const spk::JSON::Object &json)
		{
			value = static_cast<int>(json[L"value"].as<long>());
		}
	};
}

TEST_F(JSONObjectTest, DefaultInitialization)
{
	EXPECT_EQ(jsonObject.isObject(), false) << "Freshly constructed object should not be marked as an object";
	EXPECT_EQ(jsonObject.isArray(), false) << "Freshly constructed object should not be marked as an array";
	EXPECT_EQ(jsonObject.isUnit(), false) << "Freshly constructed object should not be marked as a unit";
	EXPECT_THROW(jsonObject.count(L"SomeKey"), std::runtime_error) << "Counting members on a non-object should throw an exception";

	jsonObject.setAsObject();
	EXPECT_EQ(jsonObject.count(L"SomeKey"), 0U) << "After conversion to object the count for an unknown key should be zero";
}

TEST_F(JSONObjectTest, AddAttributeAndAccess)
{
	auto &attribute = jsonObject.addAttribute(L"Key1");
	EXPECT_EQ(jsonObject.isObject(), true) << "Adding an attribute should turn the node into an object";
	EXPECT_EQ(&jsonObject[L"Key1"], &attribute) << "Accessing the key should return the created attribute";
	EXPECT_EQ(jsonObject.contains(L"Key1"), true) << "Object should report containment of the added key";

	const auto &members = jsonObject.members();
	EXPECT_EQ(members.size(), 1U) << "Members map should contain exactly one entry";
	EXPECT_NE(members.find(L"Key1"), members.end()) << "Members map should include the added key";
}

TEST_F(JSONObjectTest, SetAndRetrieveBasicTypes)
{
	jsonObject = 42L;
	EXPECT_EQ(jsonObject.isUnit(), true) << "Assigning a long should transform the node into a unit";
	EXPECT_EQ(jsonObject.as<long>(), 42L) << "Stored long value should be retrievable";

	jsonObject = 3.141592653589793;
	EXPECT_EQ(jsonObject.isUnit(), true) << "Assigning a double should keep the node as a unit";
	EXPECT_DOUBLE_EQ(jsonObject.as<double>(), 3.141592653589793) << "Stored double value should be retrievable";

	jsonObject = std::wstring(L"Hello, JSON!");
	EXPECT_EQ(jsonObject.isUnit(), true) << "Assigning a string should keep the node as a unit";
	EXPECT_EQ(jsonObject.as<std::wstring>(), L"Hello, JSON!") << "Stored wide string should be retrievable";

	jsonObject = true;
	EXPECT_EQ(jsonObject.isUnit(), true) << "Assigning a boolean should keep the node as a unit";
	EXPECT_EQ(jsonObject.as<bool>(), true) << "Stored boolean should be retrievable";
}

TEST_F(JSONObjectTest, ArrayOperations)
{
	jsonObject.setAsArray();
	EXPECT_EQ(jsonObject.isArray(), true) << "setAsArray should mark the node as an array";

	jsonObject.resize(3U);
	EXPECT_EQ(jsonObject.size(), 3U) << "resize should create the requested number of elements";

	auto &appended = jsonObject.append();
	EXPECT_EQ(jsonObject.size(), 4U) << "append should increase the array size";

	appended = 100L;
	EXPECT_EQ(jsonObject[3U].as<long>(), 100L) << "Appended element should be accessible at the back";

	spk::JSON::Object value(L"manual");
	value = 55L;
	jsonObject.pushBack(value);
	EXPECT_EQ(jsonObject.size(), 5U) << "pushBack should append an additional element";
	EXPECT_EQ(jsonObject[4U].as<long>(), 55L) << "pushBack should copy the provided object";
}

TEST_F(JSONObjectTest, ObjectOperations)
{
	jsonObject.addAttribute(L"Key1") = 1L;
	jsonObject.addAttribute(L"Key2") = 2.5;
	EXPECT_EQ(jsonObject.contains(L"Key1"), true) << "contains should report presence of added key";
	EXPECT_EQ(jsonObject.contains(L"Key2"), true) << "contains should report presence of second key";
	EXPECT_EQ(jsonObject.count(L"Key1"), 1U) << "count should report 1 for existing key";

	EXPECT_EQ(jsonObject[L"Key1"].as<long>(), 1L) << "Value stored in the first key should be retrievable";
	EXPECT_DOUBLE_EQ(jsonObject[L"Key2"].as<double>(), 2.5) << "Value stored in the second key should be retrievable";
}

TEST_F(JSONObjectTest, SerializationRoundTrip)
{
	jsonObject.addAttribute(L"A") = 1L;
	jsonObject.addAttribute(L"B") = std::wstring(L"text");
	jsonObject.addAttribute(L"C").setAsArray();
	jsonObject[L"C"].append() = 4.0;

	std::wostringstream stream;
	stream << jsonObject;
	const std::wstring serialized = stream.str();
	EXPECT_NE(serialized.find(L"\"A\""), std::wstring::npos) << "Serialized string should contain key A";
	EXPECT_NE(serialized.find(L"\"text\""), std::wstring::npos) << "Serialized string should contain the stored string value";

	spk::JSON::Object parsed = spk::JSON::Object::fromString(serialized);
	EXPECT_EQ(parsed.isObject(), true) << "Parsing the serialized string should produce an object";
	EXPECT_EQ(parsed[L"A"].as<long>(), 1L) << "Parsed value for key A should match";
	EXPECT_EQ(parsed[L"B"].as<std::wstring>(), L"text") << "Parsed value for key B should match";
	EXPECT_DOUBLE_EQ(parsed[L"C"][0U].as<double>(), 4.0) << "Parsed array value should match the serialized content";
}

TEST_F(JSONObjectTest, FromStringParsesNestedStructures)
{
	const std::wstring jsonContent = LR"({"name":"Sparkle","values":[1,2,3],"nested":{"flag":true,"number":42}})";

	spk::JSON::Object parsed = spk::JSON::Object::fromString(jsonContent);
	EXPECT_EQ(parsed.isObject(), true) << "Parsed content should be an object";
	EXPECT_EQ(parsed[L"name"].as<std::wstring>(), L"Sparkle") << "Parsed string attribute should match the source";
	EXPECT_EQ(parsed[L"values"].isArray(), true) << "Array attribute should be recognized as an array";
	EXPECT_EQ(parsed[L"values"].size(), 3U) << "Array attribute should contain three elements";
	EXPECT_EQ(parsed[L"values"][2U].as<long>(), 3L) << "Third array element should equal 3";
	EXPECT_EQ(parsed[L"nested"].isObject(), true) << "Nested attribute should be recognized as an object";
	EXPECT_EQ(parsed[L"nested"][L"flag"].as<bool>(), true) << "Nested boolean should match";
	EXPECT_EQ(parsed[L"nested"][L"number"].as<long>(), 42L) << "Nested number should match";
}

TEST_F(JSONObjectTest, JSONableConversion)
{
	JSONableSample sample;
	sample.value = 77;

	jsonObject = sample;
	EXPECT_EQ(jsonObject.isUnit(), true) << "Assigning a JSONable object should store a unit containing an object";
	EXPECT_EQ(jsonObject.hold<std::shared_ptr<spk::JSON::Object>>(), true) << "Stored unit should hold a JSON object pointer";

	JSONableSample extracted = jsonObject.as<JSONableSample>();
	EXPECT_EQ(extracted.value, sample.value) << "Converted JSONable object should retain its payload";
}

TEST_F(JSONObjectTest, ResetClearsContent)
{
	jsonObject.addAttribute(L"Key") = 5L;
	jsonObject.reset();
	EXPECT_EQ(jsonObject.isObject(), false) << "After reset the node should no longer be an object";
	const auto& jsonObjectConst = jsonObject;
	EXPECT_THROW(jsonObjectConst[L"Key"], std::runtime_error) << "Accessing a key on a reset node should throw on a const object";
	EXPECT_NO_THROW(jsonObject[L"Key"]) << "Accessing a key that doesn't exist should not throw on a non-const object";
}

TEST_F(JSONObjectTest, ExceptionHandling)
{
	EXPECT_THROW(jsonObject.as<long>(), std::runtime_error) << "Requesting a value from an uninitialized node should throw";

	jsonObject.setAsArray();
	EXPECT_THROW(jsonObject[L"NonExistentKey"], std::runtime_error) << "Accessing a key on an array should throw";

	EXPECT_THROW(jsonObject.setAsObject(), std::runtime_error) << "Calling setAsObject twice should throw";

	jsonObject.append() = 1L;
	EXPECT_NO_THROW(jsonObject[0U].as<long>()) << "Accessing a valid array index should not throw";
	EXPECT_THROW(jsonObject[1U].as<long>(), std::out_of_range) << "Accessing an out of range array index should throw";
}