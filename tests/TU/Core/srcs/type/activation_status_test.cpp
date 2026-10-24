#include <gtest/gtest.h>

#include "type/activation_status.hpp"

#include <sstream>
#include <string>

TEST(ActivationStatusTest, ToStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toString(spk::ActivationStatus::Activated), "Activated");
	EXPECT_EQ(spk::toString(spk::ActivationStatus::Deactivated), "Deactivated");
}

TEST(ActivationStatusTest, ToWStringReturnsExpectedValues)
{
	EXPECT_EQ(spk::toWString(spk::ActivationStatus::Activated), L"Activated");
	EXPECT_EQ(spk::toWString(spk::ActivationStatus::Deactivated), L"Deactivated");
}

TEST(ActivationStatusTest, OutputStreamReturnsExpectedValues)
{
	std::ostringstream stream;

	stream << spk::ActivationStatus::Activated;
	EXPECT_EQ(stream.str(), "Activated");

	stream.str("");
	stream.clear();

	stream << spk::ActivationStatus::Deactivated;
	EXPECT_EQ(stream.str(), "Deactivated");
}

TEST(ActivationStatusTest, WideOutputStreamReturnsExpectedValues)
{
	std::wostringstream stream;

	stream << spk::ActivationStatus::Activated;
	EXPECT_EQ(stream.str(), L"Activated");

	stream.str(L"");
	stream.clear();

	stream << spk::ActivationStatus::Deactivated;
	EXPECT_EQ(stream.str(), L"Deactivated");
}
