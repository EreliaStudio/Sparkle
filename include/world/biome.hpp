#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "exception.hpp"

namespace spk::World
{
	class Biome
	{
	public:
		class Registry;

		virtual ~Biome() = default;
	};

	// Owns polymorphic biomes. Configure before binding to a generator.
	// The registry must outlive all generators and cached biome references using it.
	class Biome::Registry
	{
	private:
		std::unordered_map<std::string, std::unique_ptr<Biome>> _biomes;

	public:
		Registry() = default;
		Registry(const Registry &) = delete;
		Registry &operator=(const Registry &) = delete;
		Registry(Registry &&) = delete;
		Registry &operator=(Registry &&) = delete;

		template <typename TBiome, typename... TArgs>
			requires std::derived_from<TBiome, Biome>
		const Biome *add(const std::string &name, TArgs &&...args)
		{
			if (_biomes.contains(name))
			{
				throw spk::Exception("Biome name is already registered");
			}
			auto biome = std::make_unique<TBiome>(std::forward<TArgs>(args)...);
			return _biomes.emplace(name, std::move(biome)).first->second.get();
		}

		[[nodiscard]] const Biome *get(const std::string &name) const
		{
			const auto it = _biomes.find(name);
			return it == _biomes.end() ? nullptr : it->second.get();
		}

		[[nodiscard]] bool contains(const std::string &name) const
		{
			return _biomes.contains(name);
		}

		[[nodiscard]] std::size_t size() const noexcept
		{
			return _biomes.size();
		}
	};
}
