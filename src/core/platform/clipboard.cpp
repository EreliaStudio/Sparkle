#include "core/platform/clipboard.hpp"

#ifdef _WIN32
#	include <Windows.h>

#	include <cstring>
#	include <vector>
#endif

namespace spk::Clipboard
{
#ifdef _WIN32
	namespace
	{
		struct ClipboardGuard
		{
			bool open = OpenClipboard(nullptr) != FALSE;
			~ClipboardGuard()
			{
				if (open)
				{
					CloseClipboard();
				}
			}
		};

		std::vector<wchar_t> toUTF16(const Font::Text &text)
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

		Font::Text fromUTF16(const wchar_t *text)
		{
			Font::Text result;
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
	}

	bool hasText()
	{
		return IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE;
	}
	std::optional<Font::Text> readText()
	{
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
		Font::Text result = fromUTF16(data);
		GlobalUnlock(handle);
		return result;
	}
	bool writeText(const Font::Text &text)
	{
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
	}
#else
	bool hasText()
	{
		return false;
	}
	std::optional<Font::Text> readText()
	{
		return std::nullopt;
	}
	bool writeText(const Font::Text &)
	{
		return false;
	}
#endif
}
