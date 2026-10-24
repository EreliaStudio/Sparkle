#include "container/json/reader.hpp"

#include <unordered_set>

#include "exception.hpp"

namespace spk::JSON
{
	spk::JSON::Value Loader::parseFile(const std::filesystem::path &file)
	{
		try
		{
			return spk::JSON::Value::loadFromFile(file);
		} catch (...)
		{
			throw spk::Exception(
				detail::makeErrorMessage(file, "$", "invalid JSON"),
				std::current_exception());
		}
	}

	Reader::Reader(const spk::JSON::Value &value, std::filesystem::path file, std::string path) :
		_value(value),
		_file(std::move(file)),
		_path(std::move(path))
	{
	}

	const std::filesystem::path &Reader::file() const noexcept
	{
		return _file;
	}

	const std::string &Reader::path() const noexcept
	{
		return _path;
	}

	const spk::JSON::Value &Reader::value() const noexcept
	{
		return _value;
	}

	std::string Reader::pathFor(const std::string &key) const
	{
		return _path + "." + key;
	}

	bool Reader::contains(const std::string &key) const
	{
		_requireObject();
		return _value.contains(key);
	}

	void Reader::_requireObject() const
	{
		if (!_value.isObject())
		{
			throw spk::Exception(detail::makeErrorMessage(_file, _path, "expected an object"));
		}
	}

	const spk::JSON::Value &Reader::_requireMember(const std::string &key) const
	{
		_requireObject();
		const spk::JSON::Value *member = _value.find(key);
		if (member == nullptr)
		{
			throw spk::Exception(detail::makeErrorMessage(_file, pathFor(key), "missing required field"));
		}
		return *member;
	}

	Reader Reader::child(const std::string &key) const
	{
		const spk::JSON::Value &member = _requireMember(key);
		if (!member.isObject())
		{
			throw spk::Exception(detail::makeErrorMessage(_file, pathFor(key), "expected an object"));
		}
		return Reader(member, _file, pathFor(key));
	}

	std::vector<Reader> Reader::childArray(const std::string &key) const
	{
		const spk::JSON::Value &member = _requireMember(key);
		if (!member.isArray())
		{
			throw spk::Exception(detail::makeErrorMessage(_file, pathFor(key), "expected an array"));
		}

		const spk::JSON::Value::Array &array = member.asArray();
		std::vector<Reader> result;
		result.reserve(array.size());
		for (std::size_t index = 0; index < array.size(); ++index)
		{
			const std::string elementPath = pathFor(key) + "[" + std::to_string(index) + "]";
			result.emplace_back(array[index], _file, elementPath);
		}
		return result;
	}

	void Reader::forbidUnknown(std::initializer_list<std::string_view> allowedKeys) const
	{
		_requireObject();
		const std::unordered_set<std::string_view> unorderedAllowedKeys(allowedKeys);
		for (const auto &[key, unused] : _value.asObject())
		{
			(void)unused;
			if (!unorderedAllowedKeys.contains(key))
			{
				throw spk::Exception(detail::makeErrorMessage(_file, pathFor(key), "unknown field"));
			}
		}
	}
}
