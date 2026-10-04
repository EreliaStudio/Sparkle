#pragma once

#include "exception.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
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
		void _addCommand(std::unique_ptr<Command> command);

	public:
		void addCommand(LambdaCommandDefinition command);

		template <typename TCommandType>
			requires std::derived_from<TCommandType, Command>
		[[nodiscard]] TCommandType &command()
		{
			TCommandType *result = nullptr;
			for (auto &[name, registeredCommand] : _commands)
			{
				(void)name;
				TCommandType *candidate =
					dynamic_cast<TCommandType *>(registeredCommand.get());
				if (candidate == nullptr)
				{
					continue;
				}
				if (result != nullptr)
				{
					throw spk::Exception(
						"Multiple commands registered for requested type");
				}
				result = candidate;
			}
			if (result == nullptr)
			{
				throw spk::Exception(
					"Command type is not registered");
			}
			return *result;
		}

		template <typename TCommandType>
			requires std::derived_from<TCommandType, Command>
		[[nodiscard]] const TCommandType &command() const
		{
			const TCommandType *result = nullptr;
			for (const auto &[name, registeredCommand] : _commands)
			{
				(void)name;
				const TCommandType *candidate =
					dynamic_cast<const TCommandType *>(registeredCommand.get());
				if (candidate == nullptr)
				{
					continue;
				}
				if (result != nullptr)
				{
					throw spk::Exception(
						"Multiple commands registered for requested type");
				}
				result = candidate;
			}
			if (result == nullptr)
			{
				throw spk::Exception(
					"Command type is not registered");
			}
			return *result;
		}

		template <typename TCommandType, typename... TArgs>
			requires std::derived_from<TCommandType, Command>
		TCommandType &addCommand(TArgs &&...args)
		{
			auto command = std::make_unique<TCommandType>(
				std::forward<TArgs>(args)...);
			TCommandType &result = *command;
			_addCommand(std::move(command));
			return result;
		}
		[[nodiscard]] Result execute(const std::string &input) const;
		[[nodiscard]] std::string help() const;
		[[nodiscard]] std::string help(const std::string &command) const;
	};
}
