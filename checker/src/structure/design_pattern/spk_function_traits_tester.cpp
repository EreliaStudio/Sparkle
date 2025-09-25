#include "structure/design_pattern/spk_function_traits_tester.hpp"

#include <functional>

namespace
{
	int FreeFunction(double, int)
	{
		return 0;
	}

	struct MethodHolder
	{
		void member(int, float)
		{
		}
		int constMember(double) const
		{
			return 0;
		}
		void noexceptMember(int) noexcept
		{
		}
	};
}

TEST(FunctionTraitsTest, LambdaArgsDetected)
{
	auto lambda = [](int, float) {};
	using Traits = spk::FunctionTraits<decltype(lambda)>;
	using Expected = std::tuple<int, float>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, FunctorArgsDetected)
{
	using Traits = spk::FunctionTraits<TraitsTestFunctor>;
	using Expected = std::tuple<double, float>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, MutableFunctorArgsDetected)
{
	using Traits = spk::FunctionTraits<TraitsMutableFunctor>;
	using Expected = std::tuple<int>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, NoexceptFunctorArgsDetected)
{
	using Traits = spk::FunctionTraits<TraitsNoexceptFunctor>;
	using Expected = std::tuple<int, const std::string &>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, FreeFunctionArgsDetected)
{
	using Traits = spk::FunctionTraits<decltype(&FreeFunction)>;
	using Expected = std::tuple<double, int>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, FunctionPointerArgsDetected)
{
	using Traits = spk::FunctionTraits<int (*)(double, int)>;
	using Expected = std::tuple<double, int>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, MethodPointerArgsDetected)
{
	using Traits = spk::FunctionTraits<decltype(&MethodHolder::member)>;
	using Expected = std::tuple<int, float>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, ConstMethodPointerArgsDetected)
{
	using Traits = spk::FunctionTraits<decltype(&MethodHolder::constMember)>;
	using Expected = std::tuple<double>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, NoexceptMethodPointerArgsDetected)
{
	using Traits = spk::FunctionTraits<decltype(&MethodHolder::noexceptMember)>;
	using Expected = std::tuple<int>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}

TEST(FunctionTraitsTest, StdFunctionArgsDetected)
{
	std::function<void(int, const std::string &)> func = [](int, const std::string &) {};
	using Traits = spk::FunctionTraits<decltype(func)>;
	using Expected = std::tuple<int, const std::string &>;
	ASSERT_EQ((std::is_same<typename Traits::ArgsTuple, Expected>::value), true);
}
