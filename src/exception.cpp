#include "exception.hpp"

namespace spk
{
	Exception::Exception(std::string message, std::source_location location) :
		_message(std::move(message)),
		_location(location)
	{
		_format();
	}

	Exception::Exception(std::string message, std::exception_ptr cause, std::source_location location) :
		_message(std::move(message)),
		_location(location),
		_cause(std::move(cause))
	{
		_format();
	}

	void Exception::_appendFrame(
		std::string &output,
		std::size_t indentation,
		const std::string &message,
		const std::source_location &location)
	{
		const std::string prefix(indentation, '\t');

		output += prefix;
		output += location.file_name();
		output += ":";
		output += std::to_string(location.line());
		output += '\n';

		output += prefix;
		output += '\t';
		output += message;
	}

	void Exception::_append(std::string &output, std::size_t indentation) const
	{
		for (auto context = _contexts.rbegin(); context != _contexts.rend(); ++context)
		{
			_appendFrame(output, indentation, context->message, context->location);
			output += '\n';
			++indentation;
		}

		_appendFrame(output, indentation, _message, _location);

		if (_cause == nullptr)
		{
			return;
		}

		output += '\n';

		try
		{
			std::rethrow_exception(_cause);
		}
		catch (const Exception &exception)
		{
			exception._append(output, indentation + 1);
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
		_append(_formattedMessage, 0);
	}

	void Exception::addContext(std::string message, std::source_location location)
	{
		_contexts.push_back(Context{.message = std::move(message), .location = location});
		_format();
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
