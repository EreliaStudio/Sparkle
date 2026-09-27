#include <system/command_parser.hpp>

#include <exception.hpp>

#include <sstream>
#include <utility>

namespace spk
{
	const std::vector<std::string> &CommandParser::Invocation::get(const std::string &name) const
	{
		const auto iterator = parameters.find(name);
		if (iterator == parameters.end())
		{
			throw spk::Exception("Unknown command parameter: " + name);
		}
		return iterator->second;
	}

	std::vector<std::string> CommandParser::_tokenize(const std::string &input)
	{
		std::istringstream stream(input);
		std::vector<std::string> result;
		std::string token;
		while (stream >> token)
		{
			result.emplace_back(std::move(token));
		}
		return result;
	}

	std::vector<std::string> CommandParser::_splitValues(const std::string &value)
	{
		std::vector<std::string> result;
		std::size_t begin = 0;
		while (begin <= value.size())
		{
			const std::size_t separator = value.find(',', begin);
			result.emplace_back(value.substr(begin, separator - begin));
			if (separator == std::string::npos)
			{
				break;
			}
			begin = separator + 1;
		}
		return result;
	}

	const CommandParser::Parameter *CommandParser::_parameter(const Command &command, const std::string &name) const noexcept
	{
		for (const Parameter &parameter : command.parameters)
		{
			if (parameter.name == name)
			{
				return &parameter;
			}
		}
		return nullptr;
	}

	void CommandParser::addCommand(Command command)
	{
		if (command.name.empty() == true)
		{
			throw spk::Exception("CommandParser command name cannot be empty");
		}
		if (_commands.contains(command.name) == true)
		{
			throw spk::Exception("Duplicate command registration: " + command.name);
		}
		for (std::size_t index = 0; index < command.parameters.size(); ++index)
		{
			const Parameter &parameter = command.parameters[index];
			if (parameter.name.empty() == true || parameter.arity == 0)
			{
				throw spk::Exception("CommandParser parameter requires a name and non-zero arity");
			}
			if (parameter.defaultValues.empty() == false && parameter.defaultValues.size() != parameter.arity)
			{
				throw spk::Exception("CommandParser parameter default value count must match its arity");
			}
			for (std::size_t other = 0; other < index; ++other)
			{
				if (command.parameters[other].name == parameter.name)
				{
					throw spk::Exception("Duplicate command parameter registration: " + parameter.name);
				}
			}
		}
		_commands.emplace(command.name, std::move(command));
	}

	CommandParser::Result CommandParser::execute(const std::string &input) const
	{
		const std::vector<std::string> tokens = _tokenize(input);
		if (tokens.empty() == true || tokens.front().size() < 2 || tokens.front().front() != '/')
		{
			return {.status = Status::InvalidFormat};
		}

		const std::string commandName = tokens.front().substr(1);
		const auto commandIterator = _commands.find(commandName);
		if (commandIterator == _commands.end())
		{
			return {.status = Status::UnknownCommand, .command = commandName};
		}
		const Command &command = commandIterator->second;
		Invocation invocation{.command = commandName};
		std::size_t positionalIndex = 0;

		for (std::size_t index = 1; index < tokens.size(); ++index)
		{
			const std::string &token = tokens[index];
			if (token == "--help")
			{
				if (tokens.size() != 2)
				{
					return {.status = Status::InvalidFormat, .command = commandName};
				}
				return {.status = Status::HelpRequested, .command = commandName};
			}

			if (token.starts_with("--") == true)
			{
				const std::size_t separator = token.find('=');
				const std::string parameterName = token.substr(2, separator == std::string::npos ? std::string::npos : separator - 2);
				const Parameter *parameter = _parameter(command, parameterName);
				if (parameter == nullptr)
				{
					return {.status = Status::UnknownParameter, .command = commandName, .parameter = parameterName};
				}
				if (invocation.parameters.contains(parameterName) == true)
				{
					return {.status = Status::DuplicateParameter, .command = commandName, .parameter = parameterName};
				}

				std::vector<std::string> values;
				if (separator != std::string::npos)
				{
					values = _splitValues(token.substr(separator + 1));
				}
				else
				{
					for (std::size_t count = 0; count < parameter->arity && index + 1 < tokens.size(); ++count)
					{
						if (tokens[index + 1].starts_with("--") == true)
						{
							break;
						}
						values.emplace_back(tokens[++index]);
					}
				}
				if (values.size() < parameter->arity)
				{
					return {.status = Status::MissingValue, .command = commandName, .parameter = parameterName, .expectedValueCount = parameter->arity, .actualValueCount = values.size()};
				}
				if (values.size() > parameter->arity)
				{
					return {.status = Status::TooManyValues, .command = commandName, .parameter = parameterName, .expectedValueCount = parameter->arity, .actualValueCount = values.size()};
				}
				invocation.parameters.emplace(parameterName, std::move(values));
				continue;
			}

			while (positionalIndex < command.parameters.size() && invocation.parameters.contains(command.parameters[positionalIndex].name) == true)
			{
				++positionalIndex;
			}
			if (positionalIndex >= command.parameters.size())
			{
				return {.status = Status::TooManyParameters, .command = commandName};
			}
			const Parameter &parameter = command.parameters[positionalIndex++];
			std::vector<std::string> values{token};
			while (values.size() < parameter.arity && index + 1 < tokens.size() && tokens[index + 1].starts_with("--") == false)
			{
				values.emplace_back(tokens[++index]);
			}
			if (values.size() < parameter.arity)
			{
				return {.status = Status::MissingValue, .command = commandName, .parameter = parameter.name, .expectedValueCount = parameter.arity, .actualValueCount = values.size()};
			}
			invocation.parameters.emplace(parameter.name, std::move(values));
		}

		for (const Parameter &parameter : command.parameters)
		{
			if (invocation.parameters.contains(parameter.name) == true)
			{
				continue;
			}
			if (parameter.defaultValues.empty() == false)
			{
				invocation.parameters.emplace(parameter.name, parameter.defaultValues);
				continue;
			}
			return {.status = Status::MissingParameter, .command = commandName, .parameter = parameter.name, .expectedValueCount = parameter.arity};
		}

		if (command.callback)
		{
			command.callback(invocation);
		}
		return {.status = Status::Accepted, .command = commandName};
	}

	std::string CommandParser::help() const
	{
		std::ostringstream output;
		for (const auto &[name, command] : _commands)
		{
			output << '/' << name << " - " << command.description << '\n';
		}
		return output.str();
	}

	std::string CommandParser::help(const std::string &commandName) const
	{
		const auto iterator = _commands.find(commandName);
		if (iterator == _commands.end())
		{
			return {};
		}
		const Command &command = iterator->second;
		std::ostringstream output;
		output << "Usage: /" << command.name;
		for (const Parameter &parameter : command.parameters)
		{
			output << " [--" << parameter.name << " <" << parameter.arity << (parameter.arity == 1 ? " value" : " values") << ">]";
		}
		output << "\n" << command.description << "\n";
		for (const Parameter &parameter : command.parameters)
		{
			output << "  --" << parameter.name << ": " << parameter.description;
			if (parameter.defaultValues.empty() == false)
			{
				output << " (default:";
				for (const std::string &value : parameter.defaultValues)
				{
					output << ' ' << value;
				}
				output << ')';
			}
			output << '\n';
		}
		return output.str();
	}
}
