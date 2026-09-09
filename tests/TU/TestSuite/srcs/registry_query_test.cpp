#include <gtest/gtest.h>

#include <memory>
#include <regex>
#include <type_traits>
#include <unordered_map>

#include "engine/engine.hpp"
#include "engine/query_operations.hpp"
#include "query/operations.hpp"
#include "container/query.hpp"
#include "engine/registry.hpp"

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
		DerivedQueryObject(int context, int value) :
			QueryObject(context, value)
		{
		}
	};

	using Registry = spk::Registry<int, QueryObject>;
	using Query = spk::Query<QueryObject, int>;
	using ElementSet = Registry::ElementSet;

	template <typename TType>
	class TestSource final : public spk::QuerySourceTrait<TType, int>
	{
	public:
		using Base = spk::QuerySourceTrait<TType, int>;
		using ElementSet = typename Base::ElementSet;

	private:
		ElementSet _defaultElements;
		std::unordered_map<int, ElementSet> _elements;

	public:
		TestSource() = default;

		explicit TestSource(ElementSet elements)
			: _defaultElements(std::move(elements))
		{
		}

		void set(const int &context, ElementSet elements)
		{
			_elements[context] = std::move(elements);
			this->notifyEdition(context);
		}

		[[nodiscard]] const ElementSet &elements(const int &context) const override
		{
			auto it = _elements.find(context);
			return it == _elements.end() ? _defaultElements : it->second;
		}
	};

	class KeepEven final : public Query::Operation
	{
	public:
		void execute(ElementSet &currentElements, const int &) override
		{
			for (auto it = currentElements.begin(); it != currentElements.end();)
			{
				if ((*it)->value % 2 != 0)
				{
					it = currentElements.erase(it);
				}
				else
				{
					++it;
				}
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

	TestSource<DerivedQueryObject> derivedProvider({&derived});
	TestSource<QueryObject> baseProvider({&even});

	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<KeepEven>()
		.insert<spk::Intersect<QueryObject, int>>(derivedProvider)
		.insert<spk::Union<QueryObject, int>>(baseProvider);

	const auto &result = query.elements(10);
	ASSERT_EQ(result.size(), 2u);
	EXPECT_TRUE(result.contains(&derived));
	EXPECT_TRUE(result.contains(&even));
	EXPECT_FALSE(result.contains(&odd));
}

TEST(RegistryQueryTest, EmptyAndAllRegistrySelectionsAreHandled)
{
	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider());
	EXPECT_TRUE(query.elements(1001).empty());

	QueryObject first(1002, 1);
	QueryObject second(1002, 2);
	const auto &all = query.elements(1002);
	EXPECT_EQ(all.size(), 2u);
	EXPECT_TRUE(all.contains(&first));
	EXPECT_TRUE(all.contains(&second));
}

TEST(RegistryQueryTest, RepeatedElementsUsesCacheUntilTheRegistryChanges)
{
	auto executions = std::make_shared<std::unordered_map<int, int>>();
	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<CountingOperation>(executions);

	QueryObject first(77, 1);
	EXPECT_EQ(query.elements(77).size(), 1u);
	EXPECT_EQ((*executions)[77], 1);

	EXPECT_EQ(query.elements(77).size(), 1u);
	EXPECT_EQ((*executions)[77], 1);

	QueryObject second(77, 2);
	EXPECT_EQ(query.elements(77).size(), 2u);
	EXPECT_EQ((*executions)[77], 2);
}

TEST(RegistryQueryTest, RegistryEditsInvalidateOnlyTheAffectedContext)
{
	auto executions = std::make_shared<std::unordered_map<int, int>>();
	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<CountingOperation>(executions);

	QueryObject first(1, 1);
	QueryObject second(2, 2);
	(void)query.elements(1);
	(void)query.elements(2);
	ASSERT_EQ((*executions)[1], 1);
	ASSERT_EQ((*executions)[2], 1);

	QueryObject addedOnlyToFirstContext(1, 3);
	(void)query.elements(2);
	EXPECT_EQ((*executions)[2], 1);
	(void)query.elements(1);
	EXPECT_EQ((*executions)[1], 2);
}

TEST(RegistryQueryTest, MovingAnObjectBetweenContextsRefreshesBothSelections)
{
	QueryObject object(3, 9);
	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider());

	ASSERT_TRUE(query.elements(3).contains(&object));
	ASSERT_FALSE(query.elements(4).contains(&object));

	object.changeContext(4);

	EXPECT_FALSE(query.elements(3).contains(&object));
	EXPECT_TRUE(query.elements(4).contains(&object));
}

