#include <gtest/gtest.h>

#include <type_traits>

#include "core/window.hpp"

static_assert(!std::is_copy_constructible_v<spk::Window::Native>);
static_assert(!std::is_copy_assignable_v<spk::Window::Native>);
static_assert(!std::is_move_constructible_v<spk::Window::Native>);
static_assert(!std::is_move_assignable_v<spk::Window::Native>);

TEST(WindowNativeTest, StandardLifecycleAndNativeFrameAccess)
{
	spk::Window::Native native("native-standard");

	EXPECT_EQ(native.lifeCycle(), spk::Window::LifeCycle::Pending);

	auto *mutableFrame = &native.window();
	const spk::Window::Native &constNative = native;
	const auto *constFrame = &constNative.window();
	EXPECT_EQ(static_cast<const void *>(mutableFrame), static_cast<const void *>(constFrame));

	native.markReady();
	EXPECT_EQ(native.lifeCycle(), spk::Window::LifeCycle::Ready);

	native.beginRelease();
	EXPECT_EQ(native.lifeCycle(), spk::Window::LifeCycle::Releasing);

	native.markReleased();
	EXPECT_EQ(native.lifeCycle(), spk::Window::LifeCycle::Released);
}

TEST(WindowNativeTest, RepeatedLifecycleNotificationsAreTolerated)
{
	spk::Window::Native native("native-repeated");

	native.markReady();
	native.markReady();
	EXPECT_EQ(native.lifeCycle(), spk::Window::LifeCycle::Ready);

	native.beginRelease();
	native.beginRelease();
	EXPECT_EQ(native.lifeCycle(), spk::Window::LifeCycle::Releasing);

	native.markReleased();
	native.markReleased();
	EXPECT_EQ(native.lifeCycle(), spk::Window::LifeCycle::Released);
}

TEST(WindowNativeTest, NativeFramesAreIndependentForDifferentIdentifiers)
{
	spk::Window::Native first("native-first");
	spk::Window::Native second("native-second");

	EXPECT_NE(&first.window(), &second.window());
}

TEST(WindowNativeTest, DestructionIsSafeFromEveryPublicLifecycleState)
{
	EXPECT_NO_THROW({ spk::Window::Native native("pending"); });
	EXPECT_NO_THROW({
		spk::Window::Native native("ready");
		native.markReady();
	});
	EXPECT_NO_THROW({
		spk::Window::Native native("releasing");
		native.markReady();
		native.beginRelease();
	});
	EXPECT_NO_THROW({
		spk::Window::Native native("released");
		native.markReady();
		native.beginRelease();
		native.markReleased();
	});
}

TEST(WindowNativeTest, DISABLED_IdentifierPreservationNeedsObservableAccessor)
{
	GTEST_SKIP() << "Window::Native stores an identifier internally, but the supplied public snapshot exposes no identifier accessor or other deterministic observable carrying it.";
}
