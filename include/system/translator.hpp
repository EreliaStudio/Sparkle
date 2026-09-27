#pragma once

#include <filesystem>
#include <format>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace spk
{
	class Translator
	{
	private:
		std::unordered_map<std::string, std::string> _translations;
		mutable std::shared_mutex _mutex;

		[[nodiscard]] std::string _translate(
			const std::string &key,
			std::format_args arguments) const;

	public:
		void append(std::filesystem::path path);
		void append(std::string key, std::string value);
		void clear();

		template <typename... TArgs>
		[[nodiscard]] std::string translate(
			std::string key,
			TArgs &&...args) const
		{
			return _translate(
				key,
				std::make_format_args(args...));
		}
	};
}
