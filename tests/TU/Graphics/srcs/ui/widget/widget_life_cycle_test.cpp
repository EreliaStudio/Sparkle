#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ui/widget.hpp"

TEST(WidgetLifeCycleTest, PreservesPointerCallbackAndExistingContractType)
{
	std::vector<std::string> calls;
	auto widget = std::make_unique<spk::Widget>("widget", nullptr);
	spk::Widget *expected = widget.get();

	spk::Widget::DestructionContract legacy = widget->subscribeToDestruction([&](spk::Widget *observed) {
		EXPECT_EQ(observed, expected);
		calls.push_back("legacy");
	});
	auto release = widget->subscribeToRelease([&]() {
		calls.push_back("release");
	});
	auto modern = widget->spk::LifeCycle::subscribeToDestruction([&]() {
		calls.push_back("destruction");
	});

	widget.reset();
	EXPECT_EQ(calls, (std::vector<std::string>{"release", "legacy", "destruction"}));
	EXPECT_FALSE(legacy.isValid());
	EXPECT_FALSE(modern.isValid());
}

TEST(WidgetLifeCycleTest, DestructorContinuesAfterLegacyCallbackThrows)
{
	auto widget = std::make_unique<spk::Widget>("widget", nullptr);
	auto legacy = widget->subscribeToDestruction([](spk::Widget *) {
		throw std::runtime_error("legacy failure");
	});

	EXPECT_NO_THROW(widget.reset());
}
