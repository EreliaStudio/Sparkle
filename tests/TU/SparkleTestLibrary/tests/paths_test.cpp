#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "sparkle_test.hpp"

TEST(TestPathsTest, ResolvesExecutableAndConfiguredDirectories)
{
	const std::filesystem::path executable = sparkle_test::executablePath();

	EXPECT_TRUE(executable.is_absolute());
	EXPECT_TRUE(std::filesystem::is_regular_file(executable));
	EXPECT_EQ(sparkle_test::executableDirectory(), executable.parent_path());

	const std::filesystem::path resources = sparkle_test::resourcesDirectory();
	EXPECT_TRUE(resources.is_absolute());
	EXPECT_TRUE(std::filesystem::is_directory(resources));
	EXPECT_EQ(sparkle_test::expectedImagesDirectory(), resources / "expectedImages");

	const std::filesystem::path results = sparkle_test::resultsDirectory();
	EXPECT_TRUE(results.is_absolute());
	EXPECT_TRUE(std::filesystem::is_directory(results));
}

TEST(TestPathsTest, BuildsImagePathsInTheirDedicatedRoots)
{
	EXPECT_EQ(
		sparkle_test::expectedImagePath("widgets", "button"),
		sparkle_test::expectedImagesDirectory() / "widgets" / "button.png");
	EXPECT_EQ(
		sparkle_test::resultImagePath("widgets", "button"),
		sparkle_test::resultsDirectory() / "widgets" / "button.png");
}

TEST(TestPathsTest, ConfiguresConsumerRootsAndRejectsEmptyPaths)
{
	const auto resources = sparkle_test::resourcesDirectory();
	const auto results = sparkle_test::resultsDirectory();
	const auto root = results / "consumer-paths";
	sparkle_test::configurePaths(root / "resources", root / "results");
	EXPECT_EQ(sparkle_test::resourcesDirectory(), root / "resources");
	EXPECT_EQ(sparkle_test::resultsDirectory(), root / "results");
	EXPECT_THROW(sparkle_test::configurePaths({}, root), std::invalid_argument);
	EXPECT_EQ(sparkle_test::resourcesDirectory(), root / "resources");
	EXPECT_EQ(sparkle_test::resultsDirectory(), root / "results");
	EXPECT_THROW(sparkle_test::configurePaths(root, {}), std::invalid_argument);
	EXPECT_EQ(sparkle_test::resourcesDirectory(), root / "resources");
	EXPECT_EQ(sparkle_test::resultsDirectory(), root / "results");
	sparkle_test::configurePaths(resources, results);
	std::filesystem::remove_all(root);
}


TEST(TestPathsTest, FreshResultFilePathCreatesParentAndRemovesStaleFile)
{
	const auto path = sparkle_test::freshResultFilePath("logs", "application.log");
	EXPECT_TRUE(std::filesystem::is_directory(path.parent_path()));
	{
		std::ofstream stream(path);
		stream << "stale";
	}
	ASSERT_TRUE(std::filesystem::exists(path));

	const auto refreshed = sparkle_test::freshResultFilePath("logs", "application.log");
	EXPECT_EQ(refreshed, path);
	EXPECT_FALSE(std::filesystem::exists(path));
	std::filesystem::remove_all(path.parent_path());
}
