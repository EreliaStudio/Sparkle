#include "chunk.hpp"

#include <cstring>

Chunk::Chunk()
{
	std::memset(&_content, -1, sizeof(_content));
}

spk::Vector2Int Chunk::convertWorldToChunkPosition(const spk::Vector2& p_worldPosition)
{
	return (spk::Vector2Int(
		std::floor(p_worldPosition.x / static_cast<float>(Size)), 
		std::floor(p_worldPosition.y / static_cast<float>(Size)))
	);
}

spk::Vector2Int Chunk::convertWorldToChunkPosition(const spk::Vector3& p_worldPosition)
{
	return (spk::Vector2Int(
		std::floor(p_worldPosition.x / static_cast<float>(Size)), 
		std::floor(p_worldPosition.y / static_cast<float>(Size)))
	);
}

spk::Vector3 Chunk::convertChunkToWorldPosition(const spk::Vector2Int& p_chunkPosition)
{
	return (spk::Vector3(p_chunkPosition.x * static_cast<int>(Size), p_chunkPosition.y * static_cast<int>(Size), 0));
}

spk::Vector2Int Chunk::convertAbsoluteToRelativePosition(const spk::Vector2Int& p_absolutePosition)
{
	return (spk::Vector2Int(
		static_cast<int>(std::floor(p_absolutePosition.x)) % Size + (p_absolutePosition.x < 0 ? Size : 0), 
		static_cast<int>(std::floor(p_absolutePosition.y)) % Size + (p_absolutePosition.y < 0 ? Size : 0)
	));
}

void Chunk::serialize(std::ofstream& p_os) const
{
	p_os.write(reinterpret_cast<const char*>(_content), sizeof(_content));
}

void Chunk::deserialize(std::ifstream& p_is)
{
	p_is.read(reinterpret_cast<char*>(_content), sizeof(_content));
}

void Chunk::setContent(const spk::Vector3Int& p_relPosition, int p_value)
{
	_content[p_relPosition.x][p_relPosition.y][p_relPosition.z] = p_value;
}

void Chunk::setContent(const spk::Vector2Int& p_relPosition, const int& p_layer, int p_value)
{
	_content[p_relPosition.x][p_relPosition.y][p_layer] = p_value;
}

void Chunk::setContent(const int& p_x, const int& p_y, const int& p_z, int p_value)
{
	_content[p_x][p_y][p_z] = p_value;
}

int Chunk::content(const spk::Vector3Int& p_relPosition) const
{
	return (_content[p_relPosition.x][p_relPosition.y][p_relPosition.z]);
}

int Chunk::content(const spk::Vector2Int& p_relPosition, const int& p_layer) const
{
	return (_content[p_relPosition.x][p_relPosition.y][p_layer]);
}

int Chunk::content(const int& p_x, const int& p_y, const int& p_z) const
{
	return (_content[p_x][p_y][p_z]);
}

BakableChunk::BakableChunk() :
	Chunk()
{
	
}

bool BakableChunk::needBake()
{
	return (_needBake);
}

void BakableChunk::invalidate()
{
	_needBake = true;
}

void BakableChunk::validate()
{
	_needBake = false;
}