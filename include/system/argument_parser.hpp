#pragma once

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
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
			std::string longName;
			char shortName = '\0';
			std::string description;
			std::size_t arity = 0;
			std::vector<std::string> values;
			std::optional<std::string> defaultValue;

			Option(
				std::string longName,
				char shortName,
				std::string description,
				std::size_t arity = 0,
				std::optional<std::string> defaultValue = std::nullopt);
		};

	private:
		std::unordered_map<std::string, Option> _options;
		std::unordered_map<char, std::string> _shortLookup;
		std::vector<std::string> _parameters;
		std::string _synopsis;

		[[nodiscard]] static bool _isLongOption(const std::string &token) noexcept;
		[[nodiscard]] static bool _isShortOption(const std::string &token) noexcept;
		[[nodiscard]] Option &_requireByLong(const std::string &name);
		[[nodiscard]] Option &_requireByShort(char shortName);
		void _consumeValues(
			Option &option,
			int &index,
			int argc,
			char **argv,
			std::optional<std::string> inlineValue = std::nullopt);

	public:
		ArgumentParser() = default;

		void setSynopsis(std::string synopsis);
		void addOption(Option option);

		void parse(int argc, char **argv);

		void printHelp(std::ostream &output = std::cout) const;

		[[nodiscard]] bool has(const std::string &name) const;
		[[nodiscard]] const Option &get(const std::string &longName) const;
		[[nodiscard]] const Option &get(char shortName) const;
		[[nodiscard]] const std::vector<std::string> &parameters() const noexcept;
	};
}
