#include <filesystem>

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
