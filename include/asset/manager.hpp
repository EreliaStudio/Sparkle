#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "asset/factory.hpp"
#include "container/json/object.hpp"

namespace spk::asset
{
	class Manager
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

			OwnedAsset(Manager &assets, const std::string &id, const JSON::Value &config) :
				value(Factory<TType>::create(assets, id, config))
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
			bool isDefault = false;
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
		[[nodiscard]] Asset *_getDefault(std::type_index type, const std::string &id) const;
		void _setDefault(const Identity &identity);
		void _checkCycle(const Identity &identity) const;
		void _publish(Identity identity, std::unique_ptr<Asset> asset, bool makeDefault);

	public:
		Manager() = default;
		~Manager();

		Manager(const Manager &) = delete;
		Manager &operator=(const Manager &) = delete;
		Manager(Manager &&) = delete;
		Manager &operator=(Manager &&) = delete;

		template <typename TType>
		[[nodiscard]] TType &load(const std::string &id, const JSON::Value &config, bool makeDefault = false)
		{
			static_assert(std::is_object_v<TType> && !std::is_const_v<TType> && !std::is_volatile_v<TType>);
			static_assert(
				requires(Manager &assets, const std::string &name, const JSON::Value &settings) {
					{ Factory<TType>::create(assets, name, settings) } -> std::same_as<TType>;
				},
				"Factory<T> must provide static T create(Manager&, const std::string&, const JSON::Value&)");

			Identity identity{typeid(TType), id, config};
			if (Asset *existing = _find(identity))
			{
				if (makeDefault)
				{
					_setDefault(identity);
				}
				return static_cast<OwnedAsset<TType> &>(*existing).value;
			}

			_checkCycle(identity);
			ActiveLoad active(_active, identity);
			auto owned = std::make_unique<OwnedAsset<TType>>(*this, identity.id, identity.config);
			TType &result = owned->value;
			_publish(std::move(identity), std::move(owned), makeDefault);
			return result;
		}

		template <typename TType>
		[[nodiscard]] TType &load(const std::string &id)
		{
			return load<TType>(id, JSON::Value::object());
		}

		template <typename TType>
		[[nodiscard]] TType &get(const std::string &id)
		{
			static_assert(std::is_object_v<TType> && !std::is_const_v<TType> && !std::is_volatile_v<TType>);
			return static_cast<OwnedAsset<TType> &>(*_getDefault(typeid(TType), id)).value;
		}

		template <typename TType>
		void setDefault(const std::string &id, const JSON::Value &config)
		{
			static_assert(std::is_object_v<TType> && !std::is_const_v<TType> && !std::is_volatile_v<TType>);
			_setDefault(Identity{typeid(TType), id, config});
		}

		template <typename TType>
		void setDefault(const std::string &id)
		{
			setDefault<TType>(id, JSON::Value::object());
		}

		void clear();
	};
}

namespace spk
{
	using Assets = asset::Manager;
}
