#include "sparkle_test/paths.hpp"
#include <gtest/gtest.h>

int main(int argc, char **argv)
{
	sparkle_test::configurePaths(SPARKLE_TEST_RESOURCES_DIR, SPARKLE_TEST_RESULTS_DIR);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
