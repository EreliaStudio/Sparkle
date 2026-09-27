#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace spk
{
	class CommandParser
	{
	public:
		enum class Status
		{
			Accepted,
			HelpRequested,
			UnknownCommand,
			InvalidFormat,
			UnknownParameter,
			DuplicateParameter,
			MissingParameter,
			MissingValue,
			TooManyValues,
			TooManyParameters
		};

		struct Parameter
		{
			std::string name;
			std::string description;
			std::size_t arity = 1;
			std::vector<std::string> defaultValues;
		};

		struct Invocation
		{
			std::string command;
			std::unordered_map<std::string, std::vector<std::string>> parameters;

			[[nodiscard]] const std::vector<std::string> &get(const std::string &name) const;
		};

		using Callback = std::function<void(const Invocation &)>;

		struct Command
		{
			std::string name;
			std::string description;
			std::vector<Parameter> parameters;
			Callback callback;
		};

		struct Result
		{
			Status status = Status::InvalidFormat;
			std::string command;
			std::string parameter;
			std::size_t expectedValueCount = 0;
			std::size_t actualValueCount = 0;
		};

	private:
		std::unordered_map<std::string, Command> _commands;

		[[nodiscard]] static std::vector<std::string> _tokenize(const std::string &input);
		[[nodiscard]] static std::vector<std::string> _splitValues(const std::string &value);
		[[nodiscard]] const Parameter *_parameter(const Command &command, const std::string &name) const noexcept;

	public:
		void addCommand(Command command);
		[[nodiscard]] Result execute(const std::string &input) const;
		[[nodiscard]] std::string help() const;
		[[nodiscard]] std::string help(const std::string &command) const;
	};
}
