#include <gtest/gtest.h>

#include <cstdlib>
#include <stdexcept>
#include <type_traits>

#include "core/application.hpp"

static_assert(!std::is_copy_constructible_v<spk::Application>);
static_assert(!std::is_copy_assignable_v<spk::Application>);
static_assert(!std::is_move_constructible_v<spk::Application>);
static_assert(!std::is_move_assignable_v<spk::Application>);

namespace
{
	[[nodiscard]] spk::Window::Configuration offscreenConfiguration(const char *title)
	{
		return spk::Window::Configuration{
			.title = title,
			.area = spk::Rect2D{
				.anchor = {-32000, -32000},
				.size = {32, 32}},
			.backgroundColor = spk::Color{0.05f, 0.05f, 0.08f, 1.0f}};
	}
}

TEST(ApplicationTest, EmptyApplicationCanQuitBeforeRunWithRequestedExitCode)
{
	spk::Application application;
	application.quit(23);

	EXPECT_EQ(application.run(), 23);
}

TEST(ApplicationTest, WindowLookupProvidesMutableAndConstAccess)
{
	spk::Application application;
	spk::Window &created = application.createWindow("lookup", offscreenConfiguration("lookup"));

	EXPECT_EQ(&application.window("lookup"), &created);
	const spk::Application &constApplication = application;
	EXPECT_EQ(&constApplication.window("lookup"), &created);

	application.closeWindow("lookup");
	application.quit();
	EXPECT_EQ(application.run(), EXIT_SUCCESS);
}

TEST(ApplicationTest, DuplicateWindowIdentifierThrowsLogicError)
{
	spk::Application application;
	application.createWindow("duplicate", offscreenConfiguration("first"));

	EXPECT_THROW(
		application.createWindow("duplicate", offscreenConfiguration("second")),
		std::logic_error);

	application.closeWindow("duplicate");
	application.quit();
	EXPECT_EQ(application.run(), EXIT_SUCCESS);
}

TEST(ApplicationTest, UnknownWindowLookupAndCloseThrowOutOfRange)
{
	spk::Application application;

	EXPECT_THROW((void)application.window("missing"), std::out_of_range);
	const spk::Application &constApplication = application;
	EXPECT_THROW((void)constApplication.window("missing"), std::out_of_range);
	EXPECT_THROW(application.closeWindow("missing"), std::out_of_range);
}

TEST(ApplicationTest, MultiplePendingWindowsCanBeClosedAndAllRuntimesJoin)
{
	spk::Application application;
	application.createWindow("first", offscreenConfiguration("first"));
	application.createWindow("second", offscreenConfiguration("second"));

	application.closeWindow("first");
	application.closeWindow("second");
	application.quit(7);

	EXPECT_EQ(application.run(), 7);
	EXPECT_THROW((void)application.window("first"), std::out_of_range);
	EXPECT_THROW((void)application.window("second"), std::out_of_range);
}

TEST(ApplicationTest, QuitBeforeRunningClosesCreatedWindowsAndPreservesExitCode)
{
	spk::Application application;
	application.createWindow("quit-before-run", offscreenConfiguration("quit-before-run"));
	application.quit(31);

	EXPECT_EQ(application.run(), 31);
	EXPECT_THROW((void)application.window("quit-before-run"), std::out_of_range);
}

TEST(ApplicationTest, DISABLED_StandardReadyWindowInitializationUpdateRenderAndClose)
{
	GTEST_SKIP() << "The public facade has no readiness/update/render synchronization hook. Enable with the suite's shared window-runtime fixture so the test can deterministically wait for Ready, one update and one rendered frame before closeWindow().";
}

TEST(ApplicationTest, DISABLED_CloseWhileReadyAndReleasingIsIdempotentlyCoordinated)
{
	GTEST_SKIP() << "Requires a deterministic runtime barrier exposing the Ready/Releasing milestones without polling private runtime state.";
}

TEST(ApplicationTest, DISABLED_QuitWhileRunIsActiveStopsAndJoinsAllRuntimes)
{
	GTEST_SKIP() << "Requires the shared runtime synchronization fixture so quit() can be issued after run() has deterministically entered its platform loop.";
}

TEST(ApplicationTest, DISABLED_EventRoutingTargetsOnlyTheMatchingWindow)
{
	GTEST_SKIP() << "Event publication is private to PlatformRuntime in the supplied public snapshot. Enable when the test harness exposes deterministic platform-event injection.";
}

TEST(ApplicationTest, DISABLED_DuplicateRuntimeObjectsAndSnapshotEndpointsThrowLogicError)
{
	GTEST_SKIP() << "Runtime registration and snapshot endpoint registration are private implementation details. This contract needs an internal test seam or friend test fixture.";
}

TEST(ApplicationTest, DISABLED_WorkerExceptionsCrossTheRunBoundary)
{
	GTEST_SKIP() << "No public deterministic fault-injection hook is present for UpdateRuntime/RenderRuntime. Enable when a throwing test workload can be injected into a worker.";
}

TEST(ApplicationTest, DISABLED_RuntimeFailuresReceiveApplicationContext)
{
	GTEST_SKIP() << "The requested spk::Exception/std::runtime_error contextual wrapping cannot be forced deterministically through the supplied facade alone; pair with platform/update/render fault injection.";
}

TEST(ApplicationTest, DISABLED_ReleasedNativeDuringSurfaceCreationIsReported)
{
	GTEST_SKIP() << "Native/Surface parts are intentionally hidden behind Application. A runtime test seam is required to release the native frame between platform creation and surface creation.";
}
