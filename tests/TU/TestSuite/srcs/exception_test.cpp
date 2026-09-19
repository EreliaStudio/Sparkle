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

	[[nodiscard]] std::string frame(
		std::size_t indentation,
		const std::string &message,
		const std::source_location &location)
	{
		const std::string prefix(indentation, '\t');
		return prefix + location.file_name() + ":" + std::to_string(location.line()) +
			"\n" + prefix + "\t" + message;
	}
}

TEST(ExceptionTest, StandardUsagePreservesMessageLocationAndCause)
{
	const std::source_location origin = captureLocation();
	const spk::Exception cause("unable to load scene", origin);
	const std::source_location contextLocation = captureLocation();
	const spk::Exception exception(
		"while opening startup world",
		std::make_exception_ptr(cause),
		contextLocation);

	EXPECT_EQ(exception.message(), "while opening startup world");
	EXPECT_EQ(exception.location().line(), contextLocation.line());
	EXPECT_STREQ(exception.location().file_name(), contextLocation.file_name());
	EXPECT_NE(exception.cause(), nullptr);

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
	const spk::Exception root("root");
	const spk::Exception firstContext("first context", std::make_exception_ptr(root));
	const spk::Exception secondContext("second context", std::make_exception_ptr(firstContext));
	const spk::Exception exception("third context", std::make_exception_ptr(secondContext));

	const std::string first = exception.what();
	const std::string second = exception.what();
	EXPECT_EQ(first, second);

	const auto firstPosition = first.find("first context");
	const auto secondPosition = first.find("second context");
	const auto thirdPosition = first.find("third context");
	ASSERT_NE(firstPosition, std::string::npos);
	ASSERT_NE(secondPosition, std::string::npos);
	ASSERT_NE(thirdPosition, std::string::npos);
	EXPECT_LT(thirdPosition, secondPosition);
	EXPECT_LT(secondPosition, firstPosition);
	EXPECT_LT(firstPosition, first.find("root"));
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

TEST(ExceptionTest, ExactFormattingContract)
{
	const auto innerLocation = captureLocation();
	const spk::Exception inner("inner failure", innerLocation);
	const auto outerLocation = captureLocation();
	const spk::Exception outer("outer failure", std::make_exception_ptr(inner), outerLocation);
	const auto contextLocation = captureLocation();
	const spk::Exception context("while updating", std::make_exception_ptr(outer), contextLocation);

	const std::string expected =
		frame(0, "while updating", contextLocation) + "\n" +
		frame(1, "outer failure", outerLocation) + "\n" +
		frame(2, "inner failure", innerLocation);
	EXPECT_EQ(context.what(), expected);
	EXPECT_EQ(context.what(), expected);

	const auto emptyLocation = captureLocation();
	EXPECT_EQ(spk::Exception("", emptyLocation).what(), frame(0, "", emptyLocation));
}
