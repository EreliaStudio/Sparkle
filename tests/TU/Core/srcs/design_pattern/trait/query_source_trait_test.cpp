#include <gtest/gtest.h>

#include <concepts>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "design_pattern/trait/query_source_trait.hpp"

namespace
{
	struct Element
	{
	};

	class MissingElementsSource : public spk::QuerySourceTrait<Element, int>
	{
	};

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
}

static_assert(std::is_abstract_v<MissingElementsSource>);
static_assert(std::derived_from<TestSource, spk::QuerySourceTrait<Element, int>>);

TEST(QuerySourceTraitTest, ElementsDispatchesThroughTheTraitReference)
{
	Element element;
	TestSource source;
	source.set(4, {&element});

	spk::QuerySourceTrait<Element, int> &trait = source;
	EXPECT_EQ(trait.elements(4), TestSource::ElementSet({&element}));
}

TEST(QuerySourceTraitTest, EditionSubscriptionReceivesTheEditedContext)
{
	TestSource source;
	int receivedContext = 0;
	auto contract = source.subscribeToEdition([&receivedContext](const int &context) {
		receivedContext = context;
	});

	source.set(17, {});
	EXPECT_EQ(receivedContext, 17);
}

TEST(QuerySourceTraitTest, DestroyingTheContractStopsEditionNotifications)
{
	TestSource source;
	int notificationCount = 0;
	{
		auto contract = source.subscribeToEdition([&notificationCount](const int &) {
			++notificationCount;
		});
		source.set(1, {});
	}

	source.set(2, {});
	EXPECT_EQ(notificationCount, 1);
}
