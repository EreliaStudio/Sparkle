#include "asset/manager.hpp"

#include <algorithm>
#include <utility>

#include "exception.hpp"

namespace spk::asset
{
	Manager::Asset::~Asset() = default;

	Manager::ActiveLoad::ActiveLoad(std::vector<Identity> &active, const Identity &identity) :
		_active(active)
	{
		_active.push_back(identity);
	}

	Manager::ActiveLoad::~ActiveLoad()
	{
		_active.pop_back();
	}

	Manager::~Manager() = default;

	std::string Manager::_describe(const Identity &identity)
	{
		return std::string(identity.type.name()) + " '" + identity.id + "' " +
			   identity.config.toString({.pretty = false});
	}

	std::string Manager::_describeCycle(const Identity &identity) const
	{
		std::string description = "Asset dependency cycle: ";
		const auto first = std::find(_active.begin(), _active.end(), identity);

		for (auto current = first; current != _active.end(); ++current)
		{
			description += _describe(*current) + " -> ";
		}

		return description + _describe(identity);
	}

	Manager::Asset *Manager::_find(const Identity &identity) const
	{
		const auto type = _assets.find(identity.type);
		if (type == _assets.end())
		{
			return nullptr;
		}

		const auto id = type->second.find(identity.id);
		if (id == type->second.end())
		{
			return nullptr;
		}

		for (const Entry &entry : id->second)
		{
			if (entry.config == identity.config)
			{
				return entry.asset.get();
			}
		}
		return nullptr;
	}

	Manager::Asset *Manager::_getUnique(std::type_index type, const std::string &id) const
	{
		const auto family = _assets.find(type);
		if (family == _assets.end() || !family->second.contains(id))
		{
			throw spk::Exception("Asset not loaded: " + std::string(type.name()) + " '" + id + "'");
		}

		const Entries &entries = family->second.at(id);
		if (entries.size() != 1)
		{
			throw spk::Exception("Ambiguous asset: " + std::string(type.name()) + " '" + id + "'");
		}
		return entries.front().asset.get();
	}

	void Manager::_checkCycle(const Identity &identity) const
	{
		if (std::find(_active.begin(), _active.end(), identity) != _active.end())
		{
			throw spk::Exception(_describeCycle(identity));
		}
	}

	void Manager::_publish(Identity identity, std::unique_ptr<Asset> asset)
	{
		_assets[identity.type][identity.id].push_back(Entry{std::move(identity.config), std::move(asset)});
	}

	void Manager::clear()
	{
		_assets.clear();
	}
}
