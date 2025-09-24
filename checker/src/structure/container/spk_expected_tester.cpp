#include <gtest/gtest.h>

#include <string>

#include "structure/container/spk_expected.hpp"

TEST(ExpectedTest, DefaultConstructedHasNoError)
{
	spk::Expected<int> expected;
	EXPECT_EQ(expected.hasError, false) << "Default constructed Expected should not flag an error";
}

TEST(ExpectedTest, ValueConstructedStoresValue)
{
	spk::Expected<int> expected(15);
	EXPECT_EQ(expected.hasError, false) << "Value constructed Expected should not flag an error";
	EXPECT_EQ(expected.value, 15) << "Value constructed Expected should store the provided value";
}

TEST(ExpectedTest, RaiseProducesErrorState)
{
	const std::wstring message = L"failure";
	auto expected = spk::Expected<int>::Raise(message);
	EXPECT_EQ(expected.hasError, true) << "Raise should produce an error state";
	EXPECT_EQ(expected.errorData, message) << "Raised Expected should store the provided error message";
}
