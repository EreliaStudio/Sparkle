#include <gtest/gtest.h>

#include "container/json/reader.hpp"

#include <filesystem>
#include <string>
#include <utility>

namespace
{
	[[nodiscard]] bool contains(const std::string &text, const std::string &fragment)
	{
		return text.find(fragment) != std::string::npos;
	}
}

TEST(JSONErrorTest, StandardUsagePreservesFilePathMessageAndComposedWhat)
{
	const std::filesystem::path file = "config/game.json";
	spk::JSON::Error error(file, "$.player.speed", "invalid value");

	EXPECT_EQ(error.file(), file);
	EXPECT_EQ(error.path(), "$.player.speed");
	EXPECT_EQ(error.message(), "invalid value");
	EXPECT_TRUE(contains(error.what(), file.string()));
	EXPECT_TRUE(contains(error.what(), "$.player.speed"));
	EXPECT_TRUE(contains(error.what(), "invalid value"));
}

TEST(JSONErrorTest, RootNestedAndIndexedPathsRemainUnchanged)
{
	spk::JSON::Error root("root.json", "$", "root failure");
	spk::JSON::Error nested("nested.json", "$.graphics.window", "nested failure");
	spk::JSON::Error indexed("array.json", "$.players[3].name", "indexed failure");

	EXPECT_EQ(root.path(), "$");
	EXPECT_EQ(nested.path(), "$.graphics.window");
	EXPECT_EQ(indexed.path(), "$.players[3].name");
}

TEST(JSONErrorTest, EmptyFieldsRemainSafeAndInspectable)
{
	spk::JSON::Error error({}, "", "");
	EXPECT_TRUE(error.file().empty());
	EXPECT_TRUE(error.path().empty());
	EXPECT_TRUE(error.message().empty());
	EXPECT_NO_THROW((void)error.what());
}

TEST(JSONErrorTest, CopiesAndMovesRetainObservableState)
{
	spk::JSON::Error original("settings.json", "$.audio.volume", "out of range");
	spk::JSON::Error copy(original);
	EXPECT_EQ(copy.file(), original.file());
	EXPECT_EQ(copy.path(), original.path());
	EXPECT_EQ(copy.message(), original.message());
	EXPECT_STREQ(copy.what(), original.what());

	spk::JSON::Error moved(std::move(copy));
	EXPECT_EQ(moved.file(), "settings.json");
	EXPECT_EQ(moved.path(), "$.audio.volume");
	EXPECT_EQ(moved.message(), "out of range");
	EXPECT_TRUE(contains(moved.what(), "out of range"));
}


TEST(JSONErrorTest, DISABLED_ExactComposedWhatFormattingRequiresImplementationSnapshot)
{
	GTEST_SKIP() << "Error exposes file/path/message accessors, but the exact std::runtime_error what() composition is implemented outside the supplied header snapshot.";
}
