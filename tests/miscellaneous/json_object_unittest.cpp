#include <gtest/gtest.h>
#include <rapidcheck.h>
#include <rapidcheck/gtest.h>

#include "miscellaneous/JSON/spk_JSON_object.hpp"

using namespace spk::JSON;

class JSONObjectTest : public ::testing::Test {
protected:
    Object json_object;

    void SetUp() override {
		json_object = Object();
    }

    void TearDown() override {

    }
};

TEST_F(JSONObjectTest, InitializationTest) {
    EXPECT_FALSE(json_object.isObject());
    EXPECT_FALSE(json_object.isArray());
    EXPECT_FALSE(json_object.isUnit());
}

TEST_F(JSONObjectTest, AddAttributeTest) {
    auto& child = json_object.addAttribute("key");
    child.set<std::string>("value");
    EXPECT_TRUE(json_object.isObject());
    EXPECT_TRUE(json_object.contains("key"));
    EXPECT_EQ(child.as<std::string>(), "value");
}

TEST_F(JSONObjectTest, ArrayInitializationTest) {
    json_object.setAsArray();
    EXPECT_TRUE(json_object.isArray());
    json_object.resize(2);
    EXPECT_EQ(json_object.size(), 2U);
}

TEST_F(JSONObjectTest, SetAndGetTest) {
    json_object.setAsObject();
    json_object.addAttribute("number").set<long>(42);
    json_object.addAttribute("text").set<std::string>("hello");
    
    EXPECT_EQ(json_object["number"].as<long>(), 42);
    EXPECT_EQ(json_object["text"].as<std::string>(), "hello");
}

TEST_F(JSONObjectTest, ErrorHandlingTest) {
    json_object.setAsObject();
    json_object.addAttribute("key").set<double>(3.14);
    EXPECT_THROW({
        json_object["key"].as<std::string>();
    }, std::exception);
}