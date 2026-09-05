#include <gtest/gtest.h>

#include <algorithm>
#include <type_traits>
#include <utility>

#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "engine/entity_attachment_collection.hpp"

namespace
{
	class TestAttachment : public spk::EntityAttachment
	{
	public:
		using spk::EntityAttachment::EntityAttachment;
		~TestAttachment() override = default;
	};

	class AttachmentA : public TestAttachment
	{
	private:
		std::shared_ptr<int> _destructions;

	public:
		AttachmentA(std::string name, std::shared_ptr<int> destructions = {}) :
			TestAttachment(name),
			_destructions(std::move(destructions))
		{
		}

		~AttachmentA() override
		{
			if (_destructions)
				++*_destructions;
		}
	};

	class AttachmentB : public TestAttachment
	{
	public:
		using TestAttachment::TestAttachment;
	};

	class TestCollection : public spk::EntityAttachmentCollection<TestAttachment>
	{
	private:
		using Base = spk::EntityAttachmentCollection<TestAttachment>;

	public:
		using Base::attachments;
		using Base::getAttachment;
		using Base::getAttachments;
		using Base::subscribeToAttachmentAddition;
		using Base::subscribeToAttachmentRemoval;
		using Base::unregisterAttachment;

		template <typename TAttachment, typename... TArgs>
		TAttachment &add(TArgs &&...args)
		{
			auto attachment = std::make_unique<TAttachment>(std::forward<TArgs>(args)...);
			TAttachment &result = *attachment;
			Base::registerAttachment(std::move(attachment));
			return result;
		}

		template <typename TAttachment>
		void insert(std::unique_ptr<TAttachment> attachment)
		{
			Base::registerAttachment(std::move(attachment));
		}
	};
}

TEST(EntityAttachmentCollectionTest, StandardTypedAddQueryContractsRemoveAndDestroyExactlyOnce)
{
	TestCollection collection;
	auto destructions = std::make_shared<int>(0);
	int additions = 0;
	int removals = 0;

	auto additionContract = collection.subscribeToAttachmentAddition(
		[&](TestAttachment &) { ++additions; });
	auto removalContract = collection.subscribeToAttachmentRemoval(
		[&](TestAttachment &) { ++removals; });

	AttachmentA &first = collection.add<AttachmentA>("first", destructions);
	AttachmentB &second = collection.add<AttachmentB>("second");

	EXPECT_EQ(additions, 2);
	EXPECT_EQ(collection.attachments().size(), 2u);
	EXPECT_EQ(collection.getAttachment<AttachmentA>(), &first);
	EXPECT_EQ(collection.getAttachment<AttachmentB>(), &second);

	collection.unregisterAttachment(first);
	EXPECT_EQ(removals, 1);
	EXPECT_EQ(*destructions, 1);
	EXPECT_EQ(collection.getAttachment<AttachmentA>(), nullptr);
	EXPECT_EQ(collection.attachments().size(), 1u);
}

TEST(EntityAttachmentCollectionTest, QueryReturnsNullAndEmptyVectorsWhenNothingMatches)
{
	TestCollection collection;
	collection.add<AttachmentB>("only-b");

	EXPECT_EQ(collection.getAttachment<AttachmentA>(), nullptr);
	EXPECT_TRUE(collection.getAttachments<AttachmentA>().empty());
	EXPECT_EQ(collection.getAttachment<AttachmentB>(std::regex("missing")), nullptr);
}

TEST(EntityAttachmentCollectionTest, DuplicateTypesAndNamesAreAllReturned)
{
	TestCollection collection;
	AttachmentA &first = collection.add<AttachmentA>("duplicate");
	AttachmentA &second = collection.add<AttachmentA>("duplicate");

	const auto matches = collection.getAttachments<AttachmentA>(std::regex("duplicate"));
	ASSERT_EQ(matches.size(), 2u);
	EXPECT_NE(std::find(matches.begin(), matches.end(), &first), matches.end());
	EXPECT_NE(std::find(matches.begin(), matches.end(), &second), matches.end());
}

TEST(EntityAttachmentCollectionTest, PredicateAndRegexQueriesSelectExpectedAttachments)
{
	TestCollection collection;
	AttachmentA &alpha = collection.add<AttachmentA>("alpha");
	AttachmentA &beta = collection.add<AttachmentA>("beta");
	collection.add<AttachmentB>("alpha-b");

	EXPECT_EQ(
		collection.getAttachment<AttachmentA>([](AttachmentA *attachment) { return attachment->name() == "beta"; }),
		&beta);

	const auto regexMatches = collection.getAttachments<AttachmentA>(std::regex("^a"));
	ASSERT_EQ(regexMatches.size(), 1u);
	EXPECT_EQ(regexMatches.front(), &alpha);
}

