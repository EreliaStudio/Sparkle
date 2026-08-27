#pragma once

#include <concepts>
#include <cstddef>
#include <functional>

namespace spk
{
	template <typename T>
	concept Hashable =
		std::equality_comparable<T> &&
		requires(const T &value) {
			{ std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
		};
}