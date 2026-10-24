#pragma once

#include <optional>
#include <string>

namespace spk::Clipboard
{
	using Text = std::u32string;

	class Backend
	{
	public:
		virtual ~Backend() = default;

		[[nodiscard]] virtual bool hasText() const = 0;
		[[nodiscard]] virtual std::optional<Text> readText() const = 0;
		virtual bool writeText(const Text &text) = 0;
	};

	[[nodiscard]] Backend &systemBackend() noexcept;

	[[nodiscard]] bool hasText();
	[[nodiscard]] std::optional<Text> readText();
	bool writeText(const Text &text);
}
