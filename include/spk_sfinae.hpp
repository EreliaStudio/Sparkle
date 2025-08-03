#pragma once
#include <type_traits>

namespace spk
{
	namespace JSON
	{
		class Object;
	}
}

namespace spk
{
	template <typename T, typename = void>
	struct IsContainer : std::false_type
	{
	};

	template <typename T>
	struct IsContainer<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end()), typename T::value_type>>
		: std::true_type
	{
	};

	template <typename T, template <typename...> class Template>
	struct IsSpecialization : std::false_type
	{
	};

	template <template <typename...> class Template, typename... Args>
	struct IsSpecialization<Template<Args...>, Template> : std::true_type
	{
	};

	template <typename, typename = void>
	struct ContainsToJSON : std::false_type
	{
	};

	template <typename T>
	struct ContainsToJSON<T, std::void_t<decltype(std::declval<const T &>().toJSON())>> : std::true_type
	{
	};

	template <typename T, typename = void>
	struct ContainsFromJSON : std::false_type
	{
	};

	template <typename T>
	struct ContainsFromJSON<T, std::void_t<decltype(std::declval<T>().fromJSON(std::declval<decltype(std::declval<T>().toJSON())>()))>>
		: std::true_type
	{
	};

	template <typename T, typename = void>
	struct IsJSONable : std::false_type
	{
	};

	template <typename T>
	struct IsJSONable<T, std::enable_if_t<ContainsToJSON<T>::value && ContainsFromJSON<T>::value>> : std::true_type
	{
	};

	template <typename, typename = void>
	struct ContainsToOBJ : std::false_type
	{
	};

	template <typename T>
	struct ContainsToOBJ<T, std::void_t<decltype(std::declval<const T &>().toOBJ())>> : std::true_type
	{
	};

	template <typename, typename = void>
	struct ContainsFromOBJ : std::false_type
	{
	};

	template <typename T>
	struct ContainsFromOBJ<T, std::void_t<decltype(std::declval<T>().fromOBJ(std::declval<decltype(std::declval<T>().toOBJ())>()))>> : std::true_type
	{
	};

	template <typename T, typename = void>
	struct IsOBJable : std::false_type
	{
	};

	template <typename T>
	struct IsOBJable<T, std::enable_if_t<ContainsToOBJ<T>::value && ContainsFromOBJ<T>::value>> : std::true_type
	{
	};
}