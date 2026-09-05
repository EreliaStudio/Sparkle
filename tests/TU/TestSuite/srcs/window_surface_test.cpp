#include <gtest/gtest.h>

#include <stdexcept>
#include <type_traits>

#include "core/window.hpp"

static_assert(!std::is_copy_constructible_v<spk::Window::Surface>);
static_assert(!std::is_copy_assignable_v<spk::Window::Surface>);
static_assert(!std::is_move_constructible_v<spk::Window::Surface>);
static_assert(!std::is_move_assignable_v<spk::Window::Surface>);

namespace
{
	[[nodiscard]] spk::Rect2D testGeometry()
	{
		return spk::Rect2D{
			.anchor = {11, 17},
			.size = {320, 240}};
	}
}

TEST(WindowSurfaceTest, CpuSideGeometryAndResourceCollectionAreAvailableWhilePending)
{
	spk::Window::Surface surface("surface-standard");
	const spk::Rect2D geometry = testGeometry();

	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Pending);
	surface.setGeometry(geometry);
	EXPECT_EQ(surface.geometry(), geometry);

	EXPECT_NO_THROW(surface._gpuResources().reclaimReleased());
	EXPECT_NO_THROW(surface._gpuResources().clear());
}

TEST(WindowSurfaceTest, GeometryCanBeUpdatedBeforeCreationAndAfterRelease)
{
	spk::Window::Surface surface("surface-geometry");
	const spk::Rect2D first = testGeometry();
	const spk::Rect2D second{
		.anchor = {3, 5},
		.size = {64, 96}};

	surface.setGeometry(first);
	EXPECT_EQ(surface.geometry(), first);

	surface.destroy();
	ASSERT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Released);

	surface.setGeometry(second);
	EXPECT_EQ(surface.geometry(), second);
}

TEST(WindowSurfaceTest, MakeCurrentAndPresentBeforeInitializationThrowLogicError)
{
	spk::Window::Surface surface("surface-uninitialized");

	EXPECT_THROW(surface.makeCurrent(), std::logic_error);
	EXPECT_THROW(surface.present(), std::logic_error);
}

TEST(WindowSurfaceTest, CreateWithoutANativeWindowThrowsLogicError)
{
	spk::Window::Native native("native-without-frame");
	spk::Window::Surface surface("surface-without-frame");

	EXPECT_THROW(surface.create(native.window()), std::logic_error);
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Pending);
}

TEST(WindowSurfaceTest, CreateFromReleasedStateThrowsLogicError)
{
	spk::Window::Native native("native-released-create");
	spk::Window::Surface surface("surface-released-create");

	surface.destroy();
	ASSERT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Released);

	EXPECT_THROW(surface.create(native.window()), std::logic_error);
}

TEST(WindowSurfaceTest, DestroyIsSafeAndIdempotentFromPendingState)
{
	spk::Window::Surface surface("surface-destroy");

	EXPECT_NO_THROW(surface.destroy());
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Released);

	EXPECT_NO_THROW(surface.destroy());
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Released);
}

TEST(WindowSurfaceTest, FailedCreationCanBeRetriedBecauseTheLifecycleRemainsPending)
{
	spk::Window::Native native("native-retry");
	spk::Window::Surface surface("surface-retry");

	EXPECT_THROW(surface.create(native.window()), std::logic_error);
	ASSERT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Pending);
	EXPECT_THROW(surface.create(native.window()), std::logic_error);
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Pending);
}

TEST(WindowSurfaceTest, DISABLED_StandardNativeCreationMakeCurrentPresentAndDestroy)
{
	GTEST_SKIP() << "A valid WinAPI::Window creation fixture is not part of the supplied section-05 archive. This test should create a hidden native frame, create the surface, verify Ready, make it current, reclaim resources, present, destroy, and verify Released.";
}

TEST(WindowSurfaceTest, DISABLED_RepeatedSuccessfulCreateIsRejected)
{
	GTEST_SKIP() << "Requires the shared hidden native-window fixture. After one successful create(), a second create() must throw std::logic_error.";
}

TEST(WindowSurfaceTest, DISABLED_DestroyFromPartiallyInitializedOpenGLStateCleansUp)
{
	GTEST_SKIP() << "The public API cannot intentionally stop WGL setup between device-context/bootstrap/render-context stages. Enable with a fault-injection WinAPI/OpenGL fixture.";
}

TEST(WindowSurfaceTest, DISABLED_UnsupportedWGLAndWin32SetupFailuresPropagatePrecisely)
{
	GTEST_SKIP() << "Capability-aware WGL failure injection is not exposed by the supplied public snapshots. Cover std::runtime_error/std::system_error when the platform test fixture can substitute WGL/Win32 capabilities.";
}
