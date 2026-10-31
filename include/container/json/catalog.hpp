#pragma once

#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

#include <container/json/error.hpp>
#include <container/json/reader.hpp>
#include <exception.hpp>

namespace spk::JSON
{
	template <typename TElement, typename TID = typename TElement::ID>
	class Catalog
	{
	public:
		using Element = TElement;
		using ID = TID;

	private:
		std::unordered_map<ID, Element> _elements;

		void _parseAndInsert(const Reader &elementReader)
		{
			elementReader.forbidUnknown({"id", "data"});

			ID id = _parseKey(elementReader);
			if (contains(id))
			{
				spk::JSON::throwAt(
					elementReader.file(),
					elementReader.pathFor("id"),
					"duplicate catalog ID");
			}

			const Reader dataReader = elementReader.child("data");
			Element element = _parseElement(dataReader);
			_elements.emplace(std::move(id), std::move(element));
		}

	protected:
		Catalog() = default;
		virtual ~Catalog() = default;

		[[nodiscard]] virtual ID _parseKey(const Reader &reader) const = 0;
		[[nodiscard]] virtual Element _parseElement(const Reader &reader) const = 0;

		[[nodiscard]] const std::unordered_map<ID, Element> &_entries() const noexcept
		{
			return _elements;
		}

		void _insert(ID id, Element element)
		{
			if (contains(id))
			{
				throw spk::Exception("duplicate JSON catalog ID");
			}
			_elements.emplace(std::move(id), std::move(element));
		}

	public:
		void load(const std::filesystem::path &file)
		{
			const Value document = Loader::parseFile(file);
			const Reader root(document, file);

			if (!root.contains("elements"))
			{
				_parseAndInsert(root);
				return;
			}

			root.forbidUnknown({"elements"});
			const Value &elements = root.value().at("elements");
			if (!elements.isArray())
			{
				spk::JSON::throwAt(file, root.pathFor("elements"), "expected an array");
			}

			const auto &array = elements.asArray();
			for (std::size_t index = 0; index < array.size(); ++index)
			{
				const std::string path = root.pathFor("elements") + "[" + std::to_string(index) + "]";
				_parseAndInsert(Reader(array[index], file, path));
			}
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _elements.size();
		}

		[[nodiscard]] static std::string _describeID(const ID &id)
		{
			if constexpr (requires(std::ostringstream &stream, const ID &value) { stream << value; })
			{
				std::ostringstream stream;
				stream << id;
				return stream.str();
			}
			return "<unprintable>";
		}

		[[nodiscard]] const Element &at(const ID &id) const
		{
			const auto found = _elements.find(id);
			if (found == _elements.end())
			{
				throw spk::Exception("unknown JSON catalog ID: " + _describeID(id));
			}
			return found->second;
		}

		[[nodiscard]] const Element &operator[](const ID &id) const
		{
			return at(id);
		}

		[[nodiscard]] bool contains(const ID &id) const noexcept
		{
			return _elements.contains(id);
		}

		[[nodiscard]] const Element *tryGet(const ID &id) const noexcept
		{
			const auto found = _elements.find(id);
			return found == _elements.end() ? nullptr : &found->second;
		}
	};
}
