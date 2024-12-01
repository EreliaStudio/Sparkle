#include "structure/container/spk_json_object_tester.hpp"

TEST_F(JSONObjectTest, DefaultInitialization)
{
    EXPECT_FALSE(jsonObject.isObject());
    EXPECT_FALSE(jsonObject.isArray());
    EXPECT_FALSE(jsonObject.isUnit());
    EXPECT_THROW(jsonObject.count(L"SomeKey"), std::runtime_error);
    jsonObject.setAsObject();
    EXPECT_EQ(jsonObject.count(L"SomeKey"), 0);
}

TEST_F(JSONObjectTest, AddAttributeAndAccess)
{
    spk::JSON::Object& attr = jsonObject.addAttribute(L"Key1");
    EXPECT_TRUE(jsonObject.isObject());
    EXPECT_EQ(&jsonObject[L"Key1"], &attr);
    EXPECT_TRUE(jsonObject.contains(L"Key1"));
}

TEST_F(JSONObjectTest, SetAndRetrieveBasicTypes)
{
    jsonObject = 42L;
    EXPECT_TRUE(jsonObject.isUnit());
    EXPECT_EQ(jsonObject.as<long>(), 42L);

    jsonObject = 3.14;
    EXPECT_TRUE(jsonObject.isUnit());
    EXPECT_DOUBLE_EQ(jsonObject.as<double>(), 3.14);

    jsonObject = std::wstring(L"Hello, JSON!");
    EXPECT_TRUE(jsonObject.isUnit());
    EXPECT_EQ(jsonObject.as<std::wstring>(), L"Hello, JSON!");

    jsonObject = true;
    EXPECT_TRUE(jsonObject.isUnit());
    EXPECT_EQ(jsonObject.as<bool>(), true);
}

TEST_F(JSONObjectTest, ArrayOperations)
{
    jsonObject.setAsArray();
    EXPECT_TRUE(jsonObject.isArray());

    jsonObject.resize(3);
    EXPECT_EQ(jsonObject.size(), 3);

    spk::JSON::Object& appended = jsonObject.append();
    EXPECT_EQ(jsonObject.size(), 4);

    appended = 100L;
    EXPECT_EQ(jsonObject[3].as<long>(), 100L);
}

TEST_F(JSONObjectTest, ObjectOperations)
{
    jsonObject.addAttribute(L"Key1") = 1L;
    jsonObject.addAttribute(L"Key2") = 2.5;
    EXPECT_TRUE(jsonObject.contains(L"Key1"));
    EXPECT_TRUE(jsonObject.contains(L"Key2"));
    EXPECT_EQ(jsonObject.count(L"Key1"), 1);

    EXPECT_EQ(jsonObject[L"Key1"].as<long>(), 1L);
    EXPECT_DOUBLE_EQ(jsonObject[L"Key2"].as<double>(), 2.5);
}

TEST_F(JSONObjectTest, PrintObject)
{
    jsonObject.addAttribute(L"Key1") = 1L;
    jsonObject.addAttribute(L"Key2") = 2.5;
    jsonObject.addAttribute(L"Key3") = std::wstring(L"Test");

    std::wostringstream oss;
    oss << jsonObject;
    std::wstring expectedOutput = L"{\n   \"Key1\": 1,\n   \"Key2\": 2.500000,\n   \"Key3\": \"Test\"\n}";
    EXPECT_EQ(oss.str(), expectedOutput);
}

TEST_F(JSONObjectTest, ExceptionHandling)
{
    EXPECT_THROW(jsonObject.as<long>(), std::runtime_error);

    jsonObject.setAsArray();
    EXPECT_THROW(jsonObject[L"NonExistentKey"], std::runtime_error);

    EXPECT_THROW(jsonObject.setAsObject(), std::runtime_error);

    jsonObject.append() = 1L;
    EXPECT_NO_THROW(jsonObject[0].as<long>());
    EXPECT_THROW(jsonObject[1].as<long>(), std::out_of_range);
}