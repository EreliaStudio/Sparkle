#include <gtest/gtest.h>

#include "exception.hpp"

#include <exception>
#include <source_location>
#include <stdexcept>
#include <string>

namespace
{
	[[nodiscard]] std::source_location captureLocation(
		std::source_location location = std::source_location::current())
	{
		return location;
	}

	[[nodiscard]] bool contains(const std::string &text, const std::string &fragment)
	{
		return text.find(fragment) != std::string::npos;
	}
}

TEST(ExceptionTest, StandardUsagePreservesMessageLocationCauseAndContexts)
{
	const std::source_location origin = captureLocation();
	spk::Exception exception("unable to load scene", origin);

	EXPECT_EQ(exception.message(), "unable to load scene");
	EXPECT_EQ(exception.location().line(), origin.line());
	EXPECT_STREQ(exception.location().file_name(), origin.file_name());
	EXPECT_EQ(exception.cause(), nullptr);

	const std::source_location contextLocation = captureLocation();
	exception.addContext("while opening startup world", contextLocation);

	const std::string formatted = exception.what();
	EXPECT_TRUE(contains(formatted, "unable to load scene"));
	EXPECT_TRUE(contains(formatted, "while opening startup world"));
	EXPECT_TRUE(contains(formatted, origin.file_name()));
	EXPECT_TRUE(contains(formatted, contextLocation.file_name()));
}

TEST(ExceptionTest, NestedSparkleExceptionIsRetainedAsItsConcreteType)
{
	std::exception_ptr cause;
	try
	{
		throw spk::Exception("inner sparkle failure");
	}
	catch (...)
	{
		cause = std::current_exception();
	}

	spk::Exception outer("outer failure", cause);
	ASSERT_NE(outer.cause(), nullptr);

	try
	{
		std::rethrow_exception(outer.cause());
		FAIL() << "Expected nested exception";
	}
	catch (const spk::Exception &nested)
	{
		EXPECT_EQ(nested.message(), "inner sparkle failure");
	}
	catch (...)
	{
		FAIL() << "Nested spk::Exception was not preserved";
	}

	EXPECT_TRUE(contains(outer.what(), "outer failure"));
	EXPECT_TRUE(contains(outer.what(), "inner sparkle failure"));
}

TEST(ExceptionTest, NestedStandardExceptionIsRetainedWithoutSlicing)
{
	std::exception_ptr cause;
	try
	{
		throw std::runtime_error("standard failure");
	}
	catch (...)
	{
		cause = std::current_exception();
	}

	spk::Exception outer("outer failure", cause);
	ASSERT_NE(outer.cause(), nullptr);
	EXPECT_THROW(
		{
			try
			{
				std::rethrow_exception(outer.cause());
			}
			catch (const std::runtime_error &nested)
			{
				EXPECT_STREQ(nested.what(), "standard failure");
				throw;
			}
		},
		std::runtime_error);
	EXPECT_TRUE(contains(outer.what(), "standard failure"));
}

TEST(ExceptionTest, MultipleContextsRemainOrderedAndWhatIsStable)
{
	spk::Exception exception("root");
	exception.addContext("first context");
	exception.addContext("second context");
	exception.addContext("third context");

	const std::string first = exception.what();
	const std::string second = exception.what();
	EXPECT_EQ(first, second);

	const auto firstPosition = first.find("first context");
	const auto secondPosition = first.find("second context");
	const auto thirdPosition = first.find("third context");
	ASSERT_NE(firstPosition, std::string::npos);
	ASSERT_NE(secondPosition, std::string::npos);
	ASSERT_NE(thirdPosition, std::string::npos);
	EXPECT_LT(firstPosition, secondPosition);
	EXPECT_LT(secondPosition, thirdPosition);
}

TEST(ExceptionTest, EmptyMessageAndEmptyCauseRemainInspectable)
{
	spk::Exception withoutCause("");
	EXPECT_TRUE(withoutCause.message().empty());
	EXPECT_EQ(withoutCause.cause(), nullptr);
	EXPECT_NO_THROW((void)withoutCause.what());

	spk::Exception withExplicitEmptyCause("", std::exception_ptr{});
	EXPECT_TRUE(withExplicitEmptyCause.message().empty());
	EXPECT_EQ(withExplicitEmptyCause.cause(), nullptr);
	EXPECT_NO_THROW((void)withExplicitEmptyCause.what());
}

TEST(ExceptionTest, DISABLED_ExactFormattingContractRequiresImplementationSnapshot)
{
	GTEST_SKIP() << "The public header exposes formatted what(), but the supplied snapshot does not define the exact frame/indentation syntax.";
}
