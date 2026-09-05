#include <gtest/gtest.h>

#include <type_traits>

#include "core/window.hpp"

static_assert(!std::is_copy_constructible_v<spk::Window::State>);
static_assert(!std::is_copy_assignable_v<spk::Window::State>);
static_assert(!std::is_move_constructible_v<spk::Window::State>);
static_assert(!std::is_move_assignable_v<spk::Window::State>);

namespace
{
	constexpr spk::FocusMode::Channel Channel0 = static_cast<spk::FocusMode::Channel>(0);
	constexpr spk::FocusMode::Channel Channel1 = static_cast<spk::FocusMode::Channel>(1);
}

TEST(WindowStateTest, StandardRootDevicesBackgroundFocusAndLifecycleUsage)
{
	spk::Window::State state("state-standard");
	spk::Window::State otherState("state-other");

	EXPECT_EQ(state.lifeCycle(), spk::Window::LifeCycle::Pending);

	auto *root = &state.root();
	const spk::Window::State &constState = state;
	EXPECT_EQ(&constState.root(), root);
	EXPECT_EQ(static_cast<const void *>(&constState.keyboard()), static_cast<const void *>(&state.keyboard()));
	EXPECT_EQ(static_cast<const void *>(&constState.mouse()), static_cast<const void *>(&state.mouse()));

	EXPECT_NO_THROW(state.setBackgroundColor(spk::Color{0.2f, 0.3f, 0.4f, 1.0f}));

	state.takeFocus(Channel0, &otherState.root());
	EXPECT_EQ(state.focusedWidget(Channel0), &otherState.root());
	EXPECT_EQ(&state.dispatchRoot(Channel0), &otherState.root());
	EXPECT_EQ(&state.dispatchRoot(Channel1), root);

	state.markReady();
	state.beginRelease();
	state.markReleased();
	EXPECT_EQ(state.lifeCycle(), spk::Window::LifeCycle::Released);
}

TEST(WindowStateTest, TakingFocusReplacesThePreviousOwner)
{
	spk::Window::State state("state");
	spk::Window::State firstOwner("first-owner");
	spk::Window::State secondOwner("second-owner");

	state.takeFocus(Channel0, &firstOwner.root());
	ASSERT_EQ(state.focusedWidget(Channel0), &firstOwner.root());

	state.takeFocus(Channel0, &secondOwner.root());
	EXPECT_EQ(state.focusedWidget(Channel0), &secondOwner.root());
	EXPECT_EQ(&state.dispatchRoot(Channel0), &secondOwner.root());
}

TEST(WindowStateTest, ReleasingFocusWithTheWrongWidgetDoesNothing)
{
	spk::Window::State state("state");
	spk::Window::State focusedOwner("focused-owner");
	spk::Window::State otherOwner("other-owner");

	state.takeFocus(Channel0, &focusedOwner.root());
	state.releaseFocus(Channel0, &otherOwner.root());

	EXPECT_EQ(state.focusedWidget(Channel0), &focusedOwner.root());
}

TEST(WindowStateTest, NullFocusAndClearWithNoOwnerFallBackToRoot)
{
	spk::Window::State state("state");

	EXPECT_EQ(state.focusedWidget(Channel0), nullptr);
	state.takeFocus(Channel0, nullptr);
	EXPECT_EQ(state.focusedWidget(Channel0), nullptr);
	EXPECT_EQ(&state.dispatchRoot(Channel0), &state.root());

	state.clearFocus(Channel0);
	state.clearFocus(Channel0);
	EXPECT_EQ(state.focusedWidget(Channel0), nullptr);
	EXPECT_EQ(&state.dispatchRoot(Channel0), &state.root());
}

TEST(WindowStateTest, ReleaseByCurrentOwnerClearsFocus)
{
	spk::Window::State state("state");
	spk::Window::State owner("owner");

	state.takeFocus(Channel0, &owner.root());
	state.releaseFocus(Channel0, &owner.root());

	EXPECT_EQ(state.focusedWidget(Channel0), nullptr);
	EXPECT_EQ(&state.dispatchRoot(Channel0), &state.root());
}

TEST(WindowStateTest, FocusChannelsAreIndependent)
{
	spk::Window::State state("state");
	spk::Window::State mouseOwner("mouse-owner");
	spk::Window::State keyboardOwner("keyboard-owner");

	state.takeFocus(Channel0, &mouseOwner.root());
	state.takeFocus(Channel1, &keyboardOwner.root());

	EXPECT_EQ(state.focusedWidget(Channel0), &mouseOwner.root());
	EXPECT_EQ(state.focusedWidget(Channel1), &keyboardOwner.root());

	state.clearFocus(Channel0);
	EXPECT_EQ(state.focusedWidget(Channel0), nullptr);
	EXPECT_EQ(state.focusedWidget(Channel1), &keyboardOwner.root());
}

TEST(WindowStateTest, RepeatedLifecycleNotificationsAreTolerated)
{
	spk::Window::State state("state-lifecycle");

	state.markReady();
	state.markReady();
	EXPECT_EQ(state.lifeCycle(), spk::Window::LifeCycle::Ready);

	state.beginRelease();
	state.beginRelease();
	EXPECT_EQ(state.lifeCycle(), spk::Window::LifeCycle::Releasing);

	state.markReleased();
	state.markReleased();
	EXPECT_EQ(state.lifeCycle(), spk::Window::LifeCycle::Released);
}

TEST(WindowStateTest, DISABLED_InactiveFocusedWidgetHasDocumentedDispatchBehavior)
{
	GTEST_SKIP() << "The backlog requires inactive-focused-widget coverage, but the supplied window.hpp only forward-declares Widget. Enable this case beside the Widget tests once the complete Widget API is part of this handoff.";
}

TEST(WindowStateTest, DISABLED_BackgroundColorMutationIsObservableInProducedSnapshot)
{
	GTEST_SKIP() << "State exposes setBackgroundColor() but no getter or public snapshot-building hook in the supplied section-05 API. Enable with the update/snapshot fixture and assert the clear command receives the configured color.";
}
