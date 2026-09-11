#pragma once

#include <concepts>
#include <type_traits>

namespace spk::JSON
{
	template <typename T>
	using clean_type = std::remove_cvref_t<T>;

	template <typename T>
	concept native_integer =
		std::integral<clean_type<T>> &&
		!std::same_as<clean_type<T>, bool> &&
		!std::same_as<clean_type<T>, wchar_t> &&
		!std::same_as<clean_type<T>, char> &&
		!std::same_as<clean_type<T>, char8_t> &&
		!std::same_as<clean_type<T>, char16_t> &&
		!std::same_as<clean_type<T>, char32_t>;

	template <typename T>
	concept native_floating = std::floating_point<clean_type<T>>;

	class Value;

	template <typename T>
	concept free_json_writable =
		requires(const clean_type<T> &p_value) {
			{ toJSON(p_value) } -> std::convertible_to<Value>;
		};

	template <typename T>
	concept member_json_writable =
		requires(const clean_type<T> &p_value) {
			{ p_value.toJSON() } -> std::convertible_to<Value>;
		};

	template <typename T>
	concept json_writable =
		free_json_writable<T> ||
		member_json_writable<T>;

	template <typename T>
	concept free_json_readable =
		std::default_initializable<clean_type<T>> &&
		requires(clean_type<T> &p_value, const Value &p_object) {
			{ fromJSON(p_object, p_value) } -> std::same_as<void>;
		};

	template <typename T>
	concept member_json_readable =
		requires(const Value &p_value) {
			{ clean_type<T>::fromJSON(p_value) } -> std::same_as<clean_type<T>>;
		};

	template <typename T>
	concept json_readable =
		free_json_readable<T> ||
		member_json_readable<T>;
}