#include <gtest/gtest.h>
#include "spk_sfinae.hpp"

#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <memory>

namespace
{
    using TestMap = std::map<int, int>;
}

TEST(IsContainerTest, IsContainerTrue)
{
    ASSERT_TRUE(spk::IsContainer<std::vector<int> >::value) << "std::vector should be recognized as a container";
    ASSERT_TRUE(spk::IsContainer<std::list<int> >::value) << "std::list should be recognized as a container";
    ASSERT_TRUE(spk::IsContainer<::TestMap>::value) << "std::map should be recognized as a container";
    ASSERT_TRUE(spk::IsContainer<std::set<int>>::value) << "std::set should be recognized as a container";
    ASSERT_TRUE(spk::IsContainer<std::string>::value) << "std::string should be recognized as a container";
}

TEST(IsContainerTest, IsContainerFalse)
{
    ASSERT_FALSE(spk::IsContainer<int>::value) << "int should not be recognized as a container";
    ASSERT_FALSE(spk::IsContainer<float>::value) << "float should not be recognized as a container";
    ASSERT_FALSE(spk::IsContainer<double>::value) << "double should not be recognized as a container";
    ASSERT_FALSE(spk::IsContainer<char>::value) << "char should not be recognized as a container";
    ASSERT_FALSE(spk::IsContainer<std::unique_ptr<int>>::value) << "std::unique_ptr should not be recognized as a container";
}