#include <gtest/gtest.h>

#include <Windows.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>

#include "core/application.hpp"
#include "core/context/render_context.hpp"
#include "core/context/update_context.hpp"
#include "core/platform/detail/window_surface_driver.hpp"
#include "exception.hpp"
#include "rendering/render_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/scoped_override.hpp"
#include "ui/widget.hpp"

static_assert(!std::is_copy_constructible_v<spk::Application>);
static_assert(!std::is_copy_assignable_v<spk::Application>);
static_assert(!std::is_move_constructible_v<spk::Application>);
static_assert(!std::is_move_assignable_v<spk::Application>);

namespace
{
	class CallbackRenderCommand final : public spk::RenderCommand
	{
	private:
		std::function<void()> _callback;

	public:
		explicit CallbackRenderCommand(std::function<void()> callback) :
			_callback(std::move(callback))
		{
		}

		void execute(spk::RenderContext &) const override
		{
			_callback();
		}
	};

	class ApplicationProbeWidget : public spk::Widget
	{
	private:
		std::function<void()> _onRender;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updateCalls;
		}

		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			++snapshotCalls;
			builder.renderPass(targetRenderPass()).emplace<CallbackRenderCommand>([this] {
				++renderCalls;
				_onRender();
			});
		}

		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++keyCalls;
		}

	public:
		std::atomic_size_t updateCalls = 0;
		std::atomic_size_t snapshotCalls = 0;
		std::atomic_size_t renderCalls = 0;
		std::atomic_size_t keyCalls = 0;

		ApplicationProbeWidget(std::string name, spk::Widget *parent, std::function<void()> onRender = {}) :
			spk::Widget(std::move(name), parent),
			_onRender(std::move(onRender))
		{
			activate();
		}
	};

	class ThrowingUpdateWidget final : public spk::Widget
	{
	protected:
		void _updateState(spk::UpdateContext &) override
		{
			throw std::runtime_error("injected update failure");
		}

	public:
		ThrowingUpdateWidget(std::string name, spk::Widget *parent) :
			spk::Widget(std::move(name), parent)
		{
			activate();
		}
	};

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

TEST(ApplicationTest, StandardReadyWindowInitializationUpdateRenderAndClose)
{
	spk::Application application;
	spk::Window &window = application.createWindow("standard-runtime", offscreenConfiguration("standard-runtime"));
	std::atomic_bool closureRequested = false;
	ApplicationProbeWidget probe("probe", &window.root(), [&] {
		if (!closureRequested.exchange(true))
		{
			application.closeWindow("standard-runtime");
		}
	});

	EXPECT_EQ(application.run(), EXIT_SUCCESS);
	EXPECT_GT(probe.updateCalls.load(), 0u);
	EXPECT_GT(probe.snapshotCalls.load(), 0u);
	EXPECT_GT(probe.renderCalls.load(), 0u);
	EXPECT_THROW((void)application.window("standard-runtime"), std::out_of_range);
}

TEST(ApplicationTest, RepeatedCloseRequestsAreIdempotentlyCoordinated)
{
	spk::Application application;
	spk::Window &window = application.createWindow("repeated-close", offscreenConfiguration("repeated-close"));
	std::atomic_bool closureRequested = false;
	ApplicationProbeWidget probe("probe", &window.root(), [&] {
		if (!closureRequested.exchange(true))
		{
			application.closeWindow("repeated-close");
			application.closeWindow("repeated-close");
		}
	});

	EXPECT_EQ(application.run(), EXIT_SUCCESS);
	EXPECT_TRUE(closureRequested.load());
	EXPECT_THROW((void)application.window("repeated-close"), std::out_of_range);
}

