#include <gtest/gtest.h>

#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/platform/window.hpp"

namespace
{
	std::string uniqueIdentifier(std::string_view prefix)
	{
		static std::atomic_uint64_t counter = 0;
		return std::string(prefix) + std::to_string(::GetCurrentProcessId()) + "_" +
			std::to_string(::GetTickCount64()) + "_" + std::to_string(counter.fetch_add(1));
	}

	bool contains(std::string_view text, std::string_view fragment)
	{
		return text.find(fragment) != std::string_view::npos;
	}

	spk::WinAPI::Window::CreationInfo info(std::string title = "Sparkle window")
	{
		spk::WinAPI::Window::CreationInfo result;
		result.title = std::move(title);
		result.x = 40;
		result.y = 50;
		result.width = 320;
		result.height = 180;
		result.visible = false;
		return result;
	}
}

TEST(WindowTest, StandardUsage)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_Window_"));
	spk::WinAPI::Window window;
	int callbackCount = 0;
	auto creation = info();
	creation.messageHandler = [&](HWND handle, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
		if (message == WM_APP + 10)
		{
			++callbackCount;
			EXPECT_EQ(handle, window.handle());
			EXPECT_EQ(wParam, 123u);
			EXPECT_EQ(lParam, 456);
			return 789;
		}
		return ::DefWindowProcW(handle, message, wParam, lParam);
	};

	window.create(windowClass, creation);
	ASSERT_NE(window.handle(), nullptr);
	EXPECT_FALSE(::IsWindowVisible(window.handle()));

	const spk::Rect2D geometry = window.geometry();
	EXPECT_EQ(geometry.size.x, creation.width);
	EXPECT_EQ(geometry.size.y, creation.height);
	EXPECT_EQ(::SendMessageW(window.handle(), WM_APP + 10, 123, 456), 789);
	EXPECT_EQ(callbackCount, 1);

	EXPECT_FALSE(window.consumeClosureRequest());
	::SendMessageW(window.handle(), WM_CLOSE, 0, 0);
	EXPECT_TRUE(window.consumeClosureRequest());
	EXPECT_FALSE(window.consumeClosureRequest());
	EXPECT_NE(window.handle(), nullptr);

	window.destroy();
	EXPECT_EQ(window.handle(), nullptr);

	window.create(windowClass, creation);
	EXPECT_NE(window.handle(), nullptr);
	window.destroy();
}

TEST(WindowTest, VisibleAndHiddenCreationRespectVisibilityFlag)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_Visibility_"));
	spk::WinAPI::Window window;
	auto hidden = info();
	hidden.visible = false;
	window.create(windowClass, hidden);
	EXPECT_FALSE(::IsWindowVisible(window.handle()));
	window.destroy();

	auto visible = info();
	visible.visible = true;
	window.create(windowClass, visible);
	EXPECT_TRUE(::IsWindowVisible(window.handle()));
	window.destroy();
}

TEST(WindowTest, ZeroClientDimensionsAreSupported)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_ZeroSize_"));
	spk::WinAPI::Window window;
	auto creation = info();
	creation.width = 0;
	creation.height = 0;
	window.create(windowClass, creation);
	const spk::Rect2D geometry = window.geometry();
	EXPECT_EQ(geometry.size.x, 0u);
	EXPECT_EQ(geometry.size.y, 0u);
	window.destroy();
}

TEST(WindowTest, MinimumNonZeroClientDimensionsAreSupported)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_MinSize_"));
	spk::WinAPI::Window window;
	auto creation = info();
	creation.width = 1;
	creation.height = 1;
	window.create(windowClass, creation);
	const spk::Rect2D geometry = window.geometry();
	EXPECT_EQ(geometry.size.x, 1u);
	EXPECT_EQ(geometry.size.y, 1u);
	window.destroy();
}

TEST(WindowTest, NegativePositionIsAccepted)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_NegativePosition_"));
	spk::WinAPI::Window window;
	auto creation = info();
	creation.x = -400;
	creation.y = -300;
	EXPECT_NO_THROW(window.create(windowClass, creation));
	RECT rectangle{};
	ASSERT_TRUE(::GetWindowRect(window.handle(), &rectangle));
	EXPECT_LT(rectangle.left, 0);
	EXPECT_LT(rectangle.top, 0);
	window.destroy();
}

