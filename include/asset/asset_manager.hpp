#pragma once

#include "container/json/object.hpp"
#include <concepts>
#include <memory>
#include <string>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace spk
{
	class AssetManager;

	template <typename TType>
	struct AssetFactory;

	class AssetManager
	{
	private:
		struct Asset
		{
			virtual ~Asset();
		};

		template <typename TType>
		struct OwnedAsset : Asset
		{
			TType value;

			OwnedAsset(AssetManager &assets, const std::string &id, const JSON::Value &config) :
				value(AssetFactory<TType>::create(assets, id, config))
			{
			}
		};

		struct Identity
		{
			std::type_index type;
			std::string id;
			JSON::Value config;

			[[nodiscard]] bool operator==(const Identity &) const = default;
		};

		struct Entry
		{
			JSON::Value config;
			std::unique_ptr<Asset> asset;
		};

		class ActiveLoad
		{
		private:
			std::vector<Identity> &_active;

		public:
			ActiveLoad(std::vector<Identity> &active, const Identity &identity);
			~ActiveLoad();

			ActiveLoad(const ActiveLoad &) = delete;
			ActiveLoad &operator=(const ActiveLoad &) = delete;
		};

		using Entries = std::vector<Entry>;
		using AssetsById = std::unordered_map<std::string, Entries>;

		std::unordered_map<std::type_index, AssetsById> _assets;
		std::vector<Identity> _active;

		[[nodiscard]] static std::string _describe(const Identity &identity);
		[[nodiscard]] std::string _describeCycle(const Identity &identity) const;
		[[nodiscard]] Asset *_find(const Identity &identity) const;
		void _checkCycle(const Identity &identity) const;
		void _publish(Identity identity, std::unique_ptr<Asset> asset);

	public:
		AssetManager() = default;
		~AssetManager();

		AssetManager(const AssetManager &) = delete;
		AssetManager &operator=(const AssetManager &) = delete;
		AssetManager(AssetManager &&) = delete;
		AssetManager &operator=(AssetManager &&) = delete;

		template <typename TType>
		[[nodiscard]] TType &load(const std::string &id, const JSON::Value &config)
		{
			static_assert(std::is_object_v<TType> && !std::is_const_v<TType> && !std::is_volatile_v<TType>);
			static_assert(
				requires(AssetManager &assets, const std::string &name, const JSON::Value &settings) {
					{ AssetFactory<TType>::create(assets, name, settings) } -> std::same_as<TType>;
				},
				"AssetFactory<T> must provide static T create(AssetManager&, const std::string&, const JSON::Value&)");

			Identity identity{typeid(TType), id, config};
			if (Asset *existing = _find(identity))
			{
				return static_cast<OwnedAsset<TType> &>(*existing).value;
			}

			_checkCycle(identity);
			ActiveLoad active(_active, identity);
			auto owned = std::make_unique<OwnedAsset<TType>>(*this, identity.id, identity.config);
			TType &result = owned->value;
			_publish(std::move(identity), std::move(owned));
			return result;
		}

		template <typename TType>
		[[nodiscard]] TType &load(const std::string &id)
		{
			return load<TType>(id, JSON::Value::object());
		}

		void clear();
	};
}
