#pragma once

#include "structure/spk_iostream.hpp"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace spk
{
	class ArgumentParser
	{
	public:
		struct Option
		{
			bool present = false;
			std::wstring longName = L"";
			char shortName = '\0';
			std::wstring description = L"";
			size_t arity = 0;
			std::vector<std::wstring> values;
			std::optional<std::wstring> defaultValue;

			Option(
				const std::wstring &p_longName,
				char p_shortName,
				const std::wstring &p_description,
				size_t p_arity = 0,
				std::optional<std::wstring> p_defaultVal = std::nullopt);
		};

	private:
		std::unordered_map<std::wstring, Option> _options;
		std::unordered_map<char, std::wstring> _shortLookup;
		std::vector<std::wstring> _parameters;
		std::wstring _synopsis = L"";

		static bool _isLongOpt(const std::string &p_tok) noexcept;
		static bool _isShortOpt(const std::string &p_tok) noexcept;
		Option &_requireByLong(const std::wstring &p_name);
		Option &_requireByShort(char p_shortName);
		void _consumeValues(Option &p_opt, int &p_i, int p_argc, char **p_argv);

	public:
		explicit ArgumentParser();

		void setSynopsis(const std::wstring &p_synopsis);
		void addOption(const Option &p_option);

		void parse(char **p_argv, int p_argc);

		void printHelp(IOStream &p_op = spk::cout()) const;

		bool has(const std::wstring &p_name) const;
		const Option &get(const std::wstring &p_longName) const;
		const Option &get(char p_shortName) const;
	};
}