TEST(RegistryQueryTest, IntersectionWithExplicitSetDoesNotSubscribeToRegistryEdits)
{
	DerivedQueryObject selected(5, 1);
	TestSource<DerivedQueryObject> explicitProvider({&selected});

	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<spk::Intersect<QueryObject, int>>(explicitProvider);
	ASSERT_TRUE(query.elements(5).contains(&selected));

	DerivedQueryObject later(5, 2);
	const auto &result = query.elements(5);
	EXPECT_TRUE(result.contains(&selected));
	EXPECT_FALSE(result.contains(&later));
}

TEST(RegistryQueryTest, RegistryBackedIntersectionInvalidatesWhenOtherRegistryChanges)
{
	DerivedQueryObject first(6, 1);
	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<spk::Intersect<QueryObject, int>>(
			spk::Registry<int, DerivedQueryObject>::provider());
	ASSERT_TRUE(query.elements(6).contains(&first));

	DerivedQueryObject second(6, 2);
	const auto &result = query.elements(6);
	EXPECT_EQ(result.size(), 2u);
	EXPECT_TRUE(result.contains(&second));
}

TEST(RegistryQueryTest, QueryCompositionPropagatesOnlyTheEditedContext)
{
	auto executions = std::make_shared<std::unordered_map<int, int>>();
	Query evenQuery;
	evenQuery.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<KeepEven>();

	Query composedQuery;
	composedQuery.insert<spk::From<QueryObject, int>>(evenQuery)
		.insert<CountingOperation>(executions);

	QueryObject evenInFirstContext(20, 2);
	QueryObject evenInSecondContext(21, 4);
	EXPECT_TRUE(composedQuery.elements(20).contains(&evenInFirstContext));
	EXPECT_TRUE(composedQuery.elements(21).contains(&evenInSecondContext));
	ASSERT_EQ((*executions)[20], 1);
	ASSERT_EQ((*executions)[21], 1);

	QueryObject addedToFirstContext(20, 6);
	EXPECT_EQ(composedQuery.elements(21).size(), 1u);
	EXPECT_EQ((*executions)[21], 1);
	EXPECT_EQ(composedQuery.elements(20).size(), 2u);
	EXPECT_EQ((*executions)[20], 2);
}

TEST(RegistryQueryTest, InsertingAnOperationInvalidatesComposedCachedResults)
{
	Query sourceQuery;
	sourceQuery.insert<spk::From<QueryObject, int>>(Registry::provider());

	Query composedQuery;
	composedQuery.insert<spk::From<QueryObject, int>>(sourceQuery);

	QueryObject odd(22, 1);
	QueryObject even(22, 2);
	ASSERT_EQ(composedQuery.elements(22).size(), 2u);

	sourceQuery.insert<KeepEven>();
	EXPECT_EQ(composedQuery.elements(22), ElementSet({&even}));
}

TEST(RegistryQueryTest, WhereAndExcludeComposeWithProviders)
{
	QueryObject odd(30, 1);
	QueryObject even(30, 2);
	QueryObject otherContext(31, 2);
	TestSource<QueryObject> excluded({&even});

	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<spk::Where<QueryObject, int>>([](QueryObject *object, const int &context) {
			return object->value > 0 && context == 30;
		})
		.insert<spk::Exclude<QueryObject, int>>(excluded);

	EXPECT_EQ(query.elements(30), ElementSet({&odd}));
	EXPECT_TRUE(query.elements(31).empty());
}

TEST(RegistryQueryTest, OperationOrderIsObservable)
{
	QueryObject baseOnly(8, 1);
	DerivedQueryObject derived(8, 2);
	TestSource<QueryObject> addBack({&baseOnly});

	Query intersectThenUnion;
	intersectThenUnion
		.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<spk::Intersect<QueryObject, int>>(
			spk::Registry<int, DerivedQueryObject>::provider())
		.insert<spk::Union<QueryObject, int>>(addBack);

	Query unionThenIntersect;
	unionThenIntersect
		.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<spk::Union<QueryObject, int>>(addBack)
		.insert<spk::Intersect<QueryObject, int>>(
			spk::Registry<int, DerivedQueryObject>::provider());

	EXPECT_EQ(intersectThenUnion.elements(8).size(), 2u);
	EXPECT_EQ(unionThenIntersect.elements(8).size(), 1u);
	EXPECT_TRUE(unionThenIntersect.elements(8).contains(&derived));
}

TEST(RegistryQueryTest, SetSemanticsSuppressDuplicatesIntroducedByUnion)
{
	QueryObject object(9, 1);
	TestSource<QueryObject> explicitProvider({&object});

	Query query;
	query.insert<spk::From<QueryObject, int>>(Registry::provider())
		.insert<spk::Union<QueryObject, int>>(explicitProvider)
		.insert<spk::Union<QueryObject, int>>(explicitProvider);

	EXPECT_EQ(query.elements(9).size(), 1u);
}

