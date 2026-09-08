#include <gtest/gtest.h>

#include "type/orientation.hpp"

#include <sstream>
#include <string>

TEST(OrientationTest, ToStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toString(spk::Orientation::Vertical), "Vertical");
	EXPECT_EQ(spk::toString(spk::Orientation::Horizontal), "Horizontal");
}

TEST(OrientationTest, ToWStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toWString(spk::Orientation::Vertical), L"Vertical");
	EXPECT_EQ(spk::toWString(spk::Orientation::Horizontal), L"Horizontal");
}

TEST(OrientationTest, OutputStreamReturnsExpectedValues)
{
	std::ostringstream stream;

	stream << spk::Orientation::Vertical;
	EXPECT_EQ(stream.str(), "Vertical");

	stream.str("");
	stream.clear();

	stream << spk::Orientation::Horizontal;
	EXPECT_EQ(stream.str(), "Horizontal");
}

TEST(OrientationTest, WideOutputStreamReturnsExpectedValues)
{
	std::wostringstream stream;

	stream << spk::Orientation::Vertical;
	EXPECT_EQ(stream.str(), L"Vertical");

	stream.str(L"");
	stream.clear();

	stream << spk::Orientation::Horizontal;
	EXPECT_EQ(stream.str(), L"Horizontal");
}