TEST(WindowTest, UnicodeTitleIsConvertedToWideText)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_UnicodeTitle_"));
	spk::WinAPI::Window window;
	auto creation = info("Fen\xC3\xAAtre \xE2\x98\x85");
	window.create(windowClass, creation);

	wchar_t buffer[128]{};
	const int length = ::GetWindowTextW(window.handle(), buffer, static_cast<int>(std::size(buffer)));
	ASSERT_GT(length, 0);
	EXPECT_EQ(std::wstring(buffer, static_cast<std::size_t>(length)), L"Fen\u00EAtre \u2605");
	window.destroy();
}

TEST(WindowTest, CallbackExceptionIsCapturedAndRethrown)
{
	// Isolate the native callback boundary: its current implementation lets the
	// exception terminate the process instead of storing it for later rethrow.
	EXPECT_EXIT(
		{
			spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_CallbackException_"));
			spk::WinAPI::Window window;
			auto creation = info();
			creation.messageHandler = [](HWND handle, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
				if (message == WM_APP + 11)
				{
					throw std::runtime_error("callback exploded");
				}
				return ::DefWindowProcW(handle, message, wParam, lParam);
			};
			window.create(windowClass, creation);
			(void)::SendMessageW(window.handle(), WM_APP + 11, 0, 0);

			try
			{
				window.rethrowPendingException();
			}
			catch (const std::runtime_error &exception)
			{
				std::exit(std::string_view(exception.what()) == "callback exploded" ? 0 : 2);
			}
			catch (...)
			{
				std::exit(2);
			}
			std::exit(1);
		},
		::testing::ExitedWithCode(0), "");
}

TEST(WindowTest, DestroyIsIdempotent)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_IdempotentDestroy_"));
	spk::WinAPI::Window window;
	window.create(windowClass, info());
	EXPECT_NO_THROW(window.destroy());
	EXPECT_EQ(window.handle(), nullptr);
	EXPECT_NO_THROW(window.destroy());
	EXPECT_EQ(window.handle(), nullptr);
}

TEST(WindowTest, DestructionMessagesReachCallback)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_DestroyMessages_"));
	spk::WinAPI::Window window;
	std::vector<UINT> messages;
	auto creation = info();
	creation.messageHandler = [&](HWND handle, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
		if (message == WM_DESTROY || message == WM_NCDESTROY)
		{
			messages.push_back(message);
			return 0;
		}
		return ::DefWindowProcW(handle, message, wParam, lParam);
	};
	window.create(windowClass, creation);
	window.destroy();

	EXPECT_NE(std::find(messages.begin(), messages.end(), WM_DESTROY), messages.end());
	EXPECT_NE(std::find(messages.begin(), messages.end(), WM_NCDESTROY), messages.end());
}

TEST(WindowTest, ClassOutlivesNativeWindowHandle)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_ClassLifetime_"));
	{
		spk::WinAPI::Window window;
		window.create(windowClass, info());
		ASSERT_NE(window.handle(), nullptr);
		window.destroy();
	}

	spk::WinAPI::Window second;
	EXPECT_NO_THROW(second.create(windowClass, info()));
	second.destroy();
}

TEST(WindowTest, CreatingAlreadyCreatedWindowThrowsLogicError)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_DoubleCreate_"));
	spk::WinAPI::Window window;
	window.create(windowClass, info());
	EXPECT_THROW(window.create(windowClass, info()), std::logic_error);
	window.destroy();
}

TEST(WindowTest, GeometryBeforeCreationThrowsLogicError)
{
	spk::WinAPI::Window window;
	EXPECT_THROW((void)window.geometry(), std::logic_error);
}

TEST(WindowTest, InvalidUtf8TitleReportsConversionSystemError)
{
	spk::WinAPI::Window::Class windowClass(uniqueIdentifier("Sparkle_InvalidUtf8_"));
	spk::WinAPI::Window window;
	std::string invalidTitle = "invalid ";
	invalidTitle.push_back(static_cast<char>(0xC3));
	invalidTitle.push_back('(');
	auto creation = info(std::move(invalidTitle));

	try
	{
		window.create(windowClass, creation);
		FAIL() << "Expected std::system_error";
	}
	catch (const std::system_error &exception)
	{
		EXPECT_EQ(exception.code().value(), ERROR_NO_UNICODE_TRANSLATION);
		EXPECT_TRUE(contains(exception.what(), "MultiByteToWideChar"));
	}
}