TEST(RegistryQueryTest, DestroyedQueryReleasesItsRegistrySubscriptions)
{
	QueryObject first(12, 1);
	{
		Query query;
		query.insert<spk::From<QueryObject, int>>(Registry::provider());
		ASSERT_TRUE(query.elements(12).contains(&first));
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

	spk::Query<spk::Entity, spk::Engine *> byType;
	byType.insert<spk::From<spk::Entity, spk::Engine *>>(
			  spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainParticipant<QueryParticipant>>();
	EXPECT_TRUE(byType.elements(&engine).contains(&matching));
	EXPECT_FALSE(byType.elements(&engine).contains(&empty));

	spk::Query<spk::Entity, spk::Engine *> byName;
	byName.insert<spk::From<spk::Entity, spk::Engine *>>(
			  spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainParticipant<QueryParticipant>>(std::regex("^selected"));
	EXPECT_TRUE(byName.elements(&engine).contains(&matching));

	spk::Query<spk::Entity, spk::Engine *> byPredicate;
	byPredicate.insert<spk::From<spk::Entity, spk::Engine *>>(
				   spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainParticipant<QueryParticipant>>([](QueryParticipant *candidate) {
			return candidate->value == 42;
		});
	EXPECT_TRUE(byPredicate.elements(&engine).contains(&matching));

	EXPECT_FALSE(byType.elements(&engine).contains(&reactive));
	auto &added = reactive.addParticipant<QueryParticipant>("later", 7);
	EXPECT_TRUE(byType.elements(&engine).contains(&reactive));
	reactive.removeParticipant(added);
	EXPECT_FALSE(byType.elements(&engine).contains(&reactive));
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

	spk::Query<spk::Entity, spk::Engine *> byType;
	byType.insert<spk::From<spk::Entity, spk::Engine *>>(
			  spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainBehaviour<QueryBehaviour>>();
	EXPECT_TRUE(byType.elements(&engine).contains(&matching));
	EXPECT_FALSE(byType.elements(&engine).contains(&empty));

	spk::Query<spk::Entity, spk::Engine *> byName;
	byName.insert<spk::From<spk::Entity, spk::Engine *>>(
			  spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainBehaviour<QueryBehaviour>>(std::regex("behaviour$"));
	EXPECT_TRUE(byName.elements(&engine).contains(&matching));

	spk::Query<spk::Entity, spk::Engine *> byPredicate;
	byPredicate.insert<spk::From<spk::Entity, spk::Engine *>>(
				   spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainBehaviour<QueryBehaviour>>([](QueryBehaviour *candidate) {
			return candidate->value == 42;
		});
	EXPECT_TRUE(byPredicate.elements(&engine).contains(&matching));

	EXPECT_FALSE(byType.elements(&engine).contains(&reactive));
	auto &added = reactive.addBehaviour<QueryBehaviour>("later", 7);
	EXPECT_TRUE(byType.elements(&engine).contains(&reactive));
	reactive.removeBehaviour(added);
	EXPECT_FALSE(byType.elements(&engine).contains(&reactive));
}

TEST(RegistryQueryTest, AttachmentNameChangesInvalidateRegexQueries)
{
	spk::Engine engine;
	spk::Entity entity("entity");
	engine.addEntity(&entity);
	auto &participant = entity.addParticipant<QueryParticipant>("before");
	auto &behaviour = entity.addBehaviour<QueryBehaviour>("before");

	spk::Query<spk::Entity, spk::Engine *> participantQuery;
	participantQuery.insert<spk::From<spk::Entity, spk::Engine *>>(
						spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainParticipant<QueryParticipant>>(std::regex("^after$"));
	spk::Query<spk::Entity, spk::Engine *> behaviourQuery;
	behaviourQuery.insert<spk::From<spk::Entity, spk::Engine *>>(
					  spk::Registry<spk::Engine *, spk::Entity>::provider())
		.insert<spk::ContainBehaviour<QueryBehaviour>>(std::regex("^after$"));
	EXPECT_FALSE(participantQuery.elements(&engine).contains(&entity));
	EXPECT_FALSE(behaviourQuery.elements(&engine).contains(&entity));

	participant.setName("after");
	behaviour.setName("after");
	EXPECT_TRUE(participantQuery.elements(&engine).contains(&entity));
	EXPECT_TRUE(behaviourQuery.elements(&engine).contains(&entity));

	participant.setName("before");
	behaviour.setName("before");
	EXPECT_FALSE(participantQuery.elements(&engine).contains(&entity));
	EXPECT_FALSE(behaviourQuery.elements(&engine).contains(&entity));
}
