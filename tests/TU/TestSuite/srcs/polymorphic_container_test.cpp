#include <gtest/gtest.h>

#include "container/polymorphic_container.hpp"

#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace
{
	struct Base
	{
		std::string name;
		int *destructionCount = nullptr;

		explicit Base(std::string p_name, int *p_destructionCount = nullptr) :
			name(std::move(p_name)),
			destructionCount(p_destructionCount)
		{
		}

		virtual ~Base()
		{
			if (destructionCount != nullptr)
			{
				++(*destructionCount);
			}
		}
	};

	struct Intermediate : Base
	{
		using Base::Base;
	};

	struct Leaf : Intermediate
	{
		using Intermediate::Intermediate;
	};

	struct Other : Base
	{
		using Base::Base;
	};

	struct Unrelated : Base
	{
		using Base::Base;
	};

	class TestContainer : public spk::PolymorphicContainer<Base>
	{
	public:
		using AdditionContract = OnElementEditionContract;
		using RemovalContract = OnElementEditionContract;

		template <typename T, typename... TArguments>
		T &add(TArguments &&...arguments)
		{
			auto element = std::make_unique<T>(std::forward<TArguments>(arguments)...);
			T &result = *element;
			registerElement(std::move(element));
			return result;
		}

		void remove(Base &element)
		{
			unregisterElement(element);
		}

		[[nodiscard]] AdditionContract onAddition(OnElementEditionCallback callback)
		{
			return subscribeToElementAddition(std::move(callback));
		}

		[[nodiscard]] RemovalContract onRemoval(OnElementEditionCallback callback)
		{
			return subscribeToElementRemoval(std::move(callback));
		}

		template <typename T>
		[[nodiscard]] T *one()
		{
			return getElement<T>();
		}

		template <typename T, typename TPredicate>
		[[nodiscard]] T *one(const TPredicate &predicate)
		{
			return getElement<T>(predicate);
		}

		template <typename T>
		[[nodiscard]] const T *one() const
		{
			return getElement<T>();
		}

		template <typename T, typename TPredicate>
		[[nodiscard]] const T *one(const TPredicate &predicate) const
		{
			return getElement<T>(predicate);
		}

		template <typename T>
		[[nodiscard]] std::vector<T *> all()
		{
			return getElements<T>();
		}

		template <typename T, typename TPredicate>
		[[nodiscard]] std::vector<T *> all(const TPredicate &predicate)
		{
			return getElements<T>(predicate);
		}

		template <typename T>
		[[nodiscard]] std::vector<const T *> all() const
		{
			return getElements<T>();
		}

		template <typename T, typename TPredicate>
		[[nodiscard]] std::vector<const T *> all(const TPredicate &predicate) const
		{
			return getElements<T>(predicate);
		}

		[[nodiscard]] std::size_t size() const
		{
			return elements().size();
		}
	};
}

TEST(PolymorphicContainerTest, StandardUsageRegistersQueriesAndUnregistersDerivedElementsWithCallbacks)
{
	TestContainer container;
	std::vector<std::string> additions;
	std::vector<std::string> removals;
	auto additionContract = container.onAddition(
		[&additions](Base &element) { additions.push_back(element.name); });
	auto removalContract = container.onRemoval(
		[&removals](Base &element) { removals.push_back(element.name); });
	(void)additionContract;
	(void)removalContract;

	Leaf &first = container.add<Leaf>("first");
	Other &second = container.add<Other>("second");
	Leaf &third = container.add<Leaf>("third");

	EXPECT_EQ((std::vector<std::string>{"first", "second", "third"}), additions);
	EXPECT_EQ(container.size(), 3u);
	EXPECT_EQ(container.one<Leaf>(), &first);
	EXPECT_EQ(container.one<Other>(), &second);
	EXPECT_EQ(container.one<Leaf>([](Leaf *leaf) { return leaf->name == "third"; }), &third);

	const auto leaves = container.all<Leaf>();
	ASSERT_EQ(leaves.size(), 2u);
	EXPECT_EQ(leaves[0], &first);
	EXPECT_EQ(leaves[1], &third);

	container.remove(first);
	EXPECT_EQ((std::vector<std::string>{"first"}), removals);
	EXPECT_EQ(container.size(), 2u);
	EXPECT_EQ(container.one<Leaf>(), &third);
}

TEST(PolymorphicContainerTest, MutableAndConstQueriesReturnExpectedPointerTypesAndResults)
{
	TestContainer container;
	Leaf &leaf = container.add<Leaf>("leaf");
	container.add<Other>("other");

	EXPECT_EQ(container.one<Leaf>(), &leaf);
	const TestContainer &constContainer = container;
	EXPECT_EQ(constContainer.one<Leaf>(), &leaf);

	const auto mutableLeaves = container.all<Leaf>();
	const auto constLeaves = constContainer.all<Leaf>();
	ASSERT_EQ(mutableLeaves.size(), 1u);
	ASSERT_EQ(constLeaves.size(), 1u);
	EXPECT_EQ(mutableLeaves.front(), &leaf);
	EXPECT_EQ(constLeaves.front(), &leaf);
}

TEST(PolymorphicContainerTest, NoMatchReturnsNullOrEmptyCollection)
{
	TestContainer container;
	container.add<Other>("other");

	EXPECT_EQ(container.one<Leaf>(), nullptr);
	EXPECT_TRUE(container.all<Leaf>().empty());
	EXPECT_EQ(container.one<Other>([](Other *other) { return other->name == "missing"; }), nullptr);
}

