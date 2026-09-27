#pragma once

#include <cstddef>
#include <functional>
#include <memory>
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
			bool optional = false;
		};

		struct Invocation
		{
			std::string command;
			std::unordered_map<std::string, std::vector<std::string>> parameters;

			[[nodiscard]] const std::vector<std::string> &get(const std::string &name) const;
		};

		using Callback = std::function<void(const Invocation &)>;

		class Command
		{
		private:
			std::string _name;
			std::string _description;
			std::vector<Parameter> _parameters;

		public:
			Command(
				std::string name,
				std::string description,
				std::vector<Parameter> parameters = {});
			virtual ~Command() = default;

			[[nodiscard]] const std::string &name() const noexcept;
			[[nodiscard]] const std::string &description() const noexcept;
			[[nodiscard]] const std::vector<Parameter> &parameters() const noexcept;

			virtual void execute(const Invocation &invocation) = 0;
		};

		class LambdaCommand final : public Command
		{
		private:
			Callback _callback;

		public:
			LambdaCommand(
				std::string name,
				std::string description,
				std::vector<Parameter> parameters,
				Callback callback);

			void execute(const Invocation &invocation) override;
		};

		struct LambdaCommandDefinition
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
		std::unordered_map<std::string, std::unique_ptr<Command>> _commands;

		[[nodiscard]] static std::vector<std::string> _tokenize(const std::string &input);
		[[nodiscard]] static std::vector<std::string> _splitValues(const std::string &value);
		[[nodiscard]] const Parameter *_parameter(const Command &command, const std::string &name) const noexcept;
		static void _validateParameters(const std::vector<Parameter> &parameters);

	public:
		void addCommand(LambdaCommandDefinition command);
		void addCommand(std::unique_ptr<Command> command);
		[[nodiscard]] Result execute(const std::string &input) const;
		[[nodiscard]] std::string help() const;
		[[nodiscard]] std::string help(const std::string &command) const;
	};
}
