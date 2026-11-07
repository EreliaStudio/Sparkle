#include "structure/assembly.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace spk::Assembly
{
	struct Blueprint::Data
	{
		std::unique_ptr<Blueprint::Element> root;
		std::unordered_map<std::string, const Blueprint::Element *> elements;
	};

	namespace
	{
		void collectElements(const Blueprint::Element &element, std::unordered_map<std::string, const Blueprint::Element *> &elements)
		{
			const auto [it, inserted] = elements.emplace(element.name(), &element);
			if (!inserted)
			{
				throw spk::Exception("Duplicate Blueprint::Element name '" + element.name() + "' in Blueprint");
			}
			std::unordered_set<std::string> attachmentPointNames;
			for (const auto &attachmentPoint : element.attachmentPoints())
			{
				if (!attachmentPointNames.insert(attachmentPoint.name()).second)
				{
					throw spk::Exception("Duplicate attachment point name '" + attachmentPoint.name() + "' in Blueprint::Element '" + element.name() + "'");
				}
				if (const Blueprint::Element *child = attachmentPoint.child())
				{
					collectElements(*child, elements);
				}
			}
		}
	}

	Blueprint::Blueprint(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	const Blueprint::Element &Blueprint::root() const
	{
		if (!_data || !_data->root)
		{
			throw spk::Exception("Blueprint has no root");
		}
		return *_data->root;
	}

	const Blueprint::Element &Blueprint::_element(const std::string &name) const
	{
		if (!_data)
		{
			throw spk::Exception("Blueprint has no data");
		}
		const auto it = _data->elements.find(name);
		if (it == _data->elements.end())
		{
			throw spk::Exception("Unknown Blueprint::Element '" + name + "' in Blueprint");
		}
		return *it->second;
	}

	const Matrix4x4 &Blueprint::attachmentPoint(
		const std::string &elementName,
		const std::string &attachmentName) const
	{
		return _element(elementName).attachmentPoint(attachmentName).transform();
	}

	Blueprint::Builder::Builder() :
		_data(std::make_shared<Data>())
	{
	}

	void Blueprint::Builder::_ensureEmpty() const
	{
		if (!_data)
		{
			throw spk::Exception("Blueprint Builder has already been consumed");
		}
		if (_data->root)
		{
			throw spk::Exception("Blueprint Builder already has a root");
		}
	}

	void Blueprint::Builder::_setRoot(std::unique_ptr<Blueprint::Element> root)
	{
		_ensureEmpty();
		_data->root = std::move(root);
	}

	Blueprint::Element &Blueprint::Builder::root()
	{
		if (!_data)
		{
			throw spk::Exception("Blueprint Builder has already been consumed");
		}
		if (!_data->root)
		{
			throw spk::Exception("Blueprint Builder has no root");
		}
		return *_data->root;
	}

	Blueprint Blueprint::Builder::build()
	{
		Blueprint::Element &element = root();
		_data->elements.clear();
		collectElements(element, _data->elements);
		std::shared_ptr<const Data> published = std::move(_data);
		return Blueprint(std::move(published));
	}
}
