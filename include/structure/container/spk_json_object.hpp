#pragma once

#include <iostream>
#include <variant>
#include <map>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <any>
#include <cstdint>

#include "utils/spk_string_utils.hpp"

namespace spk
{
	namespace JSON
	{
		class Object
		{
		public:
			using Unit = std::variant<bool, long, double, std::wstring, Object*, std::nullptr_t>;
			using ContentType = std::variant<Unit, std::map<std::wstring, Object*>, std::vector<Object*>>;

		private:
			bool _initialized;
			std::wstring _name;
			ContentType _content;
			static size_t _indent;
			const static uint8_t _indentSize = 4;

			void printUnit(std::wostream& p_os) const;
			void printObject(std::wostream& p_os) const;
			void printArray(std::wostream& p_os) const;

		public:
			Object(const std::wstring& p_name = L"Unnamed");

			void reset();

			bool isObject() const;

			bool isArray() const;

			bool isUnit() const;

			Object& addAttribute(const std::wstring& p_key);

			const std::map<std::wstring, Object*>& members() const;

			bool contains(const std::wstring& p_key) const;

			Object& operator[](const std::wstring& p_key);

			const Object& operator[](const std::wstring& p_key) const;

			void setAsObject();

			const std::vector<Object*>& asArray() const;

			void resize(size_t p_size);

			Object& append();

			void push_back(Object& p_object);

			Object& operator[](size_t p_index);

			const Object& operator[](size_t p_index) const;

			void setAsArray();

			size_t size() const;

			size_t count(const std::wstring& p_key) const;

			template <typename TType>
			bool hold() const
			{
				if (isUnit() == false)
					throw std::runtime_error("Can't verify the holding type of an object who isn't a Unit");
				return (std::holds_alternative<TType>(std::get<Unit>(_content)));
			}

			template <typename TType,
				typename std::enable_if<!std::is_same<TType, Object>::value, int>::type = 0>
			void set(const TType& p_value)
			{
				if (_initialized == false)
				{
					_content = Unit();
					_initialized = true;
				}

				std::get<Unit>(_content) = p_value;
			}

			template <typename TType,
				typename std::enable_if<std::is_same<TType, Object>::value, int>::type = 0>
			void set(const TType& p_value)
			{
				Object* tmpObject = new Object(p_value);
				set<Object*>(tmpObject);
			}

			template <typename TType>
			const TType& as() const
			{
				const Unit& unit = std::get<Unit>(_content);
				const TType* value = std::get_if<TType>(&unit);

				if (value == nullptr)
				{
					Unit tmpUnit = TType();
					std::string types[] = { "bool", "long", "double", "std::wstring", "Object*", "std::nullptr_t" };
					throw std::runtime_error("Wrong type request for object [" + spk::StringUtils::wstringToString(_name) + "] as Unit : Request type [" + types[tmpUnit.index()] + "] but unit contain [" + types[unit.index()] + "]");
				}

				return (*value);
			}

			template<typename TType>
			Object& operator=(const TType& value)
			{
				this->set<TType>(value);
				return (*this);
			}

			friend std::wostream& operator<<(std::wostream& p_os, const Object& p_object);
		};
	}
}