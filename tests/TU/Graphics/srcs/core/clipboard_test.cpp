#include <gtest/gtest.h>

#include <optional>

#include <Windows.h>

#include "core/platform/clipboard.hpp"
#include "core/platform/clipboard_internal.hpp"

namespace
{
	class ClipboardOpenGuard
	{
	private:
		bool _open = ::OpenClipboard(nullptr) != FALSE;

	public:
		[[nodiscard]] bool isOpen() const noexcept
		{
			return _open;
		}

		[[nodiscard]] bool close() noexcept
		{
			if (!_open)
			{
				return true;
			}

			_open = false;
			return ::CloseClipboard() != FALSE;
		}

		~ClipboardOpenGuard()
		{
			(void)close();
		}
	};

	[[nodiscard]] bool clearClipboard()
	{
		ClipboardOpenGuard guard;
		return guard.isOpen() &&
			   ::EmptyClipboard() != FALSE &&
			   guard.close();
	}

	class ClipboardOpenFailure
	{
	private:
		[[nodiscard]] static bool _fail()
		{
			return false;
		}

	public:
		ClipboardOpenFailure()
		{
			spk::Clipboard::Internal::setOpenClipboardFunctionForTesting(&_fail);
		}

		ClipboardOpenFailure(const ClipboardOpenFailure &) = delete;
		ClipboardOpenFailure &operator=(const ClipboardOpenFailure &) = delete;

		~ClipboardOpenFailure()
		{
			spk::Clipboard::Internal::setOpenClipboardFunctionForTesting(nullptr);
		}
	};

	class ClipboardTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			ASSERT_TRUE(clearClipboard());
			ASSERT_EQ(::CountClipboardFormats(), 0);
		}

		void TearDown() override
		{
			EXPECT_TRUE(clearClipboard());
		}
	};
}

TEST_F(ClipboardTest, StandardUnicodeRoundTrip)
{
	const spk::Clipboard::Text text = U"Sparkle — café — 日本語 — 😀";

	ASSERT_TRUE(spk::Clipboard::writeText(text));
	EXPECT_TRUE(spk::Clipboard::hasText());

	const auto result = spk::Clipboard::readText();
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, text);
}

TEST_F(ClipboardTest, EmptyTextRoundTripsSafely)
{
	const spk::Clipboard::Text text;

	ASSERT_TRUE(spk::Clipboard::writeText(text));
	const auto result = spk::Clipboard::readText();

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, text);
}

TEST_F(ClipboardTest, MultilineAndSupplementaryCharactersRoundTrip)
{
	const spk::Clipboard::Text text = U"first line\nsecond line\r\nU+1F680: 🚀";

	ASSERT_TRUE(spk::Clipboard::writeText(text));
	const auto result = spk::Clipboard::readText();

	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, text);
}

TEST_F(ClipboardTest, WriteReplacesExistingText)
{
	ASSERT_TRUE(spk::Clipboard::writeText(U"before"));
	ASSERT_TRUE(spk::Clipboard::writeText(U"after"));

	EXPECT_EQ(
		spk::Clipboard::readText(),
		std::optional<spk::Clipboard::Text>(U"after"));
}

TEST_F(ClipboardTest, NonTextClipboardContentReportsNoText)
{
	ClipboardOpenGuard guard;
	ASSERT_TRUE(guard.isOpen());
	ASSERT_TRUE(::EmptyClipboard());

	const UINT testFormat =
		::RegisterClipboardFormatW(L"Sparkle.Test.NonText");
	ASSERT_NE(testFormat, 0u);

	HGLOBAL payload = ::GlobalAlloc(GMEM_MOVEABLE, 4);
	ASSERT_NE(payload, nullptr);
	if (::SetClipboardData(testFormat, payload) == nullptr)
	{
		::GlobalFree(payload);
		FAIL() << "SetClipboardData failed";
	}
	ASSERT_TRUE(guard.close());

	EXPECT_FALSE(spk::Clipboard::hasText());
	EXPECT_FALSE(spk::Clipboard::readText().has_value());
}

TEST_F(ClipboardTest, ClipboardOpenFailureUsesNoThrowFailureReporting)
{
	const spk::Clipboard::Text original = U"preserved while unavailable";
	ASSERT_TRUE(spk::Clipboard::writeText(original));

	{
		ClipboardOpenFailure failure;
		EXPECT_NO_THROW({ EXPECT_TRUE(spk::Clipboard::hasText()); });
		EXPECT_NO_THROW({ EXPECT_FALSE(spk::Clipboard::writeText(U"unavailable")); });
		EXPECT_NO_THROW({ EXPECT_FALSE(spk::Clipboard::readText().has_value()); });
	}

	EXPECT_EQ(
		spk::Clipboard::readText(),
		std::optional<spk::Clipboard::Text>(original));
	EXPECT_TRUE(spk::Clipboard::writeText(U"available again"));
}
