#include "game_engine/spk_tilemap2D.hpp"
#include "game_engine/component/spk_camera_component.hpp"

namespace spk
{
    spk::Vector2Int Tilemap2D::Chunk::_computeSpriteOffset(int p_baseIndex, int p_quarterIndex, int p_x, int p_y, int p_z)
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
            /*
            BC.
            A#.
            ...
            */
            { // Top/Left
             {// A = false
              {
                  // B = False
                  spk::Vector2Int(0, 0), //[ ][ ][ ]
                  spk::Vector2Int(0, 3)  //[ ][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(0, 2), //[ ][X][ ]
                  spk::Vector2Int(0, 3)  //[ ][X][X]
              }},
             {// A = true
              {
                  // B = False
                  spk::Vector2Int(1, 2), //[X][ ][ ]
                  spk::Vector2Int(2, 0)  //[X][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(1, 2), //[X][X][ ]
                  spk::Vector2Int(1, 3)  //[X][X][X]
              }}},
            /*
            ...
            A#.
            BC.
            */
            { // Down/Left
             {// A = false
              {
                  // B = False
                  spk::Vector2Int(0, 1), //[ ][ ][ ]
                  spk::Vector2Int(0, 4)  //[ ][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(0, 5), //[ ][X][ ]
                  spk::Vector2Int(0, 4)  //[ ][X][X]
              }},
             {// A = true
              {
                  // B = False
                  spk::Vector2Int(1, 5), //[X][ ][ ]
                  spk::Vector2Int(2, 1)  //[X][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(1, 5), //[X][X][ ]
                  spk::Vector2Int(1, 4)  //[X][X][X]
              }}},
            /*
            ...
            .#C
            .AB
            */
            { // Down/Right
             {// A = false
              {
                  // B = False
                  spk::Vector2Int(1, 1), //[ ][ ][ ]
                  spk::Vector2Int(2, 5)  //[ ][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(3, 5), //[ ][X][ ]
                  spk::Vector2Int(2, 5)  //[ ][X][X]
              }},
             {// A = true
              {
                  // B = False
                  spk::Vector2Int(3, 4), //[X][ ][ ]
                  spk::Vector2Int(3, 1)  //[X][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(3, 4), //[X][X][ ]
                  spk::Vector2Int(2, 4)  //[X][X][X]
              }}},
            /*
            .CB
            .#A
            ...
            */
            { // Top/Right
             {// A = false
              {
                  // B = False
                  spk::Vector2Int(1, 0), //[ ][ ][ ]
                  spk::Vector2Int(3, 3)  //[ ][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(3, 2), //[ ][X][ ]
                  spk::Vector2Int(3, 3)  //[ ][X][X]
              }},
             {// A = true
              {
                  // B = False
                  spk::Vector2Int(2, 2), //[X][ ][ ]
                  spk::Vector2Int(3, 0)  //[X][ ][X]
              },
              {
                  // B = true
                  spk::Vector2Int(2, 2), //[X][X][ ]
                  spk::Vector2Int(2, 3)  //[X][X][X]
              }}}};

        bool isSame[3] = {false, false, false};

        for (size_t i = 0; i < 3; i++)
        {
            spk::Vector3Int toCheck = spk::Vector3Int(p_x, p_y, p_z) + quarterOffset[p_quarterIndex][i];

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

    void Tilemap2D::Chunk::_insertAutotiledNode(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const NodeIndexType& p_index, const Node& p_node, int p_x, int p_y, int p_z)
    {
        spk::Vector3 offsets[4] = {
            spk::Vector3(0.0f, 0.5f, 0.0f),
            spk::Vector3(0.0f, 0.0f, 0.0f),
            spk::Vector3(0.5f, 0.0f, 0.0f),
            spk::Vector3(0.5f, 0.5f, 0.0f)};

        for (size_t i = 0; i < 4; i++)
        {
            _insertNodeData(p_vertexData, p_indexes,
                            spk::Vector3(static_cast<float>(p_x) - 0.5f, static_cast<float>(p_y) - 0.5f, static_cast<float>(p_z)) + offsets[i],
                            spk::Vector3(0.5f, 0.5f, 0),
                            static_cast<spk::SpriteSheet*>(Chunk::texture())->sprite(p_node.sprite + _computeSpriteOffset(p_index, i, p_x, p_y, p_z)),
                            static_cast<spk::SpriteSheet*>(Chunk::texture())->unit(),
                            static_cast<spk::Vector2>(p_node.animationFrameOffset) * static_cast<spk::SpriteSheet*>(Chunk::texture())->unit(), p_node.nbFrame, p_node.animationDuration);
        }
    }
}