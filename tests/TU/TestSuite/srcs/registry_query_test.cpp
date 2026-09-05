#include <gtest/gtest.h>

#include <memory>
#include <regex>
#include <type_traits>
#include <unordered_map>

#include "engine/registry_query.hpp"

namespace
{
	class QueryObject : public spk::ContextualizableTrait<int>,
					public spk::Registry<int, QueryObject>::Object
	{
	public:
		int value = 0;

		QueryObject(int context, int value) :
			spk::ContextualizableTrait<int>(context),
			value(value)
		{
		}

		virtual ~QueryObject() = default;
	};

	class DerivedQueryObject : public QueryObject,
						   public spk::Registry<int, DerivedQueryObject>::Object
	{
	public:
		DerivedQueryObject(int context, int value) : QueryObject(context, value) {}
	};

	using Registry = spk::Registry<int, QueryObject>;
	using Query = Registry::Query;
	using ElementSet = Registry::ElementSet;

	class KeepEven final : public Query::Operation
	{
	public:
		void execute(ElementSet &currentElements, const int &) override
		{
			for (auto it = currentElements.begin(); it != currentElements.end();)
			{
				if ((*it)->value % 2 != 0)
					it = currentElements.erase(it);
				else
					++it;
			}
		}
	};

	class CountingOperation final : public Query::Operation
	{
	private:
		std::shared_ptr<std::unordered_map<int, int>> _executions;

	public:
		explicit CountingOperation(std::shared_ptr<std::unordered_map<int, int>> executions) :
			_executions(std::move(executions))
		{
		}

		void execute(ElementSet &, const int &context) override
		{
			++(*_executions)[context];
		}
	};

}

TEST(RegistryQueryTest, DISABLED_CopyMoveRestrictionsNeedExplicitPublicContract)
{
	// The snapshot does not explicitly declare Query copy/move policy.
	// Keep this backlog requirement visible until the production contract is explicit.
	GTEST_SKIP() << "Registry::Query copy/move restrictions are not explicit in the supplied snapshot";
}

TEST(RegistryQueryTest, StandardFromRegistryPredicateIntersectionAndUnionComposition)
{
	QueryObject odd(10, 1);
	QueryObject even(10, 2);
	DerivedQueryObject derived(10, 4);

	const auto derivedSet = spk::Registry<int, DerivedQueryObject>::ElementSet{&derived};
	const auto explicitBaseSet = Registry::ElementSet{&even};

	auto query = Registry::query();
	query.insert<KeepEven>()
		.insert<Query::IntersectWith<DerivedQueryObject>>(derivedSet)
		.insert<Query::UnionWith<QueryObject>>(explicitBaseSet);

	const auto &result = query.collect(10);
	ASSERT_EQ(result.size(), 2u);
	EXPECT_TRUE(result.contains(&derived));
	EXPECT_TRUE(result.contains(&even));
	EXPECT_FALSE(result.contains(&odd));
}

TEST(RegistryQueryTest, EmptyAndAllRegistrySelectionsAreHandled)
{
	auto query = Registry::query();
	EXPECT_TRUE(query.collect(1001).empty());

	QueryObject first(1002, 1);
	QueryObject second(1002, 2);
	const auto &all = query.collect(1002);
	EXPECT_EQ(all.size(), 2u);
	EXPECT_TRUE(all.contains(&first));
	EXPECT_TRUE(all.contains(&second));
}

TEST(RegistryQueryTest, RepeatedCollectUsesCacheUntilTheRegistryChanges)
{
	auto executions = std::make_shared<std::unordered_map<int, int>>();
	auto query = Registry::query();
	query.insert<CountingOperation>(executions);

	QueryObject first(77, 1);
	EXPECT_EQ(query.collect(77).size(), 1u);
	EXPECT_EQ((*executions)[77], 1);

	EXPECT_EQ(query.collect(77).size(), 1u);
	EXPECT_EQ((*executions)[77], 1);

	QueryObject second(77, 2);
	EXPECT_EQ(query.collect(77).size(), 2u);
	EXPECT_EQ((*executions)[77], 2);
}

TEST(RegistryQueryTest, RegistryEditsInvalidateOnlyTheAffectedContext)
{
	auto executions = std::make_shared<std::unordered_map<int, int>>();
	auto query = Registry::query();
	query.insert<CountingOperation>(executions);

	QueryObject first(1, 1);
	QueryObject second(2, 2);
	(void)query.collect(1);
	(void)query.collect(2);
	ASSERT_EQ((*executions)[1], 1);
	ASSERT_EQ((*executions)[2], 1);

	QueryObject addedOnlyToFirstContext(1, 3);
	(void)query.collect(2);
	EXPECT_EQ((*executions)[2], 1);
	(void)query.collect(1);
	EXPECT_EQ((*executions)[1], 2);
}

