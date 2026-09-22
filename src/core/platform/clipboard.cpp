#include "core/platform/clipboard.hpp"

#ifdef _WIN32
#	include <Windows.h>

#	include <cstring>
#	include <vector>

#	include "core/platform/clipboard_internal.hpp"
#endif

namespace spk::Clipboard
{
#ifdef _WIN32
	namespace Internal
	{
		namespace
		{
			[[nodiscard]] bool systemOpenClipboard()
			{
				return ::OpenClipboard(nullptr) != FALSE;
			}

			OpenClipboardFunction openClipboardFunction = systemOpenClipboard;
		}

		void setOpenClipboardFunctionForTesting(OpenClipboardFunction function) noexcept
		{
			openClipboardFunction = function != nullptr ? function : systemOpenClipboard;
		}

		[[nodiscard]] bool openClipboard()
		{
			return openClipboardFunction();
		}
	}
#endif

	namespace
	{
#ifdef _WIN32
		struct ClipboardGuard
		{
			bool open = Internal::openClipboard();

			~ClipboardGuard()
			{
				if (open)
				{
					::CloseClipboard();
				}
			}
		};

		std::vector<wchar_t> toUTF16(const Text &text)
		{
			std::vector<wchar_t> result;
			result.reserve(text.size() + 1);
			for (char32_t value : text)
			{
				if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
				{
					value = 0xFFFD;
				}
				if (value <= 0xFFFF)
				{
					result.push_back(static_cast<wchar_t>(value));
				}
				else
				{
					value -= 0x10000;
					result.push_back(static_cast<wchar_t>(0xD800 + (value >> 10)));
					result.push_back(static_cast<wchar_t>(0xDC00 + (value & 0x3FF)));
				}
			}
			result.push_back(L'\0');
			return result;
		}

		Text fromUTF16(const wchar_t *text)
		{
			Text result;
			for (std::size_t index = 0; text[index] != L'\0'; ++index)
			{
				char32_t value = static_cast<char32_t>(text[index]);
				if (value >= 0xD800 && value <= 0xDBFF)
				{
					const char32_t low = static_cast<char32_t>(text[index + 1]);
					if (low >= 0xDC00 && low <= 0xDFFF)
					{
						value = 0x10000 + ((value - 0xD800) << 10) + (low - 0xDC00);
						++index;
					}
					else
					{
						value = 0xFFFD;
					}
				}
				else if (value >= 0xDC00 && value <= 0xDFFF)
				{
					value = 0xFFFD;
				}
				result.push_back(value);
			}
			return result;
		}
#endif

		class SystemBackend final : public Backend
		{
		public:
			[[nodiscard]] bool hasText() const override
			{
#ifdef _WIN32
				return IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE;
#else
				return false;
#endif
			}

			[[nodiscard]] std::optional<Text> readText() const override
			{
#ifdef _WIN32
				if (!hasText())
				{
					return std::nullopt;
				}

				ClipboardGuard guard;
				if (!guard.open)
				{
					return std::nullopt;
				}

				HANDLE handle = GetClipboardData(CF_UNICODETEXT);
				if (handle == nullptr)
				{
					return std::nullopt;
				}

				const auto *data = static_cast<const wchar_t *>(GlobalLock(handle));
				if (data == nullptr)
				{
					return std::nullopt;
				}

				Text result = fromUTF16(data);
				GlobalUnlock(handle);
				return result;
#else
				return std::nullopt;
#endif
			}

			bool writeText(const Text &text) override
			{
#ifdef _WIN32
				ClipboardGuard guard;
				if (!guard.open || EmptyClipboard() == FALSE)
				{
					return false;
				}

				const std::vector<wchar_t> encoded = toUTF16(text);
				HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, encoded.size() * sizeof(wchar_t));
				if (handle == nullptr)
				{
					return false;
				}

				void *memory = GlobalLock(handle);
				if (memory == nullptr)
				{
					GlobalFree(handle);
					return false;
				}

				std::memcpy(memory, encoded.data(), encoded.size() * sizeof(wchar_t));
				GlobalUnlock(handle);
				if (SetClipboardData(CF_UNICODETEXT, handle) == nullptr)
				{
					GlobalFree(handle);
					return false;
				}
				return true;
#else
				(void)text;
				return false;
#endif
			}
		};
	}

	Backend &systemBackend() noexcept
	{
		static SystemBackend backend;
		return backend;
	}

	bool hasText()
	{
		return systemBackend().hasText();
	}

	std::optional<Text> readText()
	{
		return systemBackend().readText();
	}

	bool writeText(const Text &text)
	{
		return systemBackend().writeText(text);
	}
}
