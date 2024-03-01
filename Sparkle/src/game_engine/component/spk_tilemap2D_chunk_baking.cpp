#include "game_engine/component/spk_tilemap2D.hpp"

namespace spk
{
	void Tilemap2D::Chunk::_insertNodeData(
		const spk::Vector3& p_position,
		const spk::Vector3& p_size,
		const spk::Vector2& p_spritePosition,
		const spk::Vector2& p_spriteSize,
		const spk::Vector2& p_animationOffset,
		int p_animationLenght,
		int p_animationDuration)
	{
		size_t previousNbVertices = _vertexData.size();

		static const spk::Vector3 offsets[4] = {
			spk::Vector3(0, 1, 0),
			spk::Vector3(0, 0, 0),
			spk::Vector3(1, 1, 0),
			spk::Vector3(1, 0, 0)};
		static const spk::Vector2 spriteOffsets[4] = {
			spk::Vector2(0, 0),
			spk::Vector2(0, 1),
			spk::Vector2(1, 0),
			spk::Vector2(1, 1)};
		static const unsigned int indexes[6] = {0, 1, 2, 2, 1, 3};

		for (size_t i = 0; i < 4; i++)
		{
			_vertexData.push_back(VertexData(
				p_position + p_size * offsets[i],
				p_spritePosition + p_spriteSize * spriteOffsets[i],
				p_animationOffset,
				p_animationLenght,
				p_animationDuration));
		}

		for (size_t i = 0; i < 6; i++)
		{
			_indexes.push_back(indexes[i] + previousNbVertices);
		}
	}

	void Tilemap2D::Chunk::_bakeStandardTile(const Node& p_node, const spk::Vector3& p_relativePosition)
	{
		_insertNodeData(
			p_relativePosition - spk::Vector3(0.5f, 0.5f, 0.0f),
			spk::Vector3(1, 1, 0),
			_spriteSheet->sprite(p_node.sprite),
			_spriteSheet->unit(),
			static_cast<spk::Vector2>(p_node.animationFrameOffset) * _spriteSheet->unit(),
			p_node.nbFrame,
			p_node.animationDuration);
	}

	spk::Vector2Int Tilemap2D::Chunk::_computeSpriteOffset(int p_baseIndex, int p_quarterIndex, const spk::Vector3& p_relativePosition)
	{
		static const spk::Vector3Int quarterOffset[4][3] = {
			{
				spk::Vector3Int(-1, 0, 0),
				spk::Vector3Int(-1, 1, 0),
				spk::Vector3Int(0, 1, 0),
			},
			{
				spk::Vector3Int(-1, 0, 0),
				spk::Vector3Int(-1, -1, 0),
				spk::Vector3Int(0, -1, 0),
			},
			{
				spk::Vector3Int(0, -1, 0),
				spk::Vector3Int(1, -1, 0),
				spk::Vector3Int(1, 0, 0),
			},
			{
				spk::Vector3Int(1, 0, 0),
				spk::Vector3Int(1, 1, 0),
				spk::Vector3Int(0, 1, 0),
			}};

		spk::Vector2Int spriteOffset[4][2][2][2] = {
			{
			{
			{
				spk::Vector2Int(0, 0),
				spk::Vector2Int(0, 3)
			},
			{
				spk::Vector2Int(0, 2),
				spk::Vector2Int(0, 3)
			}},
			{
			{
				spk::Vector2Int(1, 2),
				spk::Vector2Int(2, 0)
			},
			{
				spk::Vector2Int(1, 2),
				spk::Vector2Int(1, 3)
			}}},
			{
			{
			{
				spk::Vector2Int(0, 1),
				spk::Vector2Int(0, 4)
			},
			{
				spk::Vector2Int(0, 5),
				spk::Vector2Int(0, 4)
			}},
			{
			{
				spk::Vector2Int(1, 5),
				spk::Vector2Int(2, 1)
			},
			{
				spk::Vector2Int(1, 5),
				spk::Vector2Int(1, 4)
			}}},
			{
			{
			{
				spk::Vector2Int(1, 1),
				spk::Vector2Int(2, 5)
			},
			{
				spk::Vector2Int(3, 5),
				spk::Vector2Int(2, 5)
			}},
			{
			{
				spk::Vector2Int(3, 4),
				spk::Vector2Int(3, 1)
			},
			{
				spk::Vector2Int(3, 4),
				spk::Vector2Int(2, 4)
			}}},
			{
			{
			{
				spk::Vector2Int(1, 0),
				spk::Vector2Int(3, 3)
			},
			{
				spk::Vector2Int(3, 2),
				spk::Vector2Int(3, 3)
			}},
			{
			{
				spk::Vector2Int(2, 2),
				spk::Vector2Int(3, 0)
			},
			{
				spk::Vector2Int(2, 2),
				spk::Vector2Int(2, 3)
			}}}};

		bool isSame[3] = {false, false, false};

		for (size_t i = 0; i < 3; i++)
		{
			spk::Vector3Int toCheck = p_relativePosition + quarterOffset[p_quarterIndex][i];

			const Chunk* chunkToCheck = _getNeightbourChunk(toCheck);

			if (chunkToCheck != nullptr)
			{
				if (chunkToCheck->content(toCheck) == p_baseIndex)
				{
					isSame[i] = true;
				}
			}
		}

		return (spriteOffset[p_quarterIndex][isSame[0]][isSame[1]][isSame[2]]);
	}

	void Tilemap2D::Chunk::_bakeAutotile(const Node& p_node, int p_index, const spk::Vector3& p_relativePosition)
	{
		spk::Vector3 offsets[4] = {
			spk::Vector3(0.0f, 0.5f, 0.0f),
			spk::Vector3(0.0f, 0.0f, 0.0f),
			spk::Vector3(0.5f, 0.0f, 0.0f),
			spk::Vector3(0.5f, 0.5f, 0.0f)};

		for (size_t i = 0; i < 4; i++)
		{
			_insertNodeData(p_relativePosition - spk::Vector3(0.5f, 0.5f, 0.0f) + offsets[i],
							spk::Vector3(0.5f, 0.5f, 0),
							_spriteSheet->sprite(p_node.sprite + _computeSpriteOffset(p_index, i, p_relativePosition)),
							_spriteSheet->unit(),
							static_cast<spk::Vector2>(p_node.animationFrameOffset) * _spriteSheet->unit(), p_node.nbFrame, p_node.animationDuration);
		}
	}

	void Tilemap2D::Chunk::_bakeTile(int p_index, const spk::Vector3& p_relativePosition)
	{
		const Node& node = _tilemapCreator->node(p_index);

		if (node.type == Node::Type::Standard)
		{
			_bakeStandardTile(node, p_relativePosition);
		}
		else
		{
			_bakeAutotile(node, p_index, p_relativePosition);
		}
	}

	void Tilemap2D::Chunk::_onBaking()
	{
		_vertexData.clear();
		_indexes.clear();

		for (int z = SizeZ - 1; z >= 0; z--)
		{
			for (int y = 0; y < SizeY; y++)
			{
				for (int x = 0; x < SizeX; x++)
				{
					spk::Vector3 relativePosition = spk::Vector3Int(x, y, z);
					NodeIndexType index = content(relativePosition);

					if (_tilemapCreator->containsNode(index) == true)
					{
						_bakeTile(index, relativePosition);
					}
				}
			}
		}

		_renderingObject.setVertices(_vertexData);
		_renderingObject.setIndexes(_indexes);

		_vertexData.clear();
		_indexes.clear();

		_createCollisionMesh();
	}
}