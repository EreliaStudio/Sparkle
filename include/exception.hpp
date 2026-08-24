#pragma once

#include <exception>
#include <source_location>
#include <string>
#include <vector>

namespace spk
{
	class Exception : public std::exception
	{
	private:
		struct Context
		{
			std::string message;
			std::source_location location;
		};

		std::string _message;
		std::source_location _location;
		std::exception_ptr _cause;
		std::vector<Context> _contexts;
		std::string _formattedMessage;

		static void _appendFrame(
			std::string &output,
			std::size_t indentation,
			const std::string &message,
			const std::source_location &location);
		void _append(std::string &output, std::size_t indentation) const;
		void _format();

	public:
		explicit Exception(
			std::string message,
			std::source_location location = std::source_location::current());

		Exception(
			std::string message,
			std::exception_ptr cause,
			std::source_location location = std::source_location::current());

		void addContext(
			std::string message,
			std::source_location location = std::source_location::current());

		[[nodiscard]] const char *what() const noexcept override;
		[[nodiscard]] const std::string &message() const noexcept;
		[[nodiscard]] const std::source_location &location() const noexcept;
		[[nodiscard]] const std::exception_ptr &cause() const noexcept;
	};
}
