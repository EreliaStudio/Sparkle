#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "voxel/definition.hpp"

namespace spk::Voxel
{
	class DefinitionRegistry
	{
	private:
		std::unordered_map<std::uint32_t, Definition> _definitions;

	public:
		void add(std::uint32_t id, Definition definition);
		[[nodiscard]] const Definition &at(std::uint32_t id) const;
		[[nodiscard]] bool contains(std::uint32_t id) const noexcept;
		[[nodiscard]] std::size_t size() const noexcept;
	};
}
