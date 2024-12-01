#include <gtest/gtest.h>
#include "structure/container/spk_JSON_object.hpp"

class JSONObjectTest : public ::testing::Test
{
protected:
    spk::JSON::Object jsonObject;

    virtual void SetUp()
    {
        // Set up any initial state for the test object here
        jsonObject = spk::JSON::Object(L"TestObject");
    }

    virtual void TearDown()
    {
        // Clean up after each test, if necessary
    }
};