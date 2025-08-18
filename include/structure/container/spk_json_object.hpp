#pragma once

#include "spk_sfinae.hpp"
#include "structure/system/spk_exception.hpp"
#include "utils/spk_string_utils.hpp"

#include <algorithm>
#include <any>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <variant>
#include <vector>

namespace spk
{
namespace JSON
{
class Object
{
  public:
using Unit = std::variant<bool, int32_t, double, std::wstring, std::unique_ptr<Object>, std::nullptr_t>;
using ContentType = std::variant<Unit, std::map<std::wstring, std::unique_ptr<Object>>, std::vector<std::unique_ptr<Object>>>;

  private:
bool _initialized;
std::wstring _name;
ContentType _content;
static size_t _indent;
const static uint8_t _indentSize = 4;

void printUnit(std::wostream &p_os) const;
void printObject(std::wostream &p_os) const;
void printArray(std::wostream &p_os) const;

  public:
Object(const std::wstring &p_name = L"Unnamed");

static Object fromString(const std::wstring &p_content);

void reset();

bool isObject() const;

bool isArray() const;

bool isUnit() const;

Object &addAttribute(const std::wstring &p_key);

const std::map<std::wstring, std::unique_ptr<Object>> &members() const;

bool contains(const std::wstring &p_key) const;

Object &operator[](const std::wstring &p_key);

const Object &operator[](const std::wstring &p_key) const;

void setAsObject();

const std::vector<std::unique_ptr<Object>> &asArray() const;

void resize(size_t p_size);

Object &append();

void push_back(Object &p_object);

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

template <typename TType,
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
std::unique_ptr<Object> tmp = std::make_unique<Object>(p_value);
set<std::unique_ptr<Object>>(tmp);
}

template <typename TType, std::enable_if_t<spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
void set(const TType &p_value)
{
std::unique_ptr<Object> nested = std::make_unique<Object>(p_value.toJSON());
set<std::unique_ptr<Object>>(nested);
}

template <typename TType, std::enable_if_t<!spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
Object &operator=(const TType &rhs)
{
set(rhs);
return *this;
}

template <typename TType, std::enable_if_t<spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
Object &operator=(const TType &rhs)
{
set(rhs);
return *this;
}

template <typename TType, std::enable_if_t<!spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
const TType &as() const
{
if (isUnit() == false)
{
GENERATE_ERROR("Can't request a Unit from a non-Unit node");
}

const Unit &unit = std::get<Unit>(_content);
const TType *value = std::get_if<TType>(&unit);
if (value == nullptr)
{
std::string types[] = {"bool", "int32_t", "double", "std::wstring", "Object*", "std::nullptr_t"};
GENERATE_ERROR("Wrong type request for object [" + spk::StringUtils::wstringToString(_name) + "] : wanted [" +
   types[Unit(TType()).index()] + "] but stored [" + types[unit.index()] + "]");
}
return *value;
}

template <typename TType, std::enable_if_t<spk::IsJSONable<std::decay_t<TType>>::value, int> = 0>
TType as() const
{
const Object *src = nullptr;

if (isUnit() == true)
{
const Unit &unit = std::get<Unit>(_content);
if (const auto *ptr = std::get_if<std::unique_ptr<Object>>(&unit))
{
src = ptr->get();
}
}
else if (isObject() == true)
{
src = this;
}

if (src == nullptr)
{
GENERATE_ERROR("Cannot convert object [" + spk::StringUtils::wstringToString(_name) +
   "] to native type -> no JSON object found.");
}

TType result{};
result.fromJSON(*src);
return result;
}

friend std::wostream &operator<<(std::wostream &p_os, const Object &p_object);
friend std::ostream &operator<<(std::ostream &p_os, const Object &p_object);
};
} // namespace JSON
} // namespace spk

