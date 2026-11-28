#include <gtest/gtest.h>

#include "sparkle_test.hpp"

TEST(TestFileUtilsTest, ReadsTextFile)
{
	sparkle_test::TemporaryDirectory directory;
	directory.write("sample.txt", "first\nsecond");

	EXPECT_EQ(
		sparkle_test::readTextFile(directory.file("sample.txt")),
		"first\nsecond");
}
