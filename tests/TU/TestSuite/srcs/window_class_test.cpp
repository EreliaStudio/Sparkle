#include <gtest/gtest.h>

#include <Windows.h>

#include <atomic>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

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

	spk::WinAPI::Window::CreationInfo hiddenWindowInfo()
	{
		spk::WinAPI::Window::CreationInfo info;
		info.title = "WindowClassTest";
		info.width = 160;
		info.height = 90;
		info.visible = false;
		return info;
	}
}

static_assert(!std::is_copy_constructible_v<spk::WinAPI::Window::Class>);
static_assert(!std::is_move_constructible_v<spk::WinAPI::Window::Class>);
static_assert(!std::is_copy_assignable_v<spk::WinAPI::Window::Class>);
static_assert(!std::is_move_assignable_v<spk::WinAPI::Window::Class>);

TEST(WindowClassTest, StandardUsageRegistersCreatesAndReleasesClass)
{
	const std::string identifier = uniqueIdentifier("Sparkle_WindowClass_");
	{
		spk::WinAPI::Window::Class windowClass(identifier);
		spk::WinAPI::Window window;
		window.create(windowClass, hiddenWindowInfo());
		ASSERT_NE(window.handle(), nullptr);
		window.destroy();
	}

	EXPECT_NO_THROW({ spk::WinAPI::Window::Class reused(identifier); });
}

TEST(WindowClassTest, UnicodeIdentifierCanCreateWindow)
{
	const std::string identifier = uniqueIdentifier("Sparkle_Classe_") + "_\xC3\xA9\xE7\xAA\x97";
	spk::WinAPI::Window::Class windowClass(identifier);
	spk::WinAPI::Window window;
	EXPECT_NO_THROW(window.create(windowClass, hiddenWindowInfo()));
	ASSERT_NE(window.handle(), nullptr);
	window.destroy();
}

TEST(WindowClassTest, LongIdentifierCanCreateWindow)
{
	std::string identifier = uniqueIdentifier("Sparkle_LongClass_");
	identifier.append(160, 'x');

	spk::WinAPI::Window::Class windowClass(identifier);
	spk::WinAPI::Window window;
	EXPECT_NO_THROW(window.create(windowClass, hiddenWindowInfo()));
	ASSERT_NE(window.handle(), nullptr);
	window.destroy();
}

TEST(WindowClassTest, DuplicateRegistrationReportsWin32Error)
{
	const std::string identifier = uniqueIdentifier("Sparkle_Duplicate_");
	spk::WinAPI::Window::Class first(identifier);

	try
	{
		spk::WinAPI::Window::Class second(identifier);
		FAIL() << "Expected std::system_error";
	}
	catch (const std::system_error &exception)
	{
		EXPECT_EQ(exception.code().value(), ERROR_CLASS_ALREADY_EXISTS);
		EXPECT_TRUE(contains(exception.what(), "RegisterClassExW"));
	}
}

TEST(WindowClassTest, MultipleIndependentClassesCanCoexist)
{
	spk::WinAPI::Window::Class first(uniqueIdentifier("Sparkle_ClassA_"));
	spk::WinAPI::Window::Class second(uniqueIdentifier("Sparkle_ClassB_"));
	spk::WinAPI::Window firstWindow;
	spk::WinAPI::Window secondWindow;

	EXPECT_NO_THROW(firstWindow.create(first, hiddenWindowInfo()));
	EXPECT_NO_THROW(secondWindow.create(second, hiddenWindowInfo()));
	EXPECT_NE(firstWindow.handle(), secondWindow.handle());

	firstWindow.destroy();
	secondWindow.destroy();
}

// Disabled: deterministic UnregisterClassW failure requires destroying Window::Class while
// live windows still reference it or adding an injectable platform seam. The former would
// make a throwing destructor a process-termination test, which is not a documented contract.
TEST(WindowClassTest, DISABLED_UnregistrationFailureReportsCodeAndOperation)
{
	FAIL() << "Requires an injectable UnregisterClassW failure seam";
}
