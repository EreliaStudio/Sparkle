#include "utils/spk_string_utils.hpp"
#include <gtest/gtest.h>

TEST(StringUtilsTest, ConvertStringToWString)
{
	std::string original = "Hello World";
	std::wstring converted = spk::StringUtils::stringToWString(original);

	ASSERT_EQ(converted, L"Hello World") << "Converted wide string should match expected value.";
}

TEST(StringUtilsTest, ConvertWStringToString)
{
	std::wstring original = L"Hello World";
	std::string converted = spk::StringUtils::wstringToString(original);

	ASSERT_EQ(converted, "Hello World") << "Converted string should match expected value.";
}

TEST(StringUtilsTest, SplitString)
{
	std::string original = "one,two,three,four";
	std::vector<std::string> expected = { "one", "two", "three", "four" };
	std::vector<std::string> result = spk::StringUtils::splitString(original, ",");

	ASSERT_EQ(result.size(), expected.size()) << "Resulting vector size should match expected size.";
	for (size_t i = 0; i < expected.size(); ++i)
	{
		ASSERT_EQ(result[i], expected[i]) << "Resulting string should match expected value at index " << i;
	}
}

TEST(StringUtilsTest, SplitWString)
{
	std::wstring original = L"one,two,three,four";
	std::vector<std::wstring> expected = { L"one", L"two", L"three", L"four" };
	std::vector<std::wstring> result = spk::StringUtils::splitWString(original, L",");

	ASSERT_EQ(result.size(), expected.size()) << "Resulting vector size should match expected size.";
	for (size_t i = 0; i < expected.size(); ++i)
	{
		ASSERT_EQ(result[i], expected[i]) << "Resulting wide string should match expected value at index " << i;
	}
}
