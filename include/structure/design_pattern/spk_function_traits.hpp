#pragma once

#include <tuple>
#include <type_traits>

namespace spk
{
	template <typename T>
	struct FunctionTraits : FunctionTraits<decltype(&T::operator())>
	{
	};

	template <typename C, typename R, typename... Args>
	struct FunctionTraits<R (C::*)(Args...) const>
	{
		using ArgsTuple = std::tuple<Args...>;
	};

	template <typename C, typename R, typename... Args>
	struct FunctionTraits<R (C::*)(Args...)>
	{
		using ArgsTuple = std::tuple<Args...>;
	};

	template <typename R, typename... Args>
	struct FunctionTraits<R (*)(Args...)>
	{
		using ArgsTuple = std::tuple<Args...>;
	};

	template <typename R, typename... Args>
	struct FunctionTraits<R(Args...)>
	{
		using ArgsTuple = std::tuple<Args...>;
	};

	template <typename C, typename R, typename... Args>
	struct FunctionTraits<R (C::*)(Args...) const noexcept> : FunctionTraits<R (C::*)(Args...) const>
	{
	};

	template <typename C, typename R, typename... Args>
	struct FunctionTraits<R (C::*)(Args...) noexcept> : FunctionTraits<R (C::*)(Args...)>
	{
	};
}