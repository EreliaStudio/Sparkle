#include "structure/system/spk_argument_parser.hpp"

#include "utils/spk_string_utils.hpp"

namespace spk
{
	ArgumentParser::Option::Option(const std::wstring &p_longName,
								   char p_shortName,
								   const std::wstring &p_description,
								   size_t p_arity,
								   std::optional<std::wstring> p_defaultVal) :
		longName(p_longName),
		shortName(p_shortName),
		description(p_description),
		arity(p_arity)
	{
		if (p_defaultVal)
		{
			defaultValue = *p_defaultVal;
		}
	}

	ArgumentParser::ArgumentParser() = default;

	void ArgumentParser::setSynopsis(const std::wstring &p_synopsis)
	{
		_synopsis = p_synopsis;
	}

	void ArgumentParser::addOption(const Option &p_option)
	{
		std::wstring key = !p_option.longName.empty() ? p_option.longName : std::wstring(1, p_option.shortName);

		if (_options.find(key) != _options.end())
		{
			throw std::runtime_error("Duplicate option registration: " + std::string(key.begin(), key.end()));
		}

		_options.emplace(key, p_option);
	}

	static ArgumentParser::Option *findByShort(std::unordered_map<std::wstring, ArgumentParser::Option> &p_opts, char p_c)
	{
		for (auto &[name, option] : p_opts)
		{
			if (option.shortName == p_c)
			{
				return &option;
			}
		}
		return nullptr;
	}

	void ArgumentParser::parse(char **p_argv, int p_argc)
	{
		for (int i = 1; i < p_argc; i++)
		{
			std::string token = p_argv[i];

			if (token == "--")
			{
				break;
			}

			if (token.rfind("--", 0) == 0)
			{
				std::wstring name = StringUtils::stringToWString(token.substr(2));
				auto it = _options.find(name);
				if (it == _options.end())
				{
					throw std::runtime_error("Unknown option: --" + token.substr(2));
				}

				Option &option = it->second;
				option.present = true;

				for (size_t j = 0; j < option.arity; j++)
				{
					if ((i + 1) >= p_argc)
					{
						throw std::runtime_error("Missing value for option --" + token.substr(2));
					}
					i++;

					option.values.emplace_back(StringUtils::stringToWString(std::string(p_argv[i])));
				}
				continue;
			}

			if (token.size() > 1 && token[0] == '-')
			{
				if (token.size() != 2)
				{
					throw std::runtime_error("Grouped short options are not supported: " + token);
				}

				char shortName = token[1];

				Option *optPtr = findByShort(_options, shortName);

				if (optPtr == nullptr)
				{
					throw std::runtime_error(std::string("Unknown option: -") + shortName);
				}

				Option &option = *optPtr;

				option.present = true;

				for (size_t j = 0; j < option.arity; j++)
				{
					if ((i + 1) >= p_argc)
					{
						throw std::runtime_error(std::string("Missing value for option -") + shortName);
					}
					i++;
					option.values.emplace_back(StringUtils::stringToWString(std::string(p_argv[i])));
				}

				continue;
			}
			_parameters.emplace_back(StringUtils::stringToWString(token));
		}

		for (auto &[name, option] : _options)
		{
			if (option.present == false && option.defaultValue.has_value())
			{
				option.values.emplace_back(option.defaultValue.value());
			}
		}
	}

	bool ArgumentParser::has(const std::wstring &p_name) const
	{
		auto it = _options.find(p_name);

		if (it != _options.end())
		{
			return it->second.present;
		}

		return false;
	}

	const ArgumentParser::Option &ArgumentParser::get(const std::wstring &p_longName) const
	{
		auto it = _options.find(p_longName);

		if (it != _options.end())
		{
			return it->second;
		}

		throw std::runtime_error("Unknown option: " + std::string(p_longName.begin(), p_longName.end()));
	}

	const ArgumentParser::Option &ArgumentParser::get(char p_shortName) const
	{
		for (const auto &[name, option] : _options)
		{
			if (option.shortName == p_shortName)
			{
				return option;
			}
		}
		throw std::runtime_error(std::string("Unknown option: -") + p_shortName);
	}

	void ArgumentParser::printHelp(IOStream &p_op) const
	{
		if (!_synopsis.empty())
		{
			p_op << L"Usage: " << _synopsis << L"\n\n";
		}

		auto signature = [](const Option &p_option) -> std::wstring
		{
			std::wstring result;

			if (p_option.longName.empty() == false)
			{
				result += L"--" + p_option.longName;
			}

			if (p_option.shortName != '\0')
			{
				if (!result.empty())
				{
					result += L", ";
				}
				result += L"-";
				result += static_cast<wchar_t>(p_option.shortName);
			}

			return result;
		};

		for (const auto &[name, option] : _options)
		{
			p_op << signature(option);

			if (option.arity > 0)
			{
				p_op << L" <" << option.arity << (option.arity > 1 ? L" values" : L" value") << L">";
			}

			if (option.defaultValue)
			{
				p_op << L" (default: " << option.defaultValue.value() << L")";
			}

			p_op << L"\n    " << option.description << L"\n\n";
		}
	}
}
