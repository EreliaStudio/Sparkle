#include "exception.hpp"

#include <utility>

namespace spk
{
	Exception::Exception(std::string message, std::source_location location) :
		_message(std::move(message)),
		_location(location)
	{
		_format();
	}

	Exception::Exception(
		std::string message,
		std::exception_ptr cause,
		std::source_location location) :
		_message(std::move(message)),
		_location(location),
		_cause(std::move(cause))
	{
		_format();
	}

	void Exception::_composeMessage(std::string &output, std::size_t indentation) const
	{
		const std::string prefix(indentation, '\t');

		output += prefix;
		output += _location.file_name();
		output += ":";
		output += std::to_string(_location.line());
		output += '\n';

		output += prefix;
		output += '\t';
		output += _message;

		if (_cause == nullptr)
			return;

		output += '\n';

		try
		{
			std::rethrow_exception(_cause);
		}
		catch (const Exception &exception)
		{
			exception._composeMessage(output, indentation + 1);
		}
		catch (const std::exception &exception)
		{
			output += std::string(indentation + 1, '\t');
			output += exception.what();
		}
		catch (...)
		{
			output += std::string(indentation + 1, '\t');
			output += "Unknown exception";
		}
	}

	void Exception::_format()
	{
		_formattedMessage.clear();
		_composeMessage(_formattedMessage, 0);
	}

	const char *Exception::what() const noexcept
	{
		return _formattedMessage.c_str();
	}

	const std::string &Exception::message() const noexcept
	{
		return _message;
	}

	const std::source_location &Exception::location() const noexcept
	{
		return _location;
	}

	const std::exception_ptr &Exception::cause() const noexcept
	{
		return _cause;
	}
}
