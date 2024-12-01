#pragma once
#include <type_traits>

namespace spk
{
	template <typename T, typename = void>
	struct IsContainer : std::false_type {};

	template <typename T>
	struct IsContainer<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end()), typename T::value_type>> : std::true_type {};

	template <typename T, template <typename...> class Template>
	struct IsSpecialization : std::false_type {};

	template <template <typename...> class Template, typename... Args>
	struct IsSpecialization<Template<Args...>, Template> : std::true_type {};
}