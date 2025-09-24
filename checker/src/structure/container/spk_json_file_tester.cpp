#include "structure/container/spk_json_file_tester.hpp"

#include <fstream>
#include <sstream>

TEST_F(JsonFileTest, DefaultConstructionStartsEmpty)
{
	spk::JSON::File file;
	EXPECT_EQ(file.root().isObject(), false) << "Root should be an object until populated";
	EXPECT_EQ(file.root().isArray(), false) << "Root should not be an array until populated";
}

TEST_F(JsonFileTest, SaveAndLoadRoundTrip)
{
	spk::JSON::File file;
	file[L"number"] = 42L;
	file[L"text"] = std::wstring(L"demo");
	file[L"array"].setAsArray();
	file[L"array"].append() = 1L;
	file[L"array"].append() = 2L;

	file.save(tempFile);
	EXPECT_EQ(std::filesystem::exists(tempFile), true) << "Saving should create the target file";

	spk::JSON::File loaded(tempFile);
	EXPECT_EQ(loaded.contains(L"number"), true) << "Loaded file should contain the saved numeric key";
	EXPECT_EQ(loaded[L"number"].as<long>(), 42L) << "Numeric value should persist across save/load";
	EXPECT_EQ(loaded[L"text"].as<std::wstring>(), L"demo") << "String value should persist across save/load";
	EXPECT_EQ(loaded[L"array"].size(), 2U) << "Array size should persist across save/load";
	EXPECT_EQ(loaded[L"array"][1U].as<long>(), 2L) << "Array content should persist across save/load";
}

TEST_F(JsonFileTest, IndexOperatorsAccessRootMembers)
{
	spk::JSON::File file;
	file.root().setAsArray();
	file.root().append() = 10L;
	file.root().append() = 20L;

	EXPECT_EQ(file[0U].as<long>(), 10L) << "Index operator should access the first array element";
	EXPECT_EQ(file[1U].as<long>(), 20L) << "Index operator should access the second array element";

	const spk::JSON::File &constRef = file;
	EXPECT_EQ(constRef[1U].as<long>(), 20L) << "Const index operator should access stored values";
}

TEST_F(JsonFileTest, SaveProducesReadableOutput)
{
	spk::JSON::File file;
	file[L"value"] = 99L;
	file.save(tempFile);

	std::wifstream stream(tempFile);
	ASSERT_EQ(stream.is_open(), true) << "Saved file should be readable as a wide stream";

	std::wstringstream buffer;
	buffer << stream.rdbuf();
	const std::wstring content = buffer.str();
	EXPECT_NE(content.find(L"\"value\""), std::wstring::npos) << "Serialized file should contain the stored key";
	EXPECT_NE(content.find(L"99"), std::wstring::npos) << "Serialized file should contain the stored value";
}

TEST_F(JsonFileTest, LoadThrowsForMissingFile)
{
	spk::JSON::File file;
	EXPECT_THROW(file.load(tempFile), std::runtime_error) << "Loading a non-existent file should throw";
}
