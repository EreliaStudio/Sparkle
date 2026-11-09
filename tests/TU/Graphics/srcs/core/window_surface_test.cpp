#include <gtest/gtest.h>

#include <GL/glew.h>
#include <Windows.h>

#include <array>
#include <atomic>
#include <stdexcept>
#include <vector>
#include <string>
#include <type_traits>

#include "core/context/render_context.hpp"
#include "core/platform/window.hpp"
#include "core/window.hpp"
#include "graphics/opengl/framebuffer.hpp"

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

TEST(WindowSurfaceTest, DestructionNotifiesEveryoneAndRethrowsFirstErrorAfterRelease)
{
	HiddenNativeWindow frame;
	spk::Window::Surface surface("surface-closing-errors");
	surface.create(frame.native.window());
	surface.makeCurrent();
	std::vector<int> events;
	auto first = surface.subscribeToDestruction([&] {
		EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Releasing);
		events.push_back(1);
	});
	auto second = surface.subscribeToDestruction([&] {
		events.push_back(2);
		throw std::runtime_error("first subscriber error");
	});
	auto third = surface.subscribeToDestruction([&] {
		events.push_back(3);
		throw std::runtime_error("second subscriber error");
	});
	try
	{
		surface.destroy();
		FAIL() << "Expected the first callback error";
	} catch (const std::runtime_error &error)
	{
		EXPECT_STREQ(error.what(), "first subscriber error");
	}
	EXPECT_EQ(events, (std::vector<int>{1, 2, 3}));
	EXPECT_EQ(surface.lifeCycle(), spk::Window::LifeCycle::Released);
	EXPECT_EQ(::wglGetCurrentContext(), nullptr);
	EXPECT_FALSE(first.isValid());
	EXPECT_FALSE(second.isValid());
	EXPECT_FALSE(third.isValid());
	EXPECT_NO_THROW(surface.destroy());
	EXPECT_EQ(events.size(), 3u);
}

TEST(WindowSurfaceTest, LateSubscriptionsAreRejectedAndCallbacksCanResign)
{
	HiddenNativeWindow frame;
	spk::Window::Surface surface("surface-closing-contracts");
	EXPECT_THROW((void)surface.subscribeToDestruction([] {}), std::logic_error);
	surface.create(frame.native.window());
	int calls = 0;
	auto inactive = surface.subscribeToDestruction([&] { ++calls; });
	inactive.resign();
	auto active = surface.subscribeToDestruction([&] {
		++calls;
		EXPECT_THROW((void)surface.subscribeToDestruction([] {}), std::logic_error);
	});
	surface.destroy();
	EXPECT_EQ(calls, 1);
	EXPECT_FALSE(active.isValid());
	EXPECT_THROW((void)surface.subscribeToDestruction([] {}), std::logic_error);
	EXPECT_NO_THROW(surface.destroy());
}

TEST(WindowSurfaceTest, IndependentSurfacesNeverShareRenderTargetPixelContent)
{
	HiddenNativeWindow firstFrame;
	HiddenNativeWindow secondFrame;
	spk::Window::Surface first("surface-independent-A");
	spk::Window::Surface second("surface-independent-B");
	first.create(firstFrame.native.window());
	second.create(secondFrame.native.window());
	spk::RenderContext firstContext{.targetSurface = &first};
	spk::RenderContext secondContext{.targetSurface = &second};
	spk::Framebuffer target({2, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	const auto clear = [&](spk::Window::Surface &surface, spk::RenderContext &context, float red) {
		surface.makeCurrent();
		target.activate(context);
		glViewport(0, 0, 2, 2);
		glClearColor(red, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	};
	clear(first, firstContext, 1.0f);
	clear(second, secondContext, 0.0f);
	std::array<unsigned char, 4> pixel{};
	first.makeCurrent();
	target.activate(firstContext);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
	EXPECT_EQ(pixel[0], 255);
	second.makeCurrent();
	target.activate(secondContext);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
	EXPECT_EQ(pixel[0], 0);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	first.makeCurrent();
	first._gpuResources().reclaimReleased();
	second.makeCurrent();
	second._gpuResources().reclaimReleased();
	first.destroy();
	second.destroy();
}
