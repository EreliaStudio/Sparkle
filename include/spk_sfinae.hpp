#pragma once

#include <type_traits>

namespace spk
{
	/**
	 * @brief Trait to determine if a type is a container.
	 * 
	 * A type is considered a container if it has `begin()` and `end()` methods and a `value_type` typedef.
	 * 
	 * Example usage:
	 * @code
	 * static_assert(spk::IsContainer<std::vector<int>>::value, "std::vector should be a container");
	 * static_assert(!spk::IsContainer<int>::value, "int is not a container");
	 * @endcode
	 * 
	 * @tparam T The type to check.
	 */
	template <typename T, typename = void>
	struct IsContainer : std::false_type {};

	/**
	 * @brief Specialization for types that meet the container requirements.
	 * 
	 * This specialization checks if the type has `begin()` and `end()` methods and a `value_type` typedef.
	 */
	template <typename T>
	struct IsContainer<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end()), typename T::value_type>> : std::true_type {};

	/**
	 * @brief Trait to determine if a type is a specialization of a given template.
	 * 
	 * Example usage:
	 * @code
	 * static_assert(spk::IsSpecialization<std::vector<int>, std::vector>::value, "std::vector<int> is a specialization of std::vector");
	 * static_assert(!spk::IsSpecialization<int, std::vector>::value, "int is not a specialization of std::vector");
	 * @endcode
	 * 
	 * @tparam T The type to check.
	 * @tparam Template The template to compare against.
	 */
	template <typename T, template <typename...> class Template>
	struct IsSpecialization : std::false_type {};

	/**
	 * @brief Specialization for types that match the template.
	 */
	template <template <typename...> class Template, typename... Args>
	struct IsSpecialization<Template<Args...>, Template> : std::true_type {};
}
