#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "container/json/concepts.hpp"

namespace spk::JSON
{
	namespace detail
	{
		[[noreturn]] void raise(std::string_view message);
	}

	struct FormatOptions
	{
		bool pretty = true;
		std::size_t indentationSize = 4;
	};

	struct ParseOptions
	{
		bool rejectDuplicateKeys = true;
		bool allowUtf8Bom = true;
		std::size_t maxDepth = 512;
	};

	class Value
	{
	public:
		using Members = std::map<std::string, Value, std::less<>>;
		using Array = std::vector<Value>;

		enum class Type
		{
			Null,
			Boolean,
			Integer,
			Floating,
			String,
			Object,
			Array
		};

	private:
		using Storage = std::variant<
			std::nullptr_t,
			bool,
			std::int64_t,
			double,
			std::string,
			Members,
			Array>;

		Storage _storage = nullptr;

	public:
		Value() = default;
		Value(std::nullptr_t);
		Value(bool value);
		Value(const char *value);
		Value(std::string value);
		Value(std::string_view value);

		template <native_integer T>
		Value(T value)
		{
			set(value);
		}

		template <native_floating T>
		Value(T value)
		{
			set(value);
		}

		template <json_writable T>
		Value(const T &value)
		{
			set(value);
		}

		static Value null();
		static Value object();
		static Value array();

		static Value fromString(std::string_view content, const ParseOptions &options = {});
		static Value loadFromFile(const std::filesystem::path &path, const ParseOptions &options = {});

		void saveToFile(const std::filesystem::path &path, const FormatOptions &options = {}) const;
		std::string toString(const FormatOptions &options = {}) const;

		void reset();

		Type type() const;
		bool isNull() const;
		bool isBoolean() const;
		bool isInteger() const;
		bool isFloating() const;
		bool isNumber() const;
		bool isString() const;
		bool isObject() const;
		bool isArray() const;

		void resetToNull();
		void resetToObject();
		void resetToArray();

		Members &asObject();
		const Members &asObject() const;

		Array &asArray();
		const Array &asArray() const;

		bool contains(std::string_view key) const;
		std::size_t count(std::string_view key) const;

		Value *find(std::string_view key);
		const Value *find(std::string_view key) const;

		Value &at(std::string_view key);
		const Value &at(std::string_view key) const;

		Value &at(std::size_t index);
		const Value &at(std::size_t index) const;

		Value &operator[](std::string_view key);
		const Value &operator[](std::string_view key) const;

		Value &operator[](std::size_t index);
		const Value &operator[](std::size_t index) const;

		Value &append();
		Value &pushBack(Value value);
		void resize(std::size_t size);
		std::size_t size() const;
		bool empty() const;

		Value &operator=(std::nullptr_t);
		Value &operator=(bool value);
		Value &operator=(const char *value);
		Value &operator=(std::string value);
		Value &operator=(std::string_view value);

		template <native_integer T>
		Value &operator=(T value)
		{
			set(value);
			return *this;
		}

		template <native_floating T>
		Value &operator=(T value)
		{
			set(value);
			return *this;
		}

		template <json_writable T>
		Value &operator=(const T &value)
		{
			set(value);
			return *this;
		}

		void set(std::nullptr_t);
		void set(bool value);
		void set(const char *value);
		void set(std::string value);
		void set(std::string_view value);

		template <native_integer T>
		void set(T value)
		{
			if constexpr (std::signed_integral<T>)
			{
				_storage = static_cast<std::int64_t>(value);
			}
			else
			{
				if (value > static_cast<std::make_unsigned_t<std::int64_t>>(std::numeric_limits<std::int64_t>::max()))
				{
					detail::raise("Unsigned integer value is too large for JSON integer storage");
				}
				_storage = static_cast<std::int64_t>(value);
			}
		}

		template <native_floating T>
		void set(T value)
		{
			const double castedValue = static_cast<double>(value);
			if (!std::isfinite(castedValue))
			{
				detail::raise("JSON floating value must be finite");
			}
			_storage = castedValue;
		}

