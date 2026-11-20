#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ui/widget.hpp"

TEST(WidgetLifeCycleTest, NotifiesDirectlyThroughLifeCycle)
{
	std::vector<std::string> calls;
	auto widget = std::make_unique<spk::Widget>("widget", nullptr);

	auto destruction = widget->subscribeToDestruction([&]() {
		calls.push_back("destruction");
	});
	auto release = widget->subscribeToRelease([&]() {
		calls.push_back("release");
	});

	widget.reset();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "destruction"}));
	EXPECT_FALSE(release.isValid());
	EXPECT_FALSE(destruction.isValid());
}

TEST(WidgetLifeCycleTest, DestructorContinuesAfterDestructionCallbackThrows)
{
	auto widget = std::make_unique<spk::Widget>("widget", nullptr);
	auto destruction = widget->subscribeToDestruction([]() {
		throw std::runtime_error("destruction failure");
	});

	EXPECT_NO_THROW(widget.reset());
	EXPECT_FALSE(destruction.isValid());
}