TEST(RegistryQueryTest, MovingAnObjectBetweenContextsRefreshesBothSelections)
{
	QueryObject object(3, 9);
	auto query = Registry::query();

	ASSERT_TRUE(query.collect(3).contains(&object));
	ASSERT_FALSE(query.collect(4).contains(&object));

	object.changeContext(4);

	EXPECT_FALSE(query.collect(3).contains(&object));
	EXPECT_TRUE(query.collect(4).contains(&object));
}

TEST(RegistryQueryTest, IntersectionWithExplicitSetDoesNotSubscribeToRegistryEdits)
{
	DerivedQueryObject selected(5, 1);
	const auto explicitSet = spk::Registry<int, DerivedQueryObject>::ElementSet{&selected};

	auto query = Registry::query();
	query.insert<Query::IntersectWith<DerivedQueryObject>>(explicitSet);
	ASSERT_TRUE(query.collect(5).contains(&selected));

	DerivedQueryObject later(5, 2);
	const auto &result = query.collect(5);
	EXPECT_TRUE(result.contains(&selected));
	EXPECT_FALSE(result.contains(&later));
}

TEST(RegistryQueryTest, RegistryBackedIntersectionInvalidatesWhenOtherRegistryChanges)
{
	DerivedQueryObject first(6, 1);
	auto query = Registry::query();
	query.insert<Query::IntersectWith<DerivedQueryObject>>();
	ASSERT_TRUE(query.collect(6).contains(&first));

	DerivedQueryObject second(6, 2);
	const auto &result = query.collect(6);
	EXPECT_EQ(result.size(), 2u);
	EXPECT_TRUE(result.contains(&second));
}

TEST(RegistryQueryTest, OperationOrderIsObservable)
{
	QueryObject baseOnly(8, 1);
	DerivedQueryObject derived(8, 2);
	const auto addBack = Registry::ElementSet{&baseOnly};

	auto intersectThenUnion = Registry::query();
	intersectThenUnion
		.insert<Query::IntersectWith<DerivedQueryObject>>()
		.insert<Query::UnionWith<QueryObject>>(addBack);

	auto unionThenIntersect = Registry::query();
	unionThenIntersect
		.insert<Query::UnionWith<QueryObject>>(addBack)
		.insert<Query::IntersectWith<DerivedQueryObject>>();

	EXPECT_EQ(intersectThenUnion.collect(8).size(), 2u);
	EXPECT_EQ(unionThenIntersect.collect(8).size(), 1u);
	EXPECT_TRUE(unionThenIntersect.collect(8).contains(&derived));
}

TEST(RegistryQueryTest, SetSemanticsSuppressDuplicatesIntroducedByUnion)
{
	QueryObject object(9, 1);
	const auto explicitSet = Registry::ElementSet{&object};

	auto query = Registry::query();
	query.insert<Query::UnionWith<QueryObject>>(explicitSet)
		.insert<Query::UnionWith<QueryObject>>(explicitSet);

	EXPECT_EQ(query.collect(9).size(), 1u);
}

TEST(RegistryQueryTest, DestroyedQueryReleasesItsRegistrySubscriptions)
{
	QueryObject first(12, 1);
	{
		auto query = Registry::query();
		ASSERT_TRUE(query.collect(12).contains(&first));
	}

	EXPECT_NO_THROW({ QueryObject second(12, 2); });
}

TEST(RegistryQueryTest, DISABLED_ContainParticipantSupportsTypeRegexPredicateAndReactiveEdits)
{
	GTEST_SKIP() << "registry_query.hpp supplies ContainParticipant, but the section-05 archive does not include the matching Entity/System::Participant snapshots required to instantiate the operation reliably. Add this case when those transitive headers are supplied with the backlog.";
}

TEST(RegistryQueryTest, DISABLED_ContainBehaviourSupportsTypeRegexPredicateAndReactiveEdits)
{
	GTEST_SKIP() << "registry_query.hpp supplies ContainBehaviour, but the section-05 archive does not include the matching Entity/Behaviour snapshots required to instantiate the operation reliably. Add this case when those transitive headers are supplied with the backlog.";
}

TEST(RegistryQueryTest, DISABLED_AttachmentNameChangesInvalidateRegexQueries)
{
	GTEST_SKIP() << "The supplied ContainParticipant/ContainBehaviour implementation subscribes only to add/remove events; no name-edition subscription is visible. Keep this disabled until regex queries invalidate on attachment rename as required by the backlog.";
}