TEST(EntityAttachmentCollectionTest, ConstQueriesReturnConstPointers)
{
	TestCollection collection;
	AttachmentA &attachment = collection.add<AttachmentA>("const");
	const TestCollection &constCollection = collection;

	static_assert(std::is_same_v<decltype(constCollection.getAttachment<AttachmentA>()), const AttachmentA *>);
	static_assert(std::is_same_v<decltype(constCollection.getAttachments<AttachmentA>()), std::vector<const AttachmentA *>>);

	EXPECT_EQ(constCollection.getAttachment<AttachmentA>(), &attachment);
	const auto all = constCollection.getAttachments<AttachmentA>();
	ASSERT_EQ(all.size(), 1u);
	EXPECT_EQ(all.front(), &attachment);
}

TEST(EntityAttachmentCollectionTest, CachedTypeLookupIsInvalidatedWhenAnElementIsRemoved)
{
	TestCollection collection;
	AttachmentA &first = collection.add<AttachmentA>("first");
	AttachmentA &second = collection.add<AttachmentA>("second");

	ASSERT_EQ(collection.getAttachments<AttachmentA>().size(), 2u);
	collection.unregisterAttachment(first);

	const auto remaining = collection.getAttachments<AttachmentA>();
	ASSERT_EQ(remaining.size(), 1u);
	EXPECT_EQ(remaining.front(), &second);
}

TEST(EntityAttachmentCollectionTest, RemovingAForeignAttachmentIsANoOp)
{
	TestCollection collection;
	AttachmentA &owned = collection.add<AttachmentA>("owned");
	AttachmentA foreign("foreign");
	int removals = 0;
	auto contract = collection.subscribeToAttachmentRemoval([&](TestAttachment &) { ++removals; });

	collection.unregisterAttachment(foreign);

	EXPECT_EQ(removals, 0);
	EXPECT_EQ(collection.attachments().size(), 1u);
	EXPECT_EQ(collection.getAttachment<AttachmentA>(), &owned);
}

TEST(EntityAttachmentCollectionTest, AdditionCallbackMayQueryTheCollection)
{
	TestCollection collection;
	bool observedDuringNotification = false;
	auto contract = collection.subscribeToAttachmentAddition(
		[&](TestAttachment &added) {
			observedDuringNotification =
				collection.getAttachment<AttachmentA>() == &added;
		});

	AttachmentA &attachment = collection.add<AttachmentA>("query-during-add");

	EXPECT_TRUE(observedDuringNotification);
	EXPECT_EQ(collection.getAttachment<AttachmentA>(), &attachment);
}

TEST(EntityAttachmentCollectionTest, AdditionCallbackMayRemoveTheElementBeingAdded)
{
	TestCollection collection;
	bool removalCallbackCalled = false;
	auto removal = collection.subscribeToAttachmentRemoval(
		[&](TestAttachment &) { removalCallbackCalled = true; });
	auto addition = collection.subscribeToAttachmentAddition(
		[&](TestAttachment &added) { collection.unregisterAttachment(added); });

	auto attachment = std::make_unique<AttachmentA>("remove-during-add");
	collection.insert(std::move(attachment));

	EXPECT_TRUE(removalCallbackCalled);
	EXPECT_TRUE(collection.attachments().empty());
	EXPECT_EQ(collection.getAttachment<AttachmentA>(), nullptr);
}

TEST(EntityAttachmentCollectionTest, RemovalCallbackCanQueryBeforeOwnershipIsReleased)
{
	TestCollection collection;
	AttachmentA &attachment = collection.add<AttachmentA>("remove-query");
	bool foundDuringRemoval = false;
	auto contract = collection.subscribeToAttachmentRemoval(
		[&](TestAttachment &removed) {
			foundDuringRemoval = collection.getAttachment<AttachmentA>() == &removed;
		});

	collection.unregisterAttachment(attachment);

	EXPECT_TRUE(foundDuringRemoval);
	EXPECT_EQ(collection.getAttachment<AttachmentA>(), nullptr);
}

TEST(EntityAttachmentCollectionTest, CollectionDestructionDestroysEveryRemainingElementExactlyOnce)
{
	auto destructions = std::make_shared<int>(0);
	{
		TestCollection collection;
		collection.add<AttachmentA>("first", destructions);
		collection.add<AttachmentA>("second", destructions);
		EXPECT_EQ(*destructions, 0);
	}

	EXPECT_EQ(*destructions, 2);
}

TEST(EntityAttachmentCollectionTest, DISABLED_EntityAndEnginePublicWrappersExerciseTheSameCollectionSemantics)
{
	GTEST_SKIP() << "The section-05 archive contains only EntityAttachmentCollection itself; the matching Entity/Engine wrapper snapshots are transitive main-repository dependencies. Keep wrapper-specific add/remove contract tests with those owning classes when supplied.";
}
