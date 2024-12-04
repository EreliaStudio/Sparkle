#include "structure/container/spk_json_object.hpp"
#include <gtest/gtest.h>

class JSONObjectTest : public ::testing::Test
{
protected:
	spk::JSON::Object jsonObject;

	void SetUp() override
	{
		jsonObject = spk::JSON::Object(L"TestObject");
	}
};