#include "structure/skeleton.hpp"

#include <unordered_map>
#include <utility>

namespace spk
{
	struct Skeleton::Data
	{
		std::unique_ptr<Part> root;
		std::unordered_map<std::string, const Part *> parts;
	};

	namespace
	{
		void collectParts(const Part &part, std::unordered_map<std::string, const Part *> &parts)
		{
			const auto [it, inserted] = parts.emplace(part.name(), &part);
			if (!inserted)
			{
				throw spk::Exception("Duplicate Part name '" + part.name() + "' in Skeleton");
			}
			for (const auto &attachment : part.attachments())
			{
				if (const Part *child = attachment.child())
				{
					collectParts(*child, parts);
				}
			}
		}
	}

	Skeleton::Skeleton(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	const Part &Skeleton::root() const
	{
		if (!_data || !_data->root)
		{
			throw spk::Exception("Skeleton has no root");
		}
		return *_data->root;
	}

	const Part &Skeleton::_part(const std::string &name) const
	{
		if (!_data)
		{
			throw spk::Exception("Skeleton has no data");
		}
		const auto it = _data->parts.find(name);
		if (it == _data->parts.end())
		{
			throw spk::Exception("Unknown Part '" + name + "' in Skeleton");
		}
		return *it->second;
	}

	const Matrix4x4 &Skeleton::attachmentPoint(
		const std::string &partName,
		const std::string &attachmentName) const
	{
		return _part(partName).attachment(attachmentName).transform();
	}

	Skeleton::Builder::Builder() :
		_data(std::make_shared<Data>())
	{
	}

	void Skeleton::Builder::_ensureEmpty() const
	{
		if (!_data)
		{
			throw spk::Exception("Skeleton Builder has already been consumed");
		}
		if (_data->root)
		{
			throw spk::Exception("Skeleton Builder already has a root");
		}
	}

	void Skeleton::Builder::_setRoot(std::unique_ptr<Part> root)
	{
		_ensureEmpty();
		_data->root = std::move(root);
	}

	Part &Skeleton::Builder::root()
	{
		if (!_data)
		{
			throw spk::Exception("Skeleton Builder has already been consumed");
		}
		if (!_data->root)
		{
			throw spk::Exception("Skeleton Builder has no root");
		}
		return *_data->root;
	}

	Skeleton Skeleton::Builder::build()
	{
		Part &part = root();
		_data->parts.clear();
		collectParts(part, _data->parts);
		std::shared_ptr<const Data> published = std::move(_data);
		return Skeleton(std::move(published));
	}
}
