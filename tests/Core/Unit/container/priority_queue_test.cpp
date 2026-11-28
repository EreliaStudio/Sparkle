#include <gtest/gtest.h>

#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "container/priority_queue.hpp"

TEST(PriorityQueueTest, SingleInsertMaintainsAscendingOrderWithoutDeduplication)
{
	spk::PriorityQueue<int> queue;
	queue.pushBack(3);
	queue.pushBack(1);
	queue.pushBack(2);
	queue.pushBack(1);

	EXPECT_EQ((std::vector<int>(queue.begin(), queue.end())), (std::vector<int>{1, 1, 2, 3}));
	EXPECT_EQ(queue.size(), 4u);
	EXPECT_EQ(queue[0], 1);
	queue.clear();
	EXPECT_TRUE(queue.empty());
}

TEST(PriorityQueueTest, BulkInsertAndOtherQueueUseDestinationComparator)
{
	spk::PriorityQueue<int, std::greater<int>> descending;
	descending.pushBack(std::vector<int>{5, 1, 4});
	spk::PriorityQueue<int> ascending;
	ascending.pushBack(std::vector<int>{2, 3});
	descending.pushBack(ascending);

	EXPECT_EQ((std::vector<int>(descending.begin(), descending.end())), (std::vector<int>{5, 4, 3, 2, 1}));
}

TEST(PriorityQueueTest, EqualPrioritiesPreserveInsertionOrder)
{
	struct Item
	{
		int priority;
		int identity;
	};
	auto compare = [](const Item &a, const Item &b) {
		return a.priority < b.priority;
	};
	spk::PriorityQueue<Item, decltype(compare)> queue(compare);
	queue.pushBack(std::vector<Item>{{2, 1}, {1, 2}, {2, 3}});
	queue.pushBack(Item{2, 4});

	std::vector<int> identities;
	for (const Item &item : queue)
	{
		identities.push_back(item.identity);
	}
	EXPECT_EQ(identities, (std::vector<int>{2, 1, 3, 4}));
}

TEST(PriorityQueueTest, ManualSortRespondsToExternalPriorityChanges)
{
	std::map<int, int> priorities{{1, 10}, {2, 20}, {3, 30}};
	auto compare = [&priorities](int left, int right) {
		return priorities.at(left) < priorities.at(right);
	};
	spk::PriorityQueue<int, decltype(compare)> queue(compare);
	queue.pushBack(std::vector<int>{3, 1, 2});
	EXPECT_EQ(queue[0], 1);

	priorities[3] = 0;
	queue.sort();
	EXPECT_EQ((std::vector<int>(queue.begin(), queue.end())), (std::vector<int>{3, 1, 2}));
}
