#include <gtest/gtest.h>

#include <Windows.h>

#include <atomic>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "core/platform/window.hpp"
#include "core/window.hpp"

static_assert(!std::is_copy_constructible_v<spk::Window::Surface>);
static_assert(!std::is_copy_assignable_v<spk::Window::Surface>);
static_assert(!std::is_move_constructible_v<spk::Window::Surface>);
static_assert(!std::is_move_assignable_v<spk::Window::Surface>);

namespace
{
	[[nodiscard]] std::string uniqueClassName()
	{
		static std::atomic_uint64_t counter = 0;
		return "Sparkle_WindowSurfaceTest_" + std::to_string(::GetCurrentProcessId()) + "_" +
			std::to_string(counter.fetch_add(1));
	}

	class HiddenNativeWindow
	{
	private:
		spk::WinAPI::Window::Class _class{uniqueClassName()};

	public:
		spk::Window::Native native{"surface-test-native"};

		HiddenNativeWindow()
		{
			spk::WinAPI::Window::CreationInfo info;
			info.title = "Sparkle surface test";
			info.width = 32;
			info.height = 32;
			info.visible = false;
			native.window().create(_class, info);
		}

		~HiddenNativeWindow()
		{
			try
			{
				native.window().destroy();
			} catch (...)
			{
			}
		}
	};

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

TEST(WindowSurfaceTest, StandardNativeCreationMakeCurrentPresentAndDestroy)
{
	HiddenNativeWindow frame;
	spk::Window::Surface surface("surface-standard-native");
	surface.create(frame.native.window());
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Ready);
	EXPECT_NO_THROW(surface.makeCurrent());
	EXPECT_NE(::wglGetCurrentContext(), nullptr);
	EXPECT_NO_THROW(surface._gpuResources().reclaimReleased());
	EXPECT_NO_THROW(surface.present());
	surface.destroy();
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Released);
	EXPECT_EQ(::wglGetCurrentContext(), nullptr);
}

TEST(WindowSurfaceTest, RepeatedSuccessfulCreateIsRejected)
{
	HiddenNativeWindow frame;
	spk::Window::Surface surface("surface-repeated-create");
	surface.create(frame.native.window());
	EXPECT_THROW(surface.create(frame.native.window()), std::logic_error);
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Ready);
	EXPECT_NO_THROW(surface.makeCurrent());
	surface.destroy();
}
