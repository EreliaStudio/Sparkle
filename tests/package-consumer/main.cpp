#include <sparkle_test.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

int main()
{
    namespace fs = std::filesystem;
    const auto root = fs::current_path() / "consumer-fixture";
    sparkle_test::configurePaths(root / "resources", root / "results");
    if (sparkle_test::resourcesDirectory() != fs::weakly_canonical(root / "resources"))
        throw std::runtime_error("Consumer resource root was not used");
    fs::create_directories(root / "resources");
    const auto expected = root / "resources" / "reference.ppm";
    const auto actual = sparkle_test::resultImagePath("images", "actual");
    const auto diff = sparkle_test::resultImagePath("images", "diff");
    fs::create_directories(actual.parent_path());
    std::ofstream(expected, std::ios::binary) << "P6\n1 1\n255\n" << "abc";
    fs::copy_file(expected, actual, fs::copy_options::overwrite_existing);
    if (!sparkle_test::compareImages(actual, expected, diff).matches)
        throw std::runtime_error("Identical images must match");
    fs::create_directories(actual.parent_path());
    std::ofstream(actual, std::ios::binary) << "P6\n1 1\n255\n" << "xyz";
    if (sparkle_test::compareImages(actual, expected, diff).matches || !fs::exists(diff))
        throw std::runtime_error("Different images must produce a diff");
    fs::remove_all(root);
}
