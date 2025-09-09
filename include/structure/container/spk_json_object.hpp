#pragma once

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <variant>
#include <vector>

#include "spk_sfinae.hpp"
#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	namespace JSON
	{
		class Object
		{
		public:
			using Unit = std::variant<bool, long, double, std::wstring, std::shared_ptr<Object>, std::nullptr_t>;
			using ContentType = std::variant<Unit, std::map<std::wstring, std::shared_ptr<Object>>, std::vector<std::shared_ptr<Object>>>;

		private:
			bool _initialized;
			std::wstring _name;
			ContentType _content;
			static size_t _indent;
			const static uint8_t _indentSize = 4;

			void _printUnit(std::wostream &p_os) const;
			void _printObject(std::wostream &p_os) const;
			void _printArray(std::wostream &p_os) const;

		public:
			Object(const std::wstring &p_name = L"Unnamed");

			static Object fromString(const std::wstring &p_content);

			void reset();

			bool isObject() const;

			bool isArray() const;

			bool isUnit() const;

			Object &addAttribute(const std::wstring &p_key);

			const std::map<std::wstring, std::shared_ptr<Object>> &members() const;

			bool contains(const std::wstring &p_key) const;

			Object &operator[](const std::wstring &p_key);

			const Object &operator[](const std::wstring &p_key) const;

			void setAsObject();

			const std::vector<std::shared_ptr<Object>> &asArray() const;

			void resize(size_t p_size);

			Object &append();

			void pushBack(const Object &p_object);

			Object &operator[](size_t p_index);

			const Object &operator[](size_t p_index) const;

			void setAsArray();

			size_t size() const;

			size_t count(const std::wstring &p_key) const;

			template <typename TType>
			bool hold() const
			{
				if (isUnit() == false)
				{
					GENERATE_ERROR("Can't verify the holding type of an object who isn't a Unit");
				}
				return (std::holds_alternative<TType>(std::get<Unit>(_content)));
			}

			template <
				typename TType,
				std::enable_if_t<!std::is_same_v<std::decay_t<TType>, Object> && !spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
			void set(const TType &p_value)
			{
				if (_initialized == false)
				{
					_content = Unit();
					_initialized = true;
				}
				std::get<Unit>(_content) = p_value;
			}

			template <typename TType, std::enable_if_t<std::is_same_v<std::decay_t<TType>, Object>, int> = 0>
			void set(const TType &p_value)
			{
				set<std::shared_ptr<Object>>(std::make_unique<Object>(p_value));
			}

			template <typename TType, std::enable_if_t<spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
			void set(const TType &p_value)
			{
				set<std::shared_ptr<Object>>(std::make_unique<Object>(p_value.toJSON()));
			}

			template <typename TType, std::enable_if_t<!spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
			Object &operator=(const TType &p_rhs)
			{
				set(p_rhs);
				return *this;
			}

			template <typename TType, std::enable_if_t<spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
			Object &operator=(const TType &p_rhs) // JSON-able objects
			{
				set(p_rhs);
				return *this;
			}

			template <typename TType, std::enable_if_t<!spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
			const TType &as() const
			{
				if (isUnit() == false)
				{
					GENERATE_ERROR("Can't request a Unit from a non-Unit node");
				}

				const Unit &tmpUnit = std::get<Unit>(_content);
				const TType *value = std::get_if<TType>(&tmpUnit);
				if (value == nullptr)
				{
					std::array<std::string, 6> types = {"bool", "long", "double", "std::wstring", "Object*", "std::nullptr_t"};
					GENERATE_ERROR(
						"Wrong type request for object [" + spk::StringUtils::wstringToString(_name) + "] : wanted [" + types[Unit(TType()).index()] +
						"] but stored [" + types[tmpUnit.index()] + "]");
				}
				return *value;
			}

			template <typename TType, std::enable_if_t<spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
			TType as() const
			{
				const Object *src = nullptr;

				if (isUnit())
				{
					const Unit &tmpUnit = std::get<Unit>(_content);

					if (const auto *uptr = std::get_if<std::shared_ptr<Object>>(&tmpUnit))
					{
						src = uptr->get();
					}
				}
				else if (isObject())
				{
					src = this;
				}

				if (src == nullptr)
				{
					GENERATE_ERROR(
						"Cannot convert object [" + spk::StringUtils::wstringToString(_name) + "] to native type -> no JSON object found.");
				}

				TType result{};
				result.fromJSON(*src);
				return result;
			}

			friend std::wostream &operator<<(std::wostream &p_os, const Object &p_object);
			friend std::ostream &operator<<(std::ostream &p_os, const Object &p_object);
		};
	}
}