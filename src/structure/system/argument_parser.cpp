#include <structure/system/argument_parser.hpp>

#include <exception.hpp>

#include <utility>

namespace spk
{
	ArgumentParser::Option::Option(
		std::string longName,
		char shortName,
		std::string description,
		std::size_t arity,
		std::optional<std::string> defaultValue) :
		longName(std::move(longName)),
		shortName(shortName),
		description(std::move(description)),
		arity(arity),
		defaultValue(std::move(defaultValue))
	{
	}

	void ArgumentParser::setSynopsis(std::string synopsis)
	{
		_synopsis = std::move(synopsis);
	}

	void ArgumentParser::addOption(Option option)
	{
		if (option.longName.empty() && option.shortName == '\0')
		{
			throw spk::Exception("ArgumentParser option requires a long or short name");
		}

		const std::string key = option.longName.empty() ? std::string(1, option.shortName) : option.longName;

		if (_options.contains(key))
		{
			throw spk::Exception("Duplicate option registration: " + key);
		}

		if (option.shortName != '\0' && _shortLookup.contains(option.shortName))
		{
			throw spk::Exception(
				std::string("Duplicate short option: -") + option.shortName);
		}

		const char shortName = option.shortName;
		_options.emplace(key, std::move(option));
		if (shortName != '\0')
		{
			_shortLookup.emplace(shortName, key);
		}
	}

	bool ArgumentParser::_isLongOption(const std::string &token) noexcept
	{
		return token.size() > 2 && token[0] == '-' && token[1] == '-';
	}

	bool ArgumentParser::_isShortOption(const std::string &token) noexcept
	{
		return token.size() >= 2 && token[0] == '-' && (token.size() == 2 || token[1] != '-');
	}

	ArgumentParser::Option &ArgumentParser::_requireByLong(
		const std::string &name)
	{
		const auto iterator = _options.find(name);
		if (iterator == _options.end())
		{
			throw spk::Exception("Unknown option: --" + name);
		}
		return iterator->second;
	}

	ArgumentParser::Option &ArgumentParser::_requireByShort(char shortName)
	{
		const auto iterator = _shortLookup.find(shortName);
		if (iterator == _shortLookup.end())
		{
			throw spk::Exception(
				std::string("Unknown option: -") + shortName);
		}
		return _options.at(iterator->second);
	}

	void ArgumentParser::_consumeValues(
		Option &option,
		int &index,
		int argc,
		char **argv,
		std::optional<std::string> inlineValue)
	{
		std::size_t consumed = 0;
		if (inlineValue.has_value())
		{
			if (option.arity == 0)
			{
				throw spk::Exception(
					"Option --" + option.longName + " does not accept a value");
			}
			option.values.emplace_back(std::move(*inlineValue));
			consumed = 1;
		}

		for (; consumed < option.arity; ++consumed)
		{
			if ((index + 1) >= argc)
			{
				if (!option.longName.empty())
				{
					throw spk::Exception(
						"Missing value for option --" + option.longName);
				}
				throw spk::Exception(
					std::string("Missing value for option -") + option.shortName);
			}

			++index;
			option.values.emplace_back(argv[index]);
		}
	}

	void ArgumentParser::parse(int argc, char **argv)
	{
		_parameters.clear();
		for (auto &[_, option] : _options)
		{
			option.present = false;
			option.values.clear();
		}

		for (int index = 1; index < argc; ++index)
		{
			const std::string token = argv[index];

			if (token == "--")
			{
				for (++index; index < argc; ++index)
				{
					_parameters.emplace_back(argv[index]);
				}
				break;
			}

			if (_isLongOption(token))
			{
				const std::size_t separator = token.find('=');
				const std::string name = token.substr(
					2,
					separator == std::string::npos
						? std::string::npos
						: separator - 2);

				Option &option = _requireByLong(name);
				option.present = true;

				std::optional<std::string> inlineValue;
				if (separator != std::string::npos)
				{
					inlineValue = token.substr(separator + 1);
				}

				_consumeValues(
					option,
					index,
					argc,
					argv,
					std::move(inlineValue));
				continue;
			}

			if (_isShortOption(token))
			{
				if (token.size() != 2)
				{
					throw spk::Exception(
						"Grouped short options are not supported: " + token);
				}

				Option &option = _requireByShort(token[1]);
				option.present = true;
				_consumeValues(option, index, argc, argv);
				continue;
			}

			_parameters.emplace_back(token);
		}

		for (auto &[_, option] : _options)
		{
			if (!option.present && option.defaultValue.has_value())
			{
				option.values.emplace_back(*option.defaultValue);
			}
		}
	}

	void ArgumentParser::printHelp(std::ostream &output) const
	{
		if (!_synopsis.empty())
		{
			output << "Usage: " << _synopsis << "\n\n";
		}

		for (const auto &[_, option] : _options)
		{
			if (!option.longName.empty())
			{
				output << "--" << option.longName;
			}

			if (option.shortName != '\0')
			{
				if (!option.longName.empty())
				{
					output << ", ";
				}
				output << '-' << option.shortName;
			}

			if (option.arity > 0)
			{
				output << " <" << option.arity
					   << (option.arity > 1 ? " values" : " value")
					   << '>';
			}

			if (option.defaultValue.has_value())
			{
				output << " (default: " << *option.defaultValue << ')';
			}

			output << "\n    " << option.description << "\n\n";
		}
	}

	bool ArgumentParser::has(const std::string &name) const
	{
		const auto iterator = _options.find(name);
		return iterator != _options.end() && iterator->second.present;
	}

	const ArgumentParser::Option &ArgumentParser::get(
		const std::string &longName) const
	{
		const auto iterator = _options.find(longName);
		if (iterator == _options.end())
		{
			throw spk::Exception("Unknown option: --" + longName);
		}
		return iterator->second;
	}

	const ArgumentParser::Option &ArgumentParser::get(char shortName) const
	{
		const auto iterator = _shortLookup.find(shortName);
		if (iterator == _shortLookup.end())
		{
			throw spk::Exception(
				std::string("Unknown option: -") + shortName);
		}
		return _options.at(iterator->second);
	}

	const std::vector<std::string> &ArgumentParser::parameters() const noexcept
	{
		return _parameters;
	}
}
