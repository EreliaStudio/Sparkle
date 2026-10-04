#include <gtest/gtest.h>

#include <limits>
#include <type_traits>
#include <utility>

#include "core/event/record.hpp"

TEST(MouseMovedEventTest, StandardUsage)
{
	spk::MouseMovedRecord record;
	record.position = {12, 34};
	spk::Mouse mouse;
	mouse.position = {10, 30};
	mouse.deltaPosition = {2, 4};

	spk::MouseMovedEvent event(record, mouse);
	EXPECT_EQ(&event.record, &record);
	EXPECT_EQ(&event.device, &mouse);
	EXPECT_EQ(event.record.position.x, 12);
	EXPECT_EQ(event.device.deltaPosition.x, 2);
	EXPECT_FALSE(event.mousePositionRequest().has_value());

	event.requestMousePosition({100, 200});
	ASSERT_TRUE(event.mousePositionRequest().has_value());
	EXPECT_EQ(event.mousePositionRequest()->x, 100);
	EXPECT_EQ(event.mousePositionRequest()->y, 200);
}

TEST(MouseMovedEventTest, StartsWithoutRequest)
{
	spk::MouseMovedRecord record;
	spk::Mouse mouse;
	const spk::MouseMovedEvent event(record, mouse);
	EXPECT_FALSE(event.mousePositionRequest().has_value());
}

TEST(MouseMovedEventTest, RepeatedRequestsKeepLatestValue)
{
	spk::MouseMovedRecord record;
	spk::Mouse mouse;
	spk::MouseMovedEvent event(record, mouse);

	event.requestMousePosition({1, 2});
	event.requestMousePosition({3, 4});

	ASSERT_TRUE(event.mousePositionRequest().has_value());
	EXPECT_EQ(event.mousePositionRequest()->x, 3);
	EXPECT_EQ(event.mousePositionRequest()->y, 4);
}

TEST(MouseMovedEventTest, AcceptsNegativeAndLargeCoordinates)
{
	spk::MouseMovedRecord record;
	spk::Mouse mouse;
	spk::MouseMovedEvent event(record, mouse);

	event.requestMousePosition({-123456, 987654});
	ASSERT_TRUE(event.mousePositionRequest().has_value());
	EXPECT_EQ(event.mousePositionRequest()->x, -123456);
	EXPECT_EQ(event.mousePositionRequest()->y, 987654);

	event.requestMousePosition({std::numeric_limits<int>::min(), std::numeric_limits<int>::max()});
	EXPECT_EQ(event.mousePositionRequest()->x, std::numeric_limits<int>::min());
	EXPECT_EQ(event.mousePositionRequest()->y, std::numeric_limits<int>::max());
}

TEST(MouseMovedEventTest, RequestAndAccessAreNoThrow)
{
	static_assert(noexcept(std::declval<spk::MouseMovedEvent &>().requestMousePosition(std::declval<const spk::Vector2Int &>())));
	static_assert(noexcept(std::declval<const spk::MouseMovedEvent &>().mousePositionRequest()));

	spk::MouseMovedRecord record;
	spk::Mouse mouse;
	spk::MouseMovedEvent event(record, mouse);
	EXPECT_NO_THROW(event.requestMousePosition({5, 6}));
	EXPECT_NO_THROW((void)event.mousePositionRequest());
}
