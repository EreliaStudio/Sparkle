#include "structure/assembly.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace spk
{
	struct Assembly::Data
	{
		std::unique_ptr<Assembly::Element> root;
		std::unordered_map<std::string, const Assembly::Element *> elements;
	};

	namespace
	{
		void collectElements(const Assembly::Element &element, std::unordered_map<std::string, const Assembly::Element *> &elements)
		{
			const auto [it, inserted] = elements.emplace(element.name(), &element);
			if (!inserted)
			{
				throw spk::Exception("Duplicate Assembly::Element name '" + element.name() + "' in Assembly");
			}
			std::unordered_set<std::string> attachmentPointNames;
			for (const auto &attachmentPoint : element.attachmentPoints())
			{
				if (!attachmentPointNames.insert(attachmentPoint.name()).second)
				{
					throw spk::Exception("Duplicate attachmentPoint name '" + attachmentPoint.name() + "' in Assembly::Element '" + element.name() + "'");
				}
				if (const Assembly::Element *child = attachmentPoint.child())
				{
					collectElements(*child, elements);
				}
			}
		}
	}

	Assembly::Assembly(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	const Assembly::Element &Assembly::root() const
	{
		if (!_data || !_data->root)
		{
			throw spk::Exception("Assembly has no root");
		}
		return *_data->root;
	}

	const Assembly::Element &Assembly::_element(const std::string &name) const
	{
		if (!_data)
		{
			throw spk::Exception("Assembly has no data");
		}
		const auto it = _data->elements.find(name);
		if (it == _data->elements.end())
		{
			throw spk::Exception("Unknown Assembly::Element '" + name + "' in Assembly");
		}
		return *it->second;
	}

	const Matrix4x4 &Assembly::attachmentPoint(
		const std::string &elementName,
		const std::string &attachmentName) const
	{
		return _element(elementName).attachmentPoint(attachmentName).transform();
	}

	Assembly::Builder::Builder() :
		_data(std::make_shared<Data>())
	{
	}

	void Assembly::Builder::_ensureEmpty() const
	{
		if (!_data)
		{
			throw spk::Exception("Assembly Builder has already been consumed");
		}
		if (_data->root)
		{
			throw spk::Exception("Assembly Builder already has a root");
		}
	}

	void Assembly::Builder::_setRoot(std::unique_ptr<Assembly::Element> root)
	{
		_ensureEmpty();
		_data->root = std::move(root);
	}

	Assembly::Element &Assembly::Builder::root()
	{
		if (!_data)
		{
			throw spk::Exception("Assembly Builder has already been consumed");
		}
		if (!_data->root)
		{
			throw spk::Exception("Assembly Builder has no root");
		}
		return *_data->root;
	}

	Assembly Assembly::Builder::build()
	{
		Assembly::Element &element = root();
		_data->elements.clear();
		collectElements(element, _data->elements);
		std::shared_ptr<const Data> published = std::move(_data);
		return Assembly(std::move(published));
	}
}
