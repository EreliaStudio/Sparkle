#pragma once

#include <optional>

#include "graphics/font.hpp"

namespace spk::Clipboard
{
	[[nodiscard]] bool hasText();
	[[nodiscard]] std::optional<Font::Text> readText();
	bool writeText(const Font::Text &text);
}
