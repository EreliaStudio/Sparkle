#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

#include "core/platform/clipboard.hpp"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace
{
#if defined(_WIN32)
	class ClipboardHolder
	{
		HANDLE _acquired = nullptr;
		HANDLE _release = nullptr;
		HANDLE _process = nullptr;

		void cleanup() noexcept
		{
			if (_release != nullptr) ::SetEvent(_release);
			if (_process != nullptr)
			{
				if (::WaitForSingleObject(_process, 5000) != WAIT_OBJECT_0)
				{
					::TerminateProcess(_process, 4);
					::WaitForSingleObject(_process, 5000);
				}
				::CloseHandle(_process);
			}
			if (_release != nullptr) ::CloseHandle(_release);
			if (_acquired != nullptr) ::CloseHandle(_acquired);
		}

	public:
		ClipboardHolder()
		{
			try
			{
				const std::wstring prefix = L"Local\\SparkleClipboard_" + std::to_wstring(::GetCurrentProcessId()) +
					L"_" + std::to_wstring(::GetTickCount64());
				const std::wstring acquiredName = prefix + L"_acquired";
				const std::wstring releaseName = prefix + L"_release";
				_acquired = ::CreateEventW(nullptr, TRUE, FALSE, acquiredName.c_str());
				_release = ::CreateEventW(nullptr, TRUE, FALSE, releaseName.c_str());
				if (_acquired == nullptr || _release == nullptr)
					throw std::runtime_error("Unable to create clipboard helper events");
				std::wstring executable(32768, L'\0');
				const DWORD length = ::GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
				if (length == 0 || length >= executable.size())
					throw std::runtime_error("Unable to locate clipboard helper");
				executable.resize(length);
				const auto helper = std::filesystem::path(executable).parent_path() / L"SparkleClipboardHolder.exe";
				std::wstring command = L"\"" + helper.wstring() + L"\" " + acquiredName + L" " + releaseName;
				STARTUPINFOW startup{};
				startup.cb = sizeof(startup);
				PROCESS_INFORMATION process{};
				if (!::CreateProcessW(helper.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
					nullptr, nullptr, &startup, &process))
					throw std::runtime_error("Unable to start clipboard helper");
				_process = process.hProcess;
				::CloseHandle(process.hThread);
				const HANDLE waits[] = {_acquired, _process};
				if (::WaitForMultipleObjects(2, waits, FALSE, 5000) != WAIT_OBJECT_0)
					throw std::runtime_error("Clipboard helper did not acquire the clipboard");
			} catch (...)
			{
				cleanup();
				throw;
			}
		}
		ClipboardHolder(const ClipboardHolder &) = delete;
		ClipboardHolder &operator=(const ClipboardHolder &) = delete;
		~ClipboardHolder() { cleanup(); }

		bool release()
		{
			::SetEvent(_release);
			DWORD result = 1;
			return ::WaitForSingleObject(_process, 5000) == WAIT_OBJECT_0 &&
				::GetExitCodeProcess(_process, &result) && result == 0;
		}
	};
#endif

	class ClipboardRestoreGuard
	{
	private:
		std::optional<spk::Font::Text> _previousText;
		bool _hadText = false;
		bool _wasEmpty = false;

	public:
		ClipboardRestoreGuard() :
			_previousText(spk::Clipboard::readText()),
			_hadText(spk::Clipboard::hasText())
		{
#if defined(_WIN32)
			_wasEmpty = ::CountClipboardFormats() == 0;
#endif
		}

		[[nodiscard]] bool canRestore() const noexcept
		{
			return _hadText || _wasEmpty;
		}

		~ClipboardRestoreGuard()
		{
#if defined(_WIN32)
			if (_hadText && _previousText.has_value())
			{
				(void)spk::Clipboard::writeText(*_previousText);
				return;
			}
			if (_wasEmpty && ::OpenClipboard(nullptr))
			{
				(void)::EmptyClipboard();
				(void)::CloseClipboard();
			}
#endif
		}
	};
}

