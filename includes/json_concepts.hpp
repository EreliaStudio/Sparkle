#pragma once

#include <concepts>
#include <type_traits>

namespace spk::JSON
{
	template <typename T>
	concept native_integer =
		std::integral<std::remove_cvref_t<T>> &&
		!std::same_as<std::remove_cvref_t<T>, bool> &&
		!std::same_as<std::remove_cvref_t<T>, wchar_t> &&
		!std::same_as<std::remove_cvref_t<T>, char> &&
		!std::same_as<std::remove_cvref_t<T>, char8_t> &&
		!std::same_as<std::remove_cvref_t<T>, char16_t> &&
		!std::same_as<std::remove_cvref_t<T>, char32_t>;

	template <typename T>
	concept native_floating = std::floating_point<std::remove_cvref_t<T>>;

	class Value;

	template <typename T>
	concept json_writable =
		requires(const std::remove_cvref_t<T> &p_value) {
			{ toJSON(p_value) } -> std::convertible_to<Value>;
		};

	template <typename T>
	concept json_readable =
		std::default_initializable<std::remove_cvref_t<T>> &&
		requires(std::remove_cvref_t<T> &p_value, const Value &p_object) {
			{ fromJSON(p_object, p_value) } -> std::same_as<void>;
		};
}
