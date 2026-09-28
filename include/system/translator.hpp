#pragma once

#include <filesystem>
#include <format>
#include <shared_mutex>
#include <source_location>
#include <string>
#include <unordered_map>
#include <utility>

namespace spk
{
	class Translator
	{
	public:
		struct LocatedKey
		{
			std::string value;
			std::source_location location;

			LocatedKey(
				const char *value,
				std::source_location location =
					std::source_location::current()) :
				value(value),
				location(location)
			{
			}

			LocatedKey(
				std::string value,
				std::source_location location =
					std::source_location::current()) :
				value(std::move(value)),
				location(location)
			{
			}
		};

	private:
		std::unordered_map<std::string, std::string> _translations;
		mutable std::shared_mutex _mutex;

		[[nodiscard]] std::string _translate(
			const std::string &key,
			std::format_args arguments,
			std::source_location location) const;

	public:
		void append(std::filesystem::path path);
		void append(std::string key, std::string value);
		void clear();

		template <typename... TArgs>
		[[nodiscard]] std::string translate(
			LocatedKey key,
			TArgs &&...args) const
		{
			return _translate(
				key.value,
				std::make_format_args(args...),
				key.location);
		}
	};
}