		template <json_writable T>
		void set(const T &value)
		{
			Value serialized;

			if constexpr (member_json_writable<T>)
			{
				serialized = value.toJSON();
			}
			else
			{
				serialized = toJSON(value);
			}

			_storage = std::move(serialized._storage);
		}

		template <typename T>
		bool canAs() const
		{
			using CleanType = std::remove_cvref_t<T>;

			if constexpr (std::same_as<CleanType, std::nullptr_t>)
			{
				return isNull();
			}
			else if constexpr (std::same_as<CleanType, bool>)
			{
				return isBoolean();
			}
			else if constexpr (native_integer<CleanType>)
			{
				const std::int64_t *value = std::get_if<std::int64_t>(&_storage);
				if (value == nullptr)
				{
					return false;
				}
				if constexpr (std::signed_integral<CleanType>)
				{
					return *value >= static_cast<std::int64_t>(std::numeric_limits<CleanType>::min()) &&
						   *value <= static_cast<std::int64_t>(std::numeric_limits<CleanType>::max());
				}
				else
				{
					return *value >= 0 &&
						   static_cast<std::uint64_t>(*value) <= static_cast<std::uint64_t>(std::numeric_limits<CleanType>::max());
				}
			}
			else if constexpr (native_floating<CleanType>)
			{
				if (const double *value = std::get_if<double>(&_storage))
				{
					return *value >= static_cast<double>(std::numeric_limits<CleanType>::lowest()) &&
						   *value <= static_cast<double>(std::numeric_limits<CleanType>::max());
				}
				return isInteger();
			}
			else if constexpr (std::same_as<CleanType, std::string>)
			{
				return isString();
			}
			else
			{
				return false;
			}
		}

		template <typename T>
		bool holds() const
		{
			return canAs<T>();
		}

		template <typename T>
		std::remove_cvref_t<T> as() const
		{
			using CleanType = std::remove_cvref_t<T>;

			if constexpr (std::same_as<CleanType, std::nullptr_t>)
			{
				if (!isNull())
				{
					detail::raise("Wrong JSON type requested: expected null");
				}
				return nullptr;
			}
			else if constexpr (std::same_as<CleanType, bool>)
			{
				if (const bool *value = std::get_if<bool>(&_storage))
				{
					return *value;
				}
				detail::raise("Wrong JSON type requested: expected boolean");
			}
			else if constexpr (native_integer<CleanType>)
			{
				const std::int64_t *value = std::get_if<std::int64_t>(&_storage);
				if (value == nullptr)
				{
					detail::raise("Wrong JSON type requested: expected integer");
				}
				if (!canAs<CleanType>())
				{
					detail::raise("JSON integer value is outside of requested integer type range");
				}
				return static_cast<CleanType>(*value);
			}
			else if constexpr (native_floating<CleanType>)
			{
				if (const double *value = std::get_if<double>(&_storage))
				{
					if (!canAs<CleanType>())
					{
						detail::raise("JSON floating value is outside of requested floating type range");
					}
					return static_cast<CleanType>(*value);
				}
				if (const std::int64_t *value = std::get_if<std::int64_t>(&_storage))
				{
					return static_cast<CleanType>(*value);
				}
				detail::raise("Wrong JSON type requested: expected number");
			}
			else if constexpr (std::same_as<CleanType, std::string>)
			{
				if (const std::string *value = std::get_if<std::string>(&_storage))
				{
					return *value;
				}
				detail::raise("Wrong JSON type requested: expected string");
			}
			else if constexpr (member_json_readable<CleanType>)
			{
				return CleanType::fromJSON(*this);
			}
			else if constexpr (free_json_readable<CleanType>)
			{
				CleanType result{};
				fromJSON(*this, result);
				return result;
			}
			else
			{
				static_assert(sizeof(CleanType) == 0, "Type is not readable from spk::JSON::Value");
			}
		}

		void write(std::ostream &stream, const FormatOptions &options = {}) const;

		bool operator==(const Value &) const = default;

		friend std::ostream &operator<<(std::ostream &stream, const Value &value);
	};
}
