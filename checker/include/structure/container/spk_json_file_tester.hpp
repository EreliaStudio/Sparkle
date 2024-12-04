#pragma once

#include <chrono>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "structure/container/spk_json_file.hpp"

class JsonFileTest : public ::testing::Test
{
protected:
	std::filesystem::path tempFile;

	void SetUp() override
	{
		const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		tempFile = std::filesystem::temp_directory_path() / ("sparkle_json_test_" + std::to_string(uniqueSuffix) + ".json");
	}

	void TearDown() override
	{
		if (std::filesystem::exists(tempFile))
		{
			std::filesystem::remove(tempFile);
		}
	}
};