TEST(ClipboardTest, StandardUnicodeRoundTripPreservesExistingText)
{
#if !defined(_WIN32)
	GTEST_SKIP() << "The current Clipboard implementation is Win32-backed.";
#else
	ClipboardRestoreGuard restore;
	if (!restore.canRestore())
		GTEST_SKIP() << "Existing non-text clipboard data cannot be restored losslessly by this public test helper";
	const spk::Font::Text text = U"Sparkle — café — 日本語 — 😀";

	ASSERT_TRUE(spk::Clipboard::writeText(text));
	EXPECT_TRUE(spk::Clipboard::hasText());

	const auto result = spk::Clipboard::readText();
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, text);
#endif
}

TEST(ClipboardTest, EmptyTextRoundTripsSafely)
{
#if !defined(_WIN32)
	GTEST_SKIP() << "The current Clipboard implementation is Win32-backed.";
#else
	ClipboardRestoreGuard restore;
	if (!restore.canRestore())
		GTEST_SKIP() << "Existing non-text clipboard data cannot be restored losslessly by this public test helper";
	const spk::Font::Text text;

	ASSERT_TRUE(spk::Clipboard::writeText(text));
	const auto result = spk::Clipboard::readText();
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, text);
#endif
}

TEST(ClipboardTest, MultilineAndSupplementaryCharactersRoundTrip)
{
#if !defined(_WIN32)
	GTEST_SKIP() << "The current Clipboard implementation is Win32-backed.";
#else
	ClipboardRestoreGuard restore;
	if (!restore.canRestore())
		GTEST_SKIP() << "Existing non-text clipboard data cannot be restored losslessly by this public test helper";
	const spk::Font::Text text = U"first line\nsecond line\r\nU+1F680: 🚀";

	ASSERT_TRUE(spk::Clipboard::writeText(text));
	const auto result = spk::Clipboard::readText();
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, text);
#endif
}

TEST(ClipboardTest, NonTextClipboardContentReportsNoText)
{
#if !defined(_WIN32)
	GTEST_SKIP() << "The current Clipboard implementation is Win32-backed.";
#else
	ClipboardRestoreGuard restore;
	if (!restore.canRestore())
		GTEST_SKIP() << "Existing non-text clipboard data cannot be restored losslessly by this public test helper";
	ASSERT_TRUE(::OpenClipboard(nullptr));
	ASSERT_TRUE(::EmptyClipboard());

	const UINT testFormat = ::RegisterClipboardFormatW(L"Sparkle.Test.NonText");
	ASSERT_NE(testFormat, 0u);
	HGLOBAL payload = ::GlobalAlloc(GMEM_MOVEABLE, 4);
	ASSERT_NE(payload, nullptr);
	if (::SetClipboardData(testFormat, payload) == nullptr)
	{
		::GlobalFree(payload);
		::CloseClipboard();
		FAIL() << "SetClipboardData failed";
	}
	ASSERT_TRUE(::CloseClipboard());

	EXPECT_FALSE(spk::Clipboard::hasText());
	EXPECT_FALSE(spk::Clipboard::readText().has_value());
#endif
}

TEST(ClipboardTest, ClipboardOpenContentionUsesNoThrowFailureReporting)
{
#if !defined(_WIN32)
	GTEST_SKIP() << "The current Clipboard implementation is Win32-backed.";
#else
	ClipboardRestoreGuard restore;
	if (!restore.canRestore())
		GTEST_SKIP() << "Existing non-text clipboard data cannot be restored losslessly by this public test helper";

	const spk::Font::Text original = U"preserved while contended";
	ASSERT_TRUE(spk::Clipboard::writeText(original));
	ClipboardHolder holder;
	const bool opened = ::OpenClipboard(nullptr) != FALSE;
	if (opened) ::CloseClipboard();
	ASSERT_FALSE(opened) << "The helper must block an independent clipboard open";

	EXPECT_NO_THROW({ EXPECT_TRUE(spk::Clipboard::hasText()); });
	EXPECT_NO_THROW({ EXPECT_FALSE(spk::Clipboard::writeText(U"contended")); });
	EXPECT_NO_THROW({ EXPECT_FALSE(spk::Clipboard::readText().has_value()); });

	ASSERT_TRUE(holder.release());
	EXPECT_EQ(spk::Clipboard::readText(), std::optional<spk::Font::Text>(original));
	EXPECT_TRUE(spk::Clipboard::writeText(U"available again"));
#endif
}
