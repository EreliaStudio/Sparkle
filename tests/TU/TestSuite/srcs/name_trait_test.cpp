#include <gtest/gtest.h>

#include "design_pattern/trait/name_trait.hpp"

#include <string>

TEST(NameTraitTest, StandardUsagePreservesOrdinaryNameExactly)
{
	const spk::NameTrait trait("Player Camera");

	EXPECT_EQ(trait.name(), "Player Camera");
}

TEST(NameTraitTest, EmptyNameIsPreserved)
{
	const spk::NameTrait trait("");

	EXPECT_TRUE(trait.name().empty());
}

TEST(NameTraitTest, Utf8NameIsPreservedExactly)
{
	const std::string name = "Étoile_日本_🚀";
	const spk::NameTrait trait(name);

	EXPECT_EQ(trait.name(), name);
}

TEST(NameTraitTest, DuplicateNamesAreIndependentAndAllowed)
{
	const spk::NameTrait first("duplicate");
	const spk::NameTrait second("duplicate");

	EXPECT_EQ(first.name(), "duplicate");
	EXPECT_EQ(second.name(), "duplicate");
	EXPECT_EQ(first.name(), second.name());
}
