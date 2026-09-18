#pragma once

#include <filesystem>
#include <string>

namespace sparkle_test
{
	// Configure once on the runner thread before starting tests/worker threads.
	// Relative roots are resolved at configuration time; empty roots are rejected.
	void configurePaths(const std::filesystem::path &resources, const std::filesystem::path &results);
	[[nodiscard]] std::filesystem::path executablePath();
	[[nodiscard]] std::filesystem::path executableDirectory();
	[[nodiscard]] std::filesystem::path resourcesDirectory();
	[[nodiscard]] std::filesystem::path expectedImagesDirectory();
	[[nodiscard]] std::filesystem::path resultsDirectory();
	[[nodiscard]] std::filesystem::path expectedImagePath(
		const std::filesystem::path &category,
		const std::string &name);
	[[nodiscard]] std::filesystem::path resultImagePath(
		const std::filesystem::path &category,
		const std::string &name);
	void removeEmptyResultDirectories(const std::filesystem::path &startingPath);
}
