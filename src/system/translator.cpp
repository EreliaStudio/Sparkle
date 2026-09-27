#include <system/translator.hpp>

#include <container/json/reader.hpp>
#include <diagnostics/logger.hpp>
#include <exception.hpp>

#include <exception>
#include <format>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace spk
{
	void Translator::append(std::filesystem::path path)
	{
		const spk::JSON::Value document =
			spk::JSON::Loader::parseFile(path);

		if (document.isObject() == false)
		{
			throw spk::Exception(
				"Translation catalog root must be a JSON object: " +
				path.generic_string());
		}

		std::unordered_map<std::string, std::string> translations;
		translations.reserve(document.size());

		for (const auto &[key, value] : document.asObject())
		{
			if (key.empty() == true)
			{
				throw spk::Exception(
					"Translation key cannot be empty: " +
					path.generic_string());
			}
			if (value.isString() == false)
			{
				throw spk::Exception(
					"Translation value must be a string for key '" +
					key + "': " + path.generic_string());
			}

			translations.insert_or_assign(
				key,
				value.as<std::string>());
		}

		const std::unique_lock lock(_mutex);
		for (const auto &[key, value] : translations)
		{
			(void)value;
			if (_translations.contains(key) == true)
			{
				throw spk::Exception(
					"Duplicate translation key: " + key);
			}
		}

		for (auto &[key, value] : translations)
		{
			_translations.emplace(
				std::move(key),
				std::move(value));
		}
	}

	void Translator::append(
		std::string key,
		std::string value)
	{
		if (key.empty() == true)
		{
			throw spk::Exception("Translation key cannot be empty");
		}

		const std::unique_lock lock(_mutex);
		if (_translations.contains(key) == true)
		{
			throw spk::Exception(
				"Duplicate translation key: " + key);
		}

		_translations.emplace(
			std::move(key),
			std::move(value));
	}

	void Translator::clear()
	{
		const std::unique_lock lock(_mutex);
		_translations.clear();
	}

	std::string Translator::_translate(
		const std::string &key,
		std::format_args arguments) const
	{
		std::string format;
		bool missing = false;
		{
			const std::shared_lock lock(_mutex);
			const auto iterator = _translations.find(key);
			if (iterator == _translations.end())
			{
				missing = true;
			}
			else
			{
				format = iterator->second;
			}
		}

		if (missing == true)
		{
			SPK_LOG(Warning)
				<< "Missing translation key: "
				<< key
				<< std::endl;
			return key;
		}

		try
		{
			return std::vformat(format, arguments);
		}
		catch (const std::format_error &)
		{
			throw spk::Exception(
				"Invalid translation format for key: " + key,
				std::current_exception());
		}
	}
}
