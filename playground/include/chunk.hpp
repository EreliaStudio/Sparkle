#pragma once

#include <fstream>
#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

class Chunk
{
public:
	static inline const size_t Size = 16;
	static inline const size_t Layer = 5;
	static inline const spk::Vector3Int Dimensions = spk::Vector3Int(Size, Size, Layer);

private:
	int _content[Size][Size][Layer];

public:
	Chunk();

	static spk::Vector2Int convertWorldToChunkPosition(const spk::Vector2& p_worldPosition);
	static spk::Vector2Int convertWorldToChunkPosition(const spk::Vector3& p_worldPosition);
	static spk::Vector3 convertChunkToWorldPosition(const spk::Vector2Int& p_chunkPosition);

	void serialize(std::ofstream& p_os) const;
	void deserialize(std::ifstream& p_is);

	void setContent(const spk::Vector3Int& p_relPosition, int p_value);
	void setContent(const spk::Vector2Int& p_relPosition, const int& p_layer, int p_value);
	void setContent(const int& p_x, const int& p_y, const int& p_z, int p_value);

	int content(const spk::Vector3Int& p_relPosition) const;
	int content(const spk::Vector2Int& p_relPosition, const int& p_layer) const;
	int content(const int& p_x, const int& p_y, const int& p_z) const;
};

class BakableChunk : public Chunk
{
private:
	bool _needBake = true;

public:
	BakableChunk();

	bool needBake();

	void invalidate();
	void validate();
};