TEST(ApplicationTest, QuitWhileRunIsActiveStopsAndJoinsAllRuntimes)
{
	spk::Application application;
	spk::Window &window = application.createWindow("active-quit", offscreenConfiguration("active-quit"));
	std::atomic_bool quitRequested = false;
	ApplicationProbeWidget probe("probe", &window.root(), [&] {
		if (!quitRequested.exchange(true))
		{
			application.quit(19);
			application.closeWindow("active-quit");
		}
	});

	EXPECT_EQ(application.run(), 19);
	EXPECT_TRUE(quitRequested.load());
}

TEST(ApplicationTest, EventRoutingTargetsOnlyTheMatchingWindow)
{
	spk::Application application;
	spk::Window &firstWindow = application.createWindow("routing-first", offscreenConfiguration("Sparkle routing first"));
	spk::Window &secondWindow = application.createWindow("routing-second", offscreenConfiguration("Sparkle routing second"));
	ApplicationProbeWidget first("first", &firstWindow.root());
	ApplicationProbeWidget second("second", &secondWindow.root());
	std::atomic_bool helperSucceeded = false;
	std::jthread helper([&] {
		HWND firstHandle = nullptr;
		HWND secondHandle = nullptr;
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (std::chrono::steady_clock::now() < deadline && (firstHandle == nullptr || secondHandle == nullptr))
		{
			firstHandle = ::FindWindowA(nullptr, "Sparkle routing first");
			secondHandle = ::FindWindowA(nullptr, "Sparkle routing second");
			std::this_thread::yield();
		}
		if (firstHandle == nullptr || secondHandle == nullptr)
		{
			application.quit(EXIT_FAILURE);
			return;
		}
		::PostMessageW(firstHandle, WM_KEYDOWN, 'A', 0);
		while (std::chrono::steady_clock::now() < deadline && first.keyCalls.load() == 0)
		{
			std::this_thread::yield();
		}
		helperSucceeded.store(first.keyCalls.load() != 0 && second.keyCalls.load() == 0);
		::PostMessageW(firstHandle, WM_CLOSE, 0, 0);
		::PostMessageW(secondHandle, WM_CLOSE, 0, 0);
	});

	EXPECT_EQ(application.run(), EXIT_SUCCESS);
	helper.join();
	EXPECT_TRUE(helperSucceeded.load());
	EXPECT_EQ(second.keyCalls.load(), 0u);
}

TEST(ApplicationTest, WorkerExceptionsCrossTheRunBoundary)
{
	spk::Application application;
	spk::Window &window = application.createWindow("worker-failure", offscreenConfiguration("worker-failure"));
	ThrowingUpdateWidget throwing("throwing", &window.root());

	EXPECT_THROW((void)application.run(), spk::Exception);
	EXPECT_THROW((void)application.window("worker-failure"), std::out_of_range);
}

TEST(ApplicationTest, RuntimeFailuresReceiveApplicationContext)
{
	spk::Application application;
	spk::Window &window = application.createWindow("context-failure", offscreenConfiguration("context-failure"));
	ThrowingUpdateWidget throwing("throwing", &window.root());

	try
	{
		(void)application.run();
		FAIL() << "Expected worker failure";
	} catch (const spk::Exception &exception)
	{
		const std::string message = exception.what();
		EXPECT_NE(message.find("update runtime"), std::string::npos);
		EXPECT_NE(message.find("context-failure"), std::string::npos);
		EXPECT_NE(message.find("injected update failure"), std::string::npos);
	}
}

TEST(ApplicationTest, NativeFailureDuringSurfaceCreationIsReported)
{
	spk::Application application;
	application.createWindow("native-surface-failure", offscreenConfiguration("native-surface-failure"));
	auto &operation = spk::detail::windowSurfaceDriver().getDeviceContext;
	auto override = sparkle_test::scopedOverride(operation, [](HWND) -> HDC {
		::SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return nullptr;
	});

	try
	{
		(void)application.run();
		FAIL() << "Expected surface creation failure";
	} catch (const spk::Exception &exception)
	{
		const std::string message = exception.what();
		EXPECT_NE(message.find("render runtime"), std::string::npos);
		EXPECT_NE(message.find("GetDC"), std::string::npos);
	}
}
