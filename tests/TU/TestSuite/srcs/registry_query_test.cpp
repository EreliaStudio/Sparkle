#include <gtest/gtest.h>

#include <memory>
#include <regex>
#include <type_traits>
#include <unordered_map>

#include "engine/engine.hpp"
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

	class QueryParticipant : public spk::System::Participant
	{
	public:
		int value;

		QueryParticipant(std::string name, int value = 0) :
			spk::System::Participant(std::move(name)),
			value(value)
		{
		}
	};

	class QueryBehaviour : public spk::Behaviour
	{
	public:
		int value;

		QueryBehaviour(std::string name, int value = 0) :
			spk::Behaviour(std::move(name)),
			value(value)
		{
		}
	};

}

TEST(RegistryQueryTest, CopyMoveRestrictionsAreExplicit)
{
	static_assert(!std::is_copy_constructible_v<Query>);
	static_assert(!std::is_copy_assignable_v<Query>);
	static_assert(!std::is_move_constructible_v<Query>);
	static_assert(!std::is_move_assignable_v<Query>);
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

TEST(RegistryQueryTest, ContainParticipantSupportsTypeRegexPredicateAndReactiveEdits)
{
	spk::Engine engine;
	spk::Entity matching("matching"), reactive("reactive"), empty("empty");
	engine.addEntity(&matching);
	engine.addEntity(&reactive);
	engine.addEntity(&empty);
	auto &participant = matching.addParticipant<QueryParticipant>("selected-participant", 42);

	auto byType = spk::Registry<spk::Engine *, spk::Entity>::query();
	byType.insert<spk::ContainParticipant<QueryParticipant>>();
	EXPECT_TRUE(byType.collect(&engine).contains(&matching));
	EXPECT_FALSE(byType.collect(&engine).contains(&empty));

	auto byName = spk::Registry<spk::Engine *, spk::Entity>::query();
	byName.insert<spk::ContainParticipant<QueryParticipant>>(std::regex("^selected"));
	EXPECT_TRUE(byName.collect(&engine).contains(&matching));

	auto byPredicate = spk::Registry<spk::Engine *, spk::Entity>::query();
	byPredicate.insert<spk::ContainParticipant<QueryParticipant>>([](QueryParticipant *candidate) {
		return candidate->value == 42;
	});
	EXPECT_TRUE(byPredicate.collect(&engine).contains(&matching));

	EXPECT_FALSE(byType.collect(&engine).contains(&reactive));
	auto &added = reactive.addParticipant<QueryParticipant>("later", 7);
	EXPECT_TRUE(byType.collect(&engine).contains(&reactive));
	reactive.removeParticipant(added);
	EXPECT_FALSE(byType.collect(&engine).contains(&reactive));
	EXPECT_EQ(participant.owner(), &matching);
}

TEST(RegistryQueryTest, ContainBehaviourSupportsTypeRegexPredicateAndReactiveEdits)
{
	spk::Engine engine;
	spk::Entity matching("matching"), reactive("reactive"), empty("empty");
	engine.addEntity(&matching);
	engine.addEntity(&reactive);
	engine.addEntity(&empty);
	matching.addBehaviour<QueryBehaviour>("selected-behaviour", 42);

	auto byType = spk::Registry<spk::Engine *, spk::Entity>::query();
	byType.insert<spk::ContainBehaviour<QueryBehaviour>>();
	EXPECT_TRUE(byType.collect(&engine).contains(&matching));
	EXPECT_FALSE(byType.collect(&engine).contains(&empty));

	auto byName = spk::Registry<spk::Engine *, spk::Entity>::query();
	byName.insert<spk::ContainBehaviour<QueryBehaviour>>(std::regex("behaviour$"));
	EXPECT_TRUE(byName.collect(&engine).contains(&matching));

	auto byPredicate = spk::Registry<spk::Engine *, spk::Entity>::query();
	byPredicate.insert<spk::ContainBehaviour<QueryBehaviour>>([](QueryBehaviour *candidate) {
		return candidate->value == 42;
	});
	EXPECT_TRUE(byPredicate.collect(&engine).contains(&matching));

	EXPECT_FALSE(byType.collect(&engine).contains(&reactive));
	auto &added = reactive.addBehaviour<QueryBehaviour>("later", 7);
	EXPECT_TRUE(byType.collect(&engine).contains(&reactive));
	reactive.removeBehaviour(added);
	EXPECT_FALSE(byType.collect(&engine).contains(&reactive));
}

TEST(RegistryQueryTest, AttachmentNameChangesInvalidateRegexQueries)
{
	spk::Engine engine;
	spk::Entity entity("entity");
	engine.addEntity(&entity);
	auto &participant = entity.addParticipant<QueryParticipant>("before");
	auto &behaviour = entity.addBehaviour<QueryBehaviour>("before");

	auto participantQuery = spk::Registry<spk::Engine *, spk::Entity>::query();
	participantQuery.insert<spk::ContainParticipant<QueryParticipant>>(std::regex("^after$"));
	auto behaviourQuery = spk::Registry<spk::Engine *, spk::Entity>::query();
	behaviourQuery.insert<spk::ContainBehaviour<QueryBehaviour>>(std::regex("^after$"));
	EXPECT_FALSE(participantQuery.collect(&engine).contains(&entity));
	EXPECT_FALSE(behaviourQuery.collect(&engine).contains(&entity));

	participant.setName("after");
	behaviour.setName("after");
	EXPECT_TRUE(participantQuery.collect(&engine).contains(&entity));
	EXPECT_TRUE(behaviourQuery.collect(&engine).contains(&entity));

	participant.setName("before");
	behaviour.setName("before");
	EXPECT_FALSE(participantQuery.collect(&engine).contains(&entity));
	EXPECT_FALSE(behaviourQuery.collect(&engine).contains(&entity));
}
