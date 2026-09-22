#include <gtest/gtest.h>

#include "design_pattern/trait/name_trait.hpp"

#include <string>
#include <vector>

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

TEST(NameTraitTest, NameEditionsNotifyOnlyWhenTheValueChanges)
{
	spk::NameTrait trait("before");
	std::vector<std::string> editions;
	auto contract = trait.subscribeToNameEdition([&](const std::string &name) {
		editions.push_back(name);
	});

	trait.setName("before");
	trait.setName("after");
	trait.setName("");

	EXPECT_TRUE(trait.name().empty());
	EXPECT_EQ(editions, (std::vector<std::string>{"after", ""}));
}