TEST(PolymorphicContainerTest, QueryingIntermediateTypeIncludesFurtherDerivedObjects)
{
	TestContainer container;
	Intermediate &intermediate = container.add<Intermediate>("intermediate");
	Leaf &leaf = container.add<Leaf>("leaf");
	container.add<Other>("other");

	const auto matches = container.all<Intermediate>();
	ASSERT_EQ(matches.size(), 2u);
	EXPECT_EQ(matches[0], &intermediate);
	EXPECT_EQ(matches[1], &leaf);
}

TEST(PolymorphicContainerTest, PredicateWrappersCanImplementRegexLikeQueries)
{
	TestContainer container;
	Leaf &player = container.add<Leaf>("player.main");
	container.add<Leaf>("enemy.01");
	Leaf &enemyBoss = container.add<Leaf>("enemy.boss");
	const std::regex enemyExpression(R"(^enemy\..+$)");

	const auto enemies = container.all<Leaf>([&enemyExpression](Leaf *leaf) {
		return std::regex_match(leaf->name, enemyExpression);
	});
	ASSERT_EQ(enemies.size(), 2u);
	EXPECT_EQ(enemies[1], &enemyBoss);
	EXPECT_EQ(container.one<Leaf>([](Leaf *leaf) { return leaf->name == "player.main"; }), &player);
}

TEST(PolymorphicContainerTest, RegistrationOrderIsPreservedInMultiElementQueries)
{
	TestContainer container;
	Leaf &a = container.add<Leaf>("a");
	Leaf &b = container.add<Leaf>("b");
	Leaf &c = container.add<Leaf>("c");

	const auto leaves = container.all<Leaf>();
	ASSERT_EQ(leaves.size(), 3u);
	EXPECT_EQ(leaves[0], &a);
	EXPECT_EQ(leaves[1], &b);
	EXPECT_EQ(leaves[2], &c);
}

TEST(PolymorphicContainerTest, RemovingForeignElementIsANoOp)
{
	TestContainer container;
	container.add<Leaf>("owned");
	Leaf foreign("foreign");

	container.remove(foreign);
	EXPECT_EQ(container.size(), 1u);
	ASSERT_NE(container.one<Leaf>(), nullptr);
	EXPECT_EQ(container.one<Leaf>()->name, "owned");
}

TEST(PolymorphicContainerTest, NegativeTypeCacheDoesNotHideElementsAddedLater)
{
	TestContainer container;
	container.add<Other>("first");

	EXPECT_EQ(container.one<Leaf>(), nullptr);
	EXPECT_TRUE(container.all<Leaf>().empty());

	Leaf &addedLater = container.add<Leaf>("late leaf");
	EXPECT_EQ(container.one<Leaf>(), &addedLater);
	const auto leaves = container.all<Leaf>();
	ASSERT_EQ(leaves.size(), 1u);
	EXPECT_EQ(leaves.front(), &addedLater);
}

TEST(PolymorphicContainerTest, PositiveTypeCacheIsPurgedWhenElementIsRemoved)
{
	TestContainer container;
	Leaf &first = container.add<Leaf>("first");
	Leaf &second = container.add<Leaf>("second");

	ASSERT_EQ(container.all<Leaf>().size(), 2u);
	EXPECT_EQ(container.one<Leaf>(), &first);
	container.remove(first);

	const auto remaining = container.all<Leaf>();
	ASSERT_EQ(remaining.size(), 1u);
	EXPECT_EQ(remaining.front(), &second);
	EXPECT_EQ(container.one<Leaf>(), &second);
}

TEST(PolymorphicContainerTest, OwnershipDestructionAndRemovalCallbackTimingAreObservable)
{
	int destructionCount = 0;
	bool callbackSawLiveElement = false;
	TestContainer container;
	auto removalContract = container.onRemoval(
		[&callbackSawLiveElement, &destructionCount](Base &element) {
			callbackSawLiveElement = element.name == "owned" && destructionCount == 0;
		});
	(void)removalContract;

	Leaf &owned = container.add<Leaf>("owned", &destructionCount);
	EXPECT_EQ(destructionCount, 0);
	container.remove(owned);
	EXPECT_TRUE(callbackSawLiveElement);
	EXPECT_EQ(destructionCount, 1);
}

TEST(PolymorphicContainerTest, AdditionCallbackSeesLiveElementAlreadyOwnedByContainer)
{
	TestContainer container;
	Base *callbackAddress = nullptr;
	std::size_t sizeDuringCallback = 0;
	auto additionContract = container.onAddition(
		[&](Base &element) {
			callbackAddress = &element;
			sizeDuringCallback = container.size();
		});
	(void)additionContract;

	Leaf &leaf = container.add<Leaf>("leaf");
	EXPECT_EQ(callbackAddress, &leaf);
	EXPECT_EQ(sizeDuringCallback, 1u);
}

TEST(PolymorphicContainerTest, ContainerDestructionDestroysRemainingOwnedElementsExactlyOnce)
{
	int destructionCount = 0;
	{
		TestContainer container;
		container.add<Leaf>("one", &destructionCount);
		container.add<Other>("two", &destructionCount);
		EXPECT_EQ(destructionCount, 0);
	}
	EXPECT_EQ(destructionCount, 2);
}

TEST(PolymorphicContainerTest, DISABLED_UnregisteringAlreadyRemovedOwnedElementCannotBeCalledSafely)
{
	GTEST_SKIP() << "unregisterElement takes TBase&, while successful removal destroys the owned object before returning. Calling it again would require a dangling reference and would be undefined behavior.";
}
