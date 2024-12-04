#include "structure/system/spk_argument_parser.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk
{
	// ===== Option =====
	ArgumentParser::Option::Option(
		const std::wstring &p_longName,
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

	// ===== ArgumentParser =====
	ArgumentParser::ArgumentParser() = default;

	void ArgumentParser::setSynopsis(const std::wstring &p_synopsis)
	{
		_synopsis = p_synopsis;
	}

	void ArgumentParser::addOption(const Option &p_option)
	{
		// Canonical key: prefer long name; else single-char string of short name.
		const std::wstring key = (!p_option.longName.empty()) ? p_option.longName : std::wstring(1, static_cast<wchar_t>(p_option.shortName));

		// Duplicate long/only key?
		if (_options.find(key) != _options.end())
		{
			throw std::runtime_error("Duplicate option registration: " + std::string(key.begin(), key.end()));
		}

		// Duplicate short?
		if (p_option.shortName != '\0')
		{
			if (_shortLookup.find(p_option.shortName) != _shortLookup.end())
			{
				throw std::runtime_error(std::string("Duplicate short option: -") + p_option.shortName);
			}
		}

		_options.emplace(key, p_option);
		if (p_option.shortName != '\0')
		{
			_shortLookup.emplace(p_option.shortName, key);
		}
	}

	// --- helpers ---
	bool ArgumentParser::_isLongOpt(const std::string &p_tok) noexcept
	{
		// Starts with "--" and length > 2 (so not just "--")
		return p_tok.size() > 2 && p_tok[0] == '-' && p_tok[1] == '-';
	}

	bool ArgumentParser::_isShortOpt(const std::string &p_tok) noexcept
	{
		// Starts with "-" but not "--"
		return p_tok.size() >= 2 && p_tok[0] == '-' && (p_tok.size() == 2 || p_tok[1] != '-');
	}

	ArgumentParser::Option &ArgumentParser::_requireByLong(const std::wstring &p_name)
	{
		auto it = _options.find(p_name);
		if (it == _options.end())
		{
			throw std::runtime_error("Unknown option: --" + std::string(p_name.begin(), p_name.end()));
		}
		return it->second;
	}

	ArgumentParser::Option &ArgumentParser::_requireByShort(char p_shortName)
	{
		auto it = _shortLookup.find(p_shortName);
		if (it == _shortLookup.end())
		{
			throw std::runtime_error(std::string("Unknown option: -") + p_shortName);
		}
		return _options.at(it->second);
	}

	void ArgumentParser::_consumeValues(Option &p_opt, int &p_i, int p_argc, char **p_argv)
	{
		for (size_t j = 0; j < p_opt.arity; ++j)
		{
			if ((p_i + 1) >= p_argc)
			{
				// Prefer the nicest flag name we can show
				if (!p_opt.longName.empty())
				{
					throw std::runtime_error("Missing value for option --" + std::string(p_opt.longName.begin(), p_opt.longName.end()));
				}
				throw std::runtime_error(std::string("Missing value for option -") + p_opt.shortName);
			}
			++p_i;
			p_opt.values.emplace_back(StringUtils::stringToWString(std::string(p_argv[p_i])));
		}
	}

	void ArgumentParser::parse(char **p_argv, int p_argc)
	{
		// Reset state for a fresh parse
		_parameters.clear();
		for (auto &[_, opt] : _options)
		{
			opt.present = false;
			opt.values.clear();
		}

		for (int i = 1; i < p_argc; ++i)
		{
			const std::string token = p_argv[i];

			// End of options marker: "--" — remaining tokens are parameters
			if (token == "--")
			{
				for (++i; i < p_argc; ++i)
				{
					_parameters.emplace_back(StringUtils::stringToWString(std::string(p_argv[i])));
				}
				break;
			}

			if (_isLongOpt(token))
			{
				const std::wstring name = StringUtils::stringToWString(token.substr(2));
				Option &opt = _requireByLong(name);
				opt.present = true;
				_consumeValues(opt, i, p_argc, p_argv);
				continue;
			}

			if (_isShortOpt(token))
			{
				// We do not support grouped short options (-abc). Keep the rule explicit.
				if (token.size() != 2)
				{
					throw std::runtime_error("Grouped short options are not supported: " + token);
				}

				const char shortName = token[1];
				Option &opt = _requireByShort(shortName);
				opt.present = true;
				_consumeValues(opt, i, p_argc, p_argv);
				continue;
			}

			// Positional parameter
			_parameters.emplace_back(StringUtils::stringToWString(token));
		}

		// Inject defaults for absent options
		for (auto &[_, opt] : _options)
		{
			if (opt.present == false && opt.defaultValue.has_value())
			{
				opt.values.emplace_back(opt.defaultValue.value());
			}
		}
	}

	bool ArgumentParser::has(const std::wstring &p_name) const
	{
		auto it = _options.find(p_name);
		return it != _options.end() && it->second.present;
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
		auto idx = _shortLookup.find(p_shortName);
		if (idx != _shortLookup.end())
		{
			return _options.at(idx->second);
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

			if (!p_option.longName.empty())
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

		for (const auto &[_, option] : _options)
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
