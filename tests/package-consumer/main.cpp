#include <filesystem>
#include <fstream>
#include <sparkle_test.hpp>
#include <stdexcept>

namespace
{
	namespace fs = std::filesystem;
	void require(bool condition, const char *message)
	{
		if (!condition)
		{
			throw std::runtime_error(message);
		}
	}
	void rejectEmptyPaths(const fs::path &resources, const fs::path &results)
	{
		for (const bool emptyResources : {true, false})
		{
			bool rejected = false;
			try
			{
				sparkle_test::configurePaths(emptyResources ? fs::path{} : resources, emptyResources ? results : fs::path{});
			} catch (const std::invalid_argument &)
			{
				rejected = true;
			}
			require(rejected, "Empty paths must be rejected");
			require(sparkle_test::resourcesDirectory() == resources, "Invalid configuration changed resources");
			require(sparkle_test::resultsDirectory() == results, "Invalid configuration changed results");
		}
	}
	void checkPaths(const fs::path &root)
	{
		require(sparkle_test::resourcesDirectory() == sparkle_test::executableDirectory() / "resources", "Unexpected default resources");
		require(sparkle_test::resultsDirectory() == sparkle_test::executableDirectory() / "results", "Unexpected default results");
		sparkle_test::configurePaths(root / "resources", root / "results");
		const auto resources = fs::weakly_canonical(root / "resources");
		const auto results = fs::weakly_canonical(root / "results");
		require(sparkle_test::resourcesDirectory() == resources, "Custom resources were not used");
		require(sparkle_test::resultsDirectory() == results, "Custom results were not used");
		rejectEmptyPaths(resources, results);
	}
	void checkImages(const fs::path &root)
	{
		fs::create_directories(root / "resources");
		const auto expected = root / "resources" / "reference.ppm";
		const auto actual = sparkle_test::resultImagePath("images", "actual");
		const auto diff = sparkle_test::resultImagePath("images", "diff");
		fs::create_directories(actual.parent_path());
		std::ofstream(expected, std::ios::binary) << "P6\n1 1\n255\n"
												  << "abc";
		fs::copy_file(expected, actual, fs::copy_options::overwrite_existing);
		require(sparkle_test::compareImages(actual, expected, diff).matches, "Identical images must match");
		require(!fs::exists(actual) && !fs::exists(diff), "Matching result files must be cleaned up");
		fs::create_directories(actual.parent_path());
		std::ofstream(actual, std::ios::binary) << "P6\n1 1\n255\n"
												<< "xyz";
		require(!sparkle_test::compareImages(actual, expected, diff).matches, "Different images must not match");
		require(fs::is_regular_file(diff) && fs::file_size(diff) > 0, "Mismatch must produce a difference image");
		require(fs::exists(actual) && fs::exists(expected), "Mismatch must preserve actual and reference images");
		std::ifstream reference(expected, std::ios::binary);
		const std::string bytes{std::istreambuf_iterator<char>{reference}, std::istreambuf_iterator<char>{}};
		require(bytes == "P6\n1 1\n255\nabc", "Reference image was modified");
	}
}

int main()
{
	const auto root = fs::current_path() / "consumer-fixture";
	checkPaths(root);
	checkImages(root);
	fs::remove_all(root);
}
