#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace spk::resources
{
	using Data = std::span<const std::byte>;

	[[nodiscard]] std::optional<Data> find(std::string_view path) noexcept;
	[[nodiscard]] Data get(std::string_view path);
	[[nodiscard]] bool contains(std::string_view path) noexcept;
	[[nodiscard]] std::string_view text(std::string_view path);
}
