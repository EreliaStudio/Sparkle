#pragma once

#include <optional>

#include "graphics/font.hpp"

namespace spk::Clipboard
{
	class Backend
	{
	public:
		virtual ~Backend() = default;

		[[nodiscard]] virtual bool hasText() const = 0;
		[[nodiscard]] virtual std::optional<Font::Text> readText() const = 0;
		virtual bool writeText(const Font::Text &text) = 0;
	};

	[[nodiscard]] Backend &systemBackend() noexcept;

	[[nodiscard]] bool hasText();
	[[nodiscard]] std::optional<Font::Text> readText();
	bool writeText(const Font::Text &text);
}
