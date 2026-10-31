#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "math/vector3.hpp"
#include "voxel/cell.hpp"

namespace spk::Voxel
{
	struct VolumeTestAccess;

	class Volume
	{
	private:
		struct Data;
		friend struct VolumeTestAccess;

	public:
		class Builder
		{
		private:
			friend struct VolumeTestAccess;
			spk::Vector3UInt _dimensions;
			std::vector<Definition> _definitions;
			std::vector<std::uint32_t> _cells;
			bool _consumed = false;

			void _ensureActive() const;
			[[nodiscard]] std::size_t _definitionIndex(const Definition &definition) const noexcept;

		public:
			explicit Builder(spk::Vector3UInt dimensions);
			explicit Builder(const Volume &volume);

			void set(spk::Vector3UInt position, Cell cell);
			void erase(spk::Vector3UInt position);
			[[nodiscard]] Volume build();
		};

	private:
		std::shared_ptr<const Data> _data;

		explicit Volume(std::shared_ptr<const Data> data) noexcept;

	public:
		[[nodiscard]] spk::Vector3UInt dimensions() const noexcept;
		[[nodiscard]] Cell cell(spk::Vector3UInt position) const;
	};
}
