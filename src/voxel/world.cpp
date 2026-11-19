#include "voxel/world.hpp"

#include "exception.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <utility>

namespace spk::Voxel
{
	namespace
	{
		[[nodiscard]] std::uint32_t checkedCount(spk::Vector3UInt dimensions)
		{
			std::uint64_t count = 1;
			for (const auto value : std::array{dimensions.x, dimensions.y, dimensions.z})
			{
				if (value == 0 || value > World::MaxCellsPerChunk || count > World::MaxCellsPerChunk / value)
				{
					throw spk::Exception("Voxel World chunk dimensions are invalid or exceed 65536 cells");
				}
				count *= value;
			}
			return static_cast<std::uint32_t>(count);
		}

		[[nodiscard]] std::uint32_t offset(spk::Vector3UInt position, spk::Vector3UInt dimensions)
		{
			if (position.x >= dimensions.x || position.y >= dimensions.y || position.z >= dimensions.z)
			{
				throw spk::Exception("Voxel World cell edit is outside chunk bounds");
			}
			return (position.z * dimensions.y + position.y) * dimensions.x + position.x;
		}
	}

	World::World(spk::Vector3UInt dimensions, Definition::Registry definitions) :
		_dimensions(dimensions),
		_definitions(std::move(definitions))
	{
		(void)checkedCount(dimensions);
	}

	spk::Vector3UInt World::chunkDimensions() const noexcept
	{
		return _dimensions;
	}

	const Definition::Registry &World::definitions() const noexcept
	{
		return _definitions;
	}

	const Volume *World::chunk(ChunkPosition position) const noexcept
	{
		const auto found = _chunks.find(position);
		return found == _chunks.end() ? nullptr : &found->second.volume;
	}

	std::uint64_t World::revision(ChunkPosition position) const noexcept
	{
		const auto found = _chunks.find(position);
		return found == _chunks.end() ? 0 : found->second.revision;
	}

	std::vector<ChunkPosition> World::chunks() const
	{
		std::vector<ChunkPosition> result;
		result.reserve(_chunks.size());
		for (const auto &[position, chunk] : _chunks)
		{
			(void)chunk;
			result.push_back(position);
		}
		return result;
	}

	std::vector<ChunkPosition> World::takeChangedChunks()
	{
		return std::exchange(_changed, {});
	}

	void World::_markChanged(ChunkPosition position)
	{
		if (std::find(_changed.begin(), _changed.end(), position) == _changed.end())
		{
			_changed.push_back(position);
		}
	}

	void World::_replace(ChunkPosition position, Volume volume, std::uint64_t revision)
	{
		if (revision == 0 || volume.dimensions() != _dimensions)
		{
			throw spk::Exception("Voxel World snapshot revision or dimensions are invalid");
		}
		Volume::Builder builder(_dimensions);
		builder.load(volume);
		builder.bind(_definitions);
		Chunk replacement{builder.build(), revision};
		_chunks.insert_or_assign(position, std::move(replacement));
		_markChanged(position);
	}

	bool World::_apply(ChunkPosition position, const std::vector<CellEdit> &edits)
	{
		const auto found = _chunks.find(position);
		if (found == _chunks.end())
		{
			throw spk::Exception("Voxel World cannot edit a missing chunk");
		}
		if (edits.empty() || edits.size() > 4096)
		{
			throw spk::Exception("Voxel World edit batch must contain between 1 and 4096 cells");
		}
		if (found->second.revision == std::numeric_limits<std::uint64_t>::max())
		{
			throw spk::Exception("Voxel World chunk revision overflow");
		}

		std::set<std::uint32_t> positions;
		Volume::Builder builder(_dimensions);
		builder.load(found->second.volume);
		bool changed = false;
		for (const CellEdit &edit : edits)
		{
			const auto index = offset(edit.position, _dimensions);
			if (!positions.insert(index).second)
			{
				throw spk::Exception("Voxel World edit batch contains a duplicate position");
			}
			const auto id = Volume::PackedCell::definitionId(edit.value);
			if (!_definitions.contains(id))
			{
				throw spk::Exception("Voxel World edit references an unknown Definition ID");
			}
			changed |= found->second.volume.atRaw(edit.position) != edit.value;
			builder.set(edit.position, id, Volume::PackedCell::horizontal(edit.value), Volume::PackedCell::vertical(edit.value));
		}
		if (changed)
		{
			found->second = Chunk{builder.build(), found->second.revision + 1};
			_markChanged(position);
		}
		return changed;
	}

	void World::_clear()
	{
		for (const auto &[position, chunk] : _chunks)
		{
			(void)chunk;
			_markChanged(position);
		}
		_chunks.clear();
	}
}
