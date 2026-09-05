#include <gtest/gtest.h>

#include "container/json/reader.hpp"
#include "sparkle_test.hpp"

#include <filesystem>
#include <string>

TEST(JSONLoaderTest, StandardUsageParsesValidJsonFile)
{
	sparkle_test::TemporaryDirectory directory;
	directory.write("valid.json", R"({"name":"sparkle","enabled":true,"count":3})");

	const spk::JSON::Value value = spk::JSON::Loader::parseFile(directory.file("valid.json"));
	EXPECT_EQ(value.at("name").as<std::string>(), "sparkle");
	EXPECT_TRUE(value.at("enabled").as<bool>());
	EXPECT_EQ(value.at("count").as<int>(), 3);
}

TEST(JSONLoaderTest, MissingFileThrowsJsonErrorWithFileRootPathAndWrappedMessage)
{
	sparkle_test::TemporaryDirectory directory;
	const std::filesystem::path file = directory.file("missing.json");

	try
	{
		(void)spk::JSON::Loader::parseFile(file);
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.file(), file);
		EXPECT_EQ(error.path(), "$");
		EXPECT_FALSE(error.message().empty());
		EXPECT_NE(std::string(error.what()).find(file.string()), std::string::npos);
	}
}

TEST(JSONLoaderTest, InvalidFileThrowsJsonErrorWithFileRootPathAndParserMessage)
{
	sparkle_test::TemporaryDirectory directory;
	directory.write("invalid.json", R"({"broken":[1,})");
	const std::filesystem::path file = directory.file("invalid.json");

	try
	{
		(void)spk::JSON::Loader::parseFile(file);
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.file(), file);
		EXPECT_EQ(error.path(), "$");
		EXPECT_FALSE(error.message().empty());
		EXPECT_NE(std::string(error.what()).find(error.message()), std::string::npos);
	}
}


TEST(JSONLoaderTest, DirectoryPathIsReportedAsJsonError)
{
	sparkle_test::TemporaryDirectory directory;
	try
	{
		(void)spk::JSON::Loader::parseFile(directory.path());
		FAIL() << "Expected spk::JSON::Error";
	}
	catch (const spk::JSON::Error &error)
	{
		EXPECT_EQ(error.file(), directory.path());
		EXPECT_EQ(error.path(), "$");
		EXPECT_FALSE(error.message().empty());
	}
}
