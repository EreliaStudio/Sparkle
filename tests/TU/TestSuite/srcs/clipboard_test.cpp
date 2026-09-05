#include <gtest/gtest.h>

#include <future>
#include <optional>
#include <thread>

#include "core/platform/clipboard.hpp"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace
{
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

	std::promise<bool> openedPromise;
	auto openedFuture = openedPromise.get_future();
	std::promise<void> releasePromise;
	auto releaseFuture = releasePromise.get_future();

	std::jthread holder([&] {
		const bool opened = ::OpenClipboard(nullptr) != FALSE;
		openedPromise.set_value(opened);
		if (!opened)
			return;
		releaseFuture.wait();
		(void)::CloseClipboard();
	});

	if (!openedFuture.get())
	{
		releasePromise.set_value();
		GTEST_SKIP() << "Could not acquire the clipboard for the contention fixture";
	}

	EXPECT_NO_THROW({ EXPECT_FALSE(spk::Clipboard::hasText()); });
	EXPECT_NO_THROW({ EXPECT_FALSE(spk::Clipboard::writeText(U"contended")); });
	EXPECT_NO_THROW({ EXPECT_FALSE(spk::Clipboard::readText().has_value()); });

	releasePromise.set_value();
#endif
}
