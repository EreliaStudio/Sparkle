#include <gtest/gtest.h>

#include "file_utils.hpp"
#include "sparkle_test.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

TEST(ReadTextFileTest, StandardUsagePreservesExactContentsIncludingNulAndNewlines)
{
	sparkle_test::TemporaryDirectory directory;
	const std::string content("alpha\nbe\0ta\r\ngamma\n", 19);
	directory.write("sample.txt", content);

	const std::string result = spk::readTextFile(directory.file("sample.txt"));
	EXPECT_EQ(result.size(), content.size());
	EXPECT_EQ(result, content);
}

TEST(ReadTextFileTest, ReadsEmptyFile)
{
	sparkle_test::TemporaryDirectory directory;
	directory.write("empty.txt", "");

	EXPECT_TRUE(spk::readTextFile(directory.file("empty.txt")).empty());
}

TEST(ReadTextFileTest, ReadsLargeFile)
{
	sparkle_test::TemporaryDirectory directory;
	std::string content;
	content.reserve(1024 * 1024);
	for (std::size_t index = 0; index < 16384; ++index)
	{
		content += "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\n";
	}
	directory.write("large.txt", content);

	EXPECT_EQ(spk::readTextFile(directory.file("large.txt")), content);
}

TEST(ReadTextFileTest, MissingFileThrowsRuntimeErrorContainingPath)
{
	sparkle_test::TemporaryDirectory directory;
	const std::filesystem::path missing = directory.file("does-not-exist.txt");

	try
	{
		(void)spk::readTextFile(missing);
		FAIL() << "Expected std::runtime_error";
	}
	catch (const std::runtime_error &exception)
	{
		EXPECT_TRUE(sparkle_test::containsText(exception.what(), missing.string()));
	}
}
