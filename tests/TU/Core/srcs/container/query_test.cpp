#include <gtest/gtest.h>

#include <memory>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "container/query.hpp"

namespace
{
	struct Element
	{
		int value;
	};

	using Query = spk::Query<Element, int>;
	using ElementSet = Query::ElementSet;

	class TestSource final : public spk::QuerySourceTrait<Element, int>
	{
	public:
		using Base = spk::QuerySourceTrait<Element, int>;
		using ElementSet = typename Base::ElementSet;

	private:
		std::unordered_map<int, ElementSet> _elements;

	public:
		void set(const int &context, ElementSet elements)
		{
			_elements[context] = std::move(elements);
			this->notifyEdition(context);
		}

		[[nodiscard]] const ElementSet &elements(const int &context) const override
		{
			static const ElementSet empty;
			auto it = _elements.find(context);
			return it == _elements.end() ? empty : it->second;
		}
	};

	class FromSource final : public Query::Operation
	{
	private:
		spk::QuerySourceTrait<Element, int> &_source;
		typename spk::QuerySourceTrait<Element, int>::OnEditionContract _editionContract;

	public:
		explicit FromSource(spk::QuerySourceTrait<Element, int> &source) :
			_source(source)
		{
			_editionContract = _source.subscribeToEdition([this](const int &context) {
				this->invalidate(context);
			});
		}

		void execute(ElementSet &currentElements, const int &context) override
		{
			const ElementSet &sourceElements = _source.elements(context);
			currentElements.insert(sourceElements.begin(), sourceElements.end());
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

	class NoOp final : public Query::Operation
	{
	public:
		void execute(ElementSet &, const int &) override
		{
		}
	};
}

TEST(QueryTest, CopyAndMoveOperationsAreDisabled)
{
	static_assert(!std::is_copy_constructible_v<Query>);
	static_assert(!std::is_copy_assignable_v<Query>);
	static_assert(!std::is_move_constructible_v<Query>);
	static_assert(!std::is_move_assignable_v<Query>);
}

TEST(QueryTest, ElementsGeneratesLazilyAndCachesEachContextIndependently)
{
	Element first{1};
	Element second{2};
	TestSource source;
	source.set(10, {&first});
	source.set(20, {&second});
	auto executions = std::make_shared<std::unordered_map<int, int>>();

	Query query;
	query.insert<FromSource>(source)
		.insert<CountingOperation>(executions);
	EXPECT_TRUE(executions->empty());

	EXPECT_EQ(query.elements(10), ElementSet({&first}));
	EXPECT_EQ(query.elements(10), ElementSet({&first}));
	EXPECT_EQ(query.elements(20), ElementSet({&second}));
	EXPECT_EQ((*executions)[10], 1);
	EXPECT_EQ((*executions)[20], 1);

	source.set(10, {&first, &second});
	EXPECT_EQ(query.elements(20), ElementSet({&second}));
	EXPECT_EQ((*executions)[20], 1);
	EXPECT_EQ(query.elements(10), ElementSet({&first, &second}));
	EXPECT_EQ((*executions)[10], 2);
}

TEST(QueryTest, QueryCanBeTheSourceOfAnotherQuery)
{
	Element first{1};
	Element second{2};
	TestSource source;
	source.set(30, {&first});

	Query firstQuery;
	firstQuery.insert<FromSource>(source);
	Query secondQuery;
	secondQuery.insert<FromSource>(firstQuery);

	EXPECT_EQ(secondQuery.elements(30), ElementSet({&first}));
	source.set(30, {&first, &second});
	EXPECT_EQ(secondQuery.elements(30), ElementSet({&first, &second}));
}

TEST(QueryTest, InsertingAnOperationInvalidatesAndNotifiesEveryCachedContext)
{
	Element first{1};
	Element second{2};
	TestSource source;
	source.set(40, {&first});
	source.set(50, {&second});

	Query query;
	query.insert<FromSource>(source);
	(void)query.elements(40);
	(void)query.elements(50);

	std::vector<int> invalidatedContexts;
	auto contract = query.subscribeToEdition([&invalidatedContexts](const int &context) {
		invalidatedContexts.push_back(context);
	});
	query.insert<NoOp>();

	EXPECT_EQ(std::set(invalidatedContexts.begin(), invalidatedContexts.end()), std::set({40, 50}));
	EXPECT_EQ(invalidatedContexts.size(), 2u);
}
