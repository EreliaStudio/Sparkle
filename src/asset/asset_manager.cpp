#include "asset/asset_manager.hpp"

#include <algorithm>
#include <utility>

#include "exception.hpp"

namespace spk
{
	AssetManager::Asset::~Asset() = default;

	AssetManager::ActiveLoad::ActiveLoad(std::vector<Identity> &active, const Identity &identity) :
		_active(active)
	{
		_active.push_back(identity);
	}

	AssetManager::ActiveLoad::~ActiveLoad()
	{
		_active.pop_back();
	}

	AssetManager::~AssetManager() = default;

	std::string AssetManager::_describe(const Identity &identity)
	{
		return std::string(identity.type.name()) + " '" + identity.id + "' " +
			   identity.config.toString({.pretty = false});
	}

	std::string AssetManager::_describeCycle(const Identity &identity) const
	{
		std::string description = "Asset dependency cycle: ";
		const auto first = std::find(_active.begin(), _active.end(), identity);

		for (auto current = first; current != _active.end(); ++current)
		{
			description += _describe(*current) + " -> ";
		}

		return description + _describe(identity);
	}

	AssetManager::Asset *AssetManager::_find(const Identity &identity) const
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

	void AssetManager::_checkCycle(const Identity &identity) const
	{
		if (std::find(_active.begin(), _active.end(), identity) != _active.end())
		{
			throw spk::Exception(_describeCycle(identity));
		}
	}

	void AssetManager::_publish(Identity identity, std::unique_ptr<Asset> asset)
	{
		_assets[identity.type][identity.id].push_back(Entry{std::move(identity.config), std::move(asset)});
	}

	void AssetManager::clear()
	{
		_assets.clear();
	}
}
