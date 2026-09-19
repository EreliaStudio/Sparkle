#pragma once

#include "container/json/object.hpp"
#include "exception.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace spk::JSON
{
	class Loader
	{
	public:
		[[nodiscard]] static spk::JSON::Value parseFile(const std::filesystem::path &file);
	};

	namespace detail
	{
		[[nodiscard]] inline std::string makeErrorMessage(
			const std::filesystem::path &file,
			const std::string &path,
			const std::string &message)
		{
			return file.generic_string() + ":" + path + ": " + message;
		}

		template <typename TType>
		struct JsonSequence : std::false_type
		{
		};
		template <typename TElement, typename TAllocator>
		struct JsonSequence<std::vector<TElement, TAllocator>> : std::true_type
		{
			using Element = TElement;
		};

		template <typename TType>
		struct JsonFixedArray : std::false_type
		{
		};
		template <typename TElement, std::size_t TCount>
		struct JsonFixedArray<std::array<TElement, TCount>> : std::true_type
		{
			using Element = TElement;
			static constexpr std::size_t Count = TCount;
		};

		template <typename TType>
		[[nodiscard]] TType readJsonValue(const spk::JSON::Value &value)
		{
			if constexpr (JsonSequence<TType>::value)
			{
				using Element = typename JsonSequence<TType>::Element;
				TType result;
				const spk::JSON::Value::Array &array = value.asArray();
				result.reserve(array.size());
				for (const spk::JSON::Value &element : array)
				{
					result.push_back(readJsonValue<Element>(element));
				}
				return result;
			}
			else if constexpr (JsonFixedArray<TType>::value)
			{
				using Element = typename JsonFixedArray<TType>::Element;
				const spk::JSON::Value::Array &array = value.asArray();
				if (array.size() != JsonFixedArray<TType>::Count)
				{
					throw std::runtime_error(
						"expected exactly " + std::to_string(JsonFixedArray<TType>::Count) + " elements, got " +
						std::to_string(array.size()));
				}
				TType result{};
				for (std::size_t index = 0; index < array.size(); ++index)
				{
					result[index] = readJsonValue<Element>(array[index]);
				}
				return result;
			}
			else
			{
				return value.as<TType>();
			}
		}
	}

	class Reader
	{
	private:
		const spk::JSON::Value &_value;
		std::filesystem::path _file;
		std::string _path;

		void _requireObject() const;
		[[nodiscard]] const spk::JSON::Value &_requireMember(const std::string &key) const;

	public:
		Reader(const spk::JSON::Value &value, std::filesystem::path file, std::string path = "$");

		[[nodiscard]] const std::filesystem::path &file() const noexcept;
		[[nodiscard]] const std::string &path() const noexcept;
		[[nodiscard]] const spk::JSON::Value &value() const noexcept;
		[[nodiscard]] std::string pathFor(const std::string &key) const;
		[[nodiscard]] bool contains(const std::string &key) const;

		template <typename TType>
		[[nodiscard]] TType require(const std::string &key) const
		{
			const spk::JSON::Value &member = _requireMember(key);

			try
			{
				return detail::readJsonValue<TType>(member);
			} catch (...)
			{
				throw spk::Exception(
					detail::makeErrorMessage(_file, pathFor(key), "invalid value"),
					std::current_exception());
			}
		}

		template <typename TType>
		[[nodiscard]] TType optional(const std::string &key, TType defaultValue) const
		{
			_requireObject();
			const spk::JSON::Value *member = _value.find(key);
			if (member == nullptr)
			{
				return defaultValue;
			}

			try
			{
				return detail::readJsonValue<TType>(*member);
			} catch (const std::exception &exception)
			{
				throw spk::Exception(
					detail::makeErrorMessage(_file, pathFor(key), "invalid value"),
					std::current_exception());
			}
		}

		template <typename TType, typename TMap>
		[[nodiscard]] TType requireEnum(const std::string &key, const TMap &values) const
		{
			const std::string value = require<std::string>(key);
			const auto iterator = values.find(value);
			if (iterator == values.end())
			{
				std::string knownValues;
				for (const auto &[name, unused] : values)
				{
					(void)unused;
					if (!knownValues.empty())
					{
						knownValues += ", ";
					}
					knownValues += name;
				}

				throw spk::Exception(detail::makeErrorMessage(_file, pathFor(key), "unknown enum value '" + value + "' (expected one of: " + knownValues + ")"));
			}

			return iterator->second;
		}

		[[nodiscard]] Reader child(const std::string &key) const;
		[[nodiscard]] std::vector<Reader> childArray(const std::string &key) const;
		void forbidUnknown(std::initializer_list<std::string_view> allowedKeys) const;
	};
}
