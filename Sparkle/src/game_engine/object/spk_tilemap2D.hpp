#pragma once

#include "game_engine/object/spk_itilemap.hpp"

namespace spk
{
	/**
	 * @class Tilemap2D
	 * @brief A specialized 2D tilemap implementation derived from ITilemap, providing specific functionalities for 2D tile-based environments.
	 *
	 * Tilemap2D extends the generic ITilemap with additional features tailored for 2D games. It allows for the management of
	 * tiles, including their properties, rendering, and collision information. This class integrates with the game's rendering and
	 * physics systems to display the tilemap and handle interactions.
	 *
	 * Usage example:
	 * @code
	 * spk::Tilemap2D tilemap("Level1");
	 * tilemap.setSpriteSheet(spriteSheet);
	 * tilemap.insertNodeType(1, nodeType);
	 * tilemap.setActiveChunkRange({0, 0}, {5, 5});
	 * tilemap.updateActiveChunks();
	 * @endcode
	 */
	class Tilemap2D : public spk::ITilemap<short, 16, 16, 5>
	{
	public:
		/**
		 * @struct Node
		 * @brief Represents a single tile node within the tilemap, encapsulating its visual and behavioral properties.
		 *
		 * The Node structure is used to define the attributes of individual tiles within the tilemap. It includes
		 * information about the tile's appearance, such as its sprite and animation, as well as its behavior, defined
		 * through flags like walkable or obstacle.
		 */
		struct Node
		{
			/**
			 * @enum Type
			 * @brief Defines the type of tile node, influencing how it should be rendered and interacted with.
			 */
			enum class Type
			{
				Autotile,  ///< The node uses autotiling logic to determine its sprite based on neighboring tiles.
				Standard   ///< The node uses a standard, fixed sprite regardless of its neighbors.
			};

			static inline const uint16_t WALKABLE = 0b0000000000000000; ///< Flag indicating the tile is walkable.
			static inline const uint16_t OBSTACLE = 0b0000000000000001; ///< Flag indicating the tile is an obstacle.

			spk::Vector2Int sprite = 0;                 ///< The position of the tile's sprite in the sprite sheet.
			Type type = Type::Standard;                 ///< The type of the node, determining its rendering logic.
			spk::Vector2Int animationFrameOffset = 0;   ///< The starting offset for the tile's animation frames in the sprite sheet.
			int nbFrame = 0;                            ///< The number of frames in the tile's animation.
			int animationDuration = 0;                  ///< The duration of the tile's animation cycle.
			int flags = WALKABLE;                       ///< Flags indicating the tile's behavior (e.g., walkable, obstacle).


			/**
			 * @brief Default constructor, initializes a Node with default values.
			 */
			Node()
			{

			}

			/**
			 * @brief Constructs a Node with specified sprite, flags, and type, without animation.
			 *
			 * @param p_sprite The position of the tile's sprite in the sprite sheet.
			 * @param p_flags The behavior flags for the tile (e.g., walkable, obstacle).
			 * @param p_type The type of the node (e.g., Autotile, Standard).
			 */
			Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, Type p_type) :
				sprite(p_sprite),
				type(p_type),
				flags(p_flags)
			{

			}

			/**
			 * @brief Constructs a Node with specified sprite, flags, type, and animation properties.
			 *
			 * @param p_sprite The position of the tile's sprite in the sprite sheet.
			 * @param p_flags The behavior flags for the tile.
			 * @param p_type The type of the node.
			 * @param p_animationFrameOffset The starting offset for the animation frames in the sprite sheet.
			 * @param p_nbFrame The number of frames in the animation.
			 * @param p_animationDuration The duration of the animation cycle.
			 */
			Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, Type p_type, const spk::Vector2Int& p_animationFrameOffset, int p_nbFrame, int p_animationDuration) :
				sprite(p_sprite),
				type(p_type),
				animationFrameOffset(p_animationFrameOffset),
				nbFrame(p_nbFrame),
				animationDuration(p_animationDuration),
				flags(p_flags)
			{

			}
		};

		/**
		 * @class Chunk
		 * @brief Represents a single chunk in the Tilemap2D, encapsulating the rendering and collision data for a section of the tilemap.
		 *
		 * This class is a specialized version of the IChunk, designed to work within the Tilemap2D. It holds the data necessary for rendering a chunk
		 * of tiles and managing its collision mesh. The Chunk class is responsible for baking the tile data into a format suitable for rendering and collision detection.
		 */
		class Chunk : public Tilemap2D::IChunk
		{
			friend class Tilemap2D;

		private:
			static inline const std::string _renderingPipelineCode = R"(#version 450

			#include <timeConstants>
			#include <cameraConstants>
			#include <transform>
			#include <transformUtils>
			
			Input -> Geometry : vec3 modelVertex;
			Input -> Geometry : vec2 modelUVs;
			Input -> Geometry : vec2 modelAnimationOffset;
			Input -> Geometry : int modelAnimationSize;
			Input -> Geometry : int modelAnimationDuration;

			Geometry -> Render : vec2 fragmentUVs;
			Geometry -> Render : float depth;

			AttributeBlock self
			{
				Transform transform;
			};

			Texture textureID;

			vec2 computeSpriteAnimationOffset(int animationDuration, int animationSize, vec2 frameOffset)
			{
				if (animationSize == 0)
					return (vec2(0, 0));

				int nbFrame = (timeConstants.epoch % animationDuration) / (animationDuration / animationSize);
				return (frameOffset * vec2(nbFrame, nbFrame));
			}
			
			void geometryPass()
			{
				vec3 transformedPosition = applyTransform(modelVertex, self.transform);
				vec4 cameraSpacePosition = cameraConstants.view * vec4(transformedPosition, 1.0f);
				pixelPosition = cameraConstants.projection * cameraSpacePosition;
				fragmentUVs = modelUVs + computeSpriteAnimationOffset(modelAnimationDuration, modelAnimationSize, modelAnimationOffset);
				depth = pixelPosition.z;
			}

			void renderPass()
			{
				if (false)
				{
					if (depth <= 0.001f)
						pixelColor = vec4(1, 0, 0.5, 1);
					else if (depth >= 0.999f)
						pixelColor = vec4(1, 0, 0, 1);
					else
						pixelColor = vec4(depth, depth, depth, 1.0f);
				}
				else
				{
					pixelColor = texture(textureID, fragmentUVs);
					if (pixelColor.a == 0)
						discard;
				}
			})";
			static inline spk::Pipeline _renderingPipeline = spk::Pipeline(_renderingPipelineCode);

			spk::Pipeline::Object _renderingObject;

			spk::Pipeline::Object::Attribute& _renderingObjectSelfAttribute;
			spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformTranslationElement;
			std::unique_ptr<spk::Transform::Contract> _transformTranslationContract = nullptr;

			spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformScaleElement;
			std::unique_ptr<spk::Transform::Contract> _transformScaleContract = nullptr;
			
			spk::Pipeline::Object::Attribute::Element& _renderingObjectSelfTransformRotationElement;
			std::unique_ptr<spk::Transform::Contract> _transformRotationContract = nullptr;

			spk::Pipeline::Texture& _textureObject;
			spk::Texture* _texture;

			struct VertexData
			{
				spk::Vector3 modelVertex;
				spk::Vector2 modelUVs;
				spk::Vector2 modelAnimationOffset;
				int modelAnimationSize;
				int modelAnimationDuration;

				VertexData() :
					modelVertex(0),
					modelUVs(0),
					modelAnimationOffset(0),
					modelAnimationSize(0),
					modelAnimationDuration(0)
				{

				}

				VertexData( const spk::Vector3& p_modelVertex,
							const spk::Vector2& p_modelUVs,
							const spk::Vector2& p_modelAnimationOffset,
							int p_modelAnimationSize,
							int p_modelAnimationDuration ) :
					modelVertex(p_modelVertex),
					modelUVs(p_modelUVs),
					modelAnimationOffset(p_modelAnimationOffset),
					modelAnimationSize(p_modelAnimationSize),
					modelAnimationDuration(p_modelAnimationDuration)
				{

				}
			};

			static inline std::vector<VertexData> _vertexData;
			static inline std::vector<unsigned int> _indexes;

			const Tilemap2D* _tilemapCreator;
			const spk::SpriteSheet* _spriteSheet;
			spk::Mesh _collisionMesh;

			const Chunk* _neightbours[3][3] = {
				{nullptr, nullptr, nullptr},
				{nullptr, nullptr, nullptr},
				{nullptr, nullptr, nullptr}
			};

			const Chunk* _getChunkNeighbour(const spk::Vector2Int& p_chunkOffset)
			{
				_searchChunkNeighbour(p_chunkOffset);
				return (_neightbours[p_chunkOffset.x + 1][p_chunkOffset.y + 1]);
			}

			void _searchChunkNeighbour(const spk::Vector2Int& p_chunkOffset)
			{
				spk::Vector2Int relativeChunkPosition = p_chunkOffset + 1;
				if (_neightbours[relativeChunkPosition.x][relativeChunkPosition.y] == nullptr)
				{
					_neightbours[relativeChunkPosition.x][relativeChunkPosition.y] = dynamic_cast<const Chunk*>(_tilemapCreator->chunk(position() + p_chunkOffset));
				}
			}
			
			const Tilemap2D::Chunk* _getNeightbourChunk(spk::Vector3Int& p_relativePosition)
			{
				spk::Vector2Int chunkOffset = 0;

				if (p_relativePosition.x < 0)
				{
					chunkOffset.x--;
					p_relativePosition.x += SizeX;
				}
				else if (p_relativePosition.x >= SizeX)
				{
					chunkOffset.x++;
					p_relativePosition.x -= SizeX;
				}

				if (p_relativePosition.y < 0)
				{
					chunkOffset.y--;
					p_relativePosition.y += SizeY;
				}
				else if (p_relativePosition.y >= SizeY)
				{
					chunkOffset.y++;
					p_relativePosition.y -= SizeY;
				}

				return (_getChunkNeighbour(chunkOffset));
			}

			void _insertNodeData(
				const spk::Vector3& p_position,
				const spk::Vector3& p_size,
				const spk::Vector2& p_spritePosition,
				const spk::Vector2& p_spriteSize,
				const spk::Vector2& p_animationOffset,
				int p_animationLenght,
				int p_animationDuration)			
			{
				unsigned int previousNbVertices = static_cast<int>(_vertexData.size());

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

			void _bakeStandardTile(const Node& p_node, const spk::Vector3& p_relativePosition)
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

			spk::Vector2Int _computeSpriteOffset(int p_baseIndex, int p_quarterIndex, const spk::Vector3& p_relativePosition)
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
			void _bakeAutotile(const Node& p_node, int p_index, const spk::Vector3& p_relativePosition)
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
									_spriteSheet->sprite(p_node.sprite + _computeSpriteOffset(p_index, static_cast<int>(i), p_relativePosition)),
									_spriteSheet->unit(),
									static_cast<spk::Vector2>(p_node.animationFrameOffset) * _spriteSheet->unit(), p_node.nbFrame, p_node.animationDuration);
				}
			}

			void _bakeTile(int p_index, const spk::Vector3& p_relativePosition)
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

			void _onBaking() override
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

			bool _isValidSquare(bool (&p_used)[SizeX][SizeY], const spk::Vector2Int& p_start, const spk::Vector2Int& p_end)
			{
				if (p_end.x >= SizeX || p_end.y >= SizeY)
					return (false);

				for (int x = p_start.x; x <= p_end.x; x++)
				{
					for (int y = p_start.y; y <= p_end.y; y++)
					{
						if (p_used[x][y] == true)
							return (false);
					}
				}

				return (true);
			}

			spk::Vector2Int _searchEndPosition(bool (&p_used)[SizeX][SizeY], const spk::Vector2Int& p_start)
			{
				spk::Vector2Int result = p_start;

				while (_isValidSquare(p_used, p_start, result + spk::Vector2Int(1, 0)) == true)result += spk::Vector2Int(1, 0);
				while (_isValidSquare(p_used, p_start, result + spk::Vector2Int(0, 1)) == true)result += spk::Vector2Int(0, 1);

				return result;
			}

			void _createCollisionMesh()
			{
				unsigned int indexes[6] = {
					0, 1, 2, 2, 1, 3
				};
				bool used[SizeX][SizeY];

				for (int y = 0; y < SizeY; y++)
				{
					for (int x = 0; x < SizeX; x++)
					{
						used[x][y] = isObstacle(spk::Vector2Int(x, y));
					}
				}

				for (int y = 0; y < SizeY; y++)
				{
					for (int x = 0; x < SizeX; x++)
					{
						if (used[x][y] == false)
						{
							spk::Vector2Int start = spk::Vector2Int(x, y);
							spk::Vector2Int end = _searchEndPosition(used, start);

							for (int i = start.x; i <= end.x; i++)
							{
								for (int j = start.y; j <= end.y; j++)
								{
									used[i][j] = true;
								}
							}

							unsigned int baseIndexPoint = static_cast<unsigned int>(_collisionMesh.points().size());
							unsigned int baseIndexNormal = static_cast<unsigned int>(_collisionMesh.normals().size());

							_collisionMesh.addPoint(spk::Vector3(start.x, start.y, 0));
							_collisionMesh.addPoint(spk::Vector3(start.x, end.y, 0));
							_collisionMesh.addPoint(spk::Vector3(end.x, start.y, 0));
							_collisionMesh.addPoint(spk::Vector3(end.x, end.y, 0));

							_collisionMesh.addNormal(spk::Vector3(0, 0, 1));

							for (size_t i = 0; i < 6; i++)
								_collisionMesh.addIndex(baseIndexPoint + indexes[i], -1, baseIndexNormal);
						}
					}
				}
			}

			void _onObjectRendering() override
			{
				_textureObject.attach(_spriteSheet);
				_renderingObject.render();
			}

			void _updateTransform()
			{
				_renderingObjectSelfTransformTranslationElement = globalPosition();
				_renderingObjectSelfTransformScaleElement = globalScale();
				_renderingObjectSelfTransformRotationElement = globalRotation();
				_renderingObjectSelfAttribute.update();
			}

			std::vector<spk::GameObject*> _activeChunks;
			
			spk::Vector2Int _activeChunkStart;
			spk::Vector2Int _activeChunkEnd;

			std::map<Chunk::NodeIndexType, Node> _nodes;

			Chunk(const Tilemap2D* p_tilemapCreator, const spk::Vector2Int& p_chunkPosition) :
				IChunk(p_chunkPosition),
				_tilemapCreator(p_tilemapCreator),
				_renderingObject(_renderingPipeline.createObject()),
				_textureObject(_renderingPipeline.texture("textureID")),
				_renderingObjectSelfAttribute(_renderingObject.attribute("self")),
				_renderingObjectSelfTransformTranslationElement(_renderingObjectSelfAttribute["transform"]["translation"]),
				_transformTranslationContract(transform().translation.subscribe([&](){_updateTransform();})),
				_renderingObjectSelfTransformScaleElement(_renderingObjectSelfAttribute["transform"]["scale"]),
				_transformScaleContract(transform().scale.subscribe([&](){_updateTransform();})),
				_renderingObjectSelfTransformRotationElement(_renderingObjectSelfAttribute["transform"]["rotation"]),
				_transformRotationContract(transform().rotation.subscribe([&](){_updateTransform();}))
			{
				_updateTransform();

				for (int x = -1; x <= 1; x++)
				{
					for (int y = -1; y <= 1; y++)
					{
						_searchChunkNeighbour(spk::Vector2Int(x, y));
					}
				}
				
				bake();
			}

		public:
		    /**
			 * @brief Sets the sprite sheet used by this chunk for rendering its tiles.
			 * 
			 * @param p_spriteSheet Pointer to the sprite sheet resource.
			 */
			void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
			{
				_spriteSheet = p_spriteSheet;
			}

			/**
			 * @brief Retrieves the sprite sheet associated with this chunk.
			 * 
			 * @return A pointer to the sprite sheet used by this chunk.
			 */
			const spk::SpriteSheet* spriteSheet()
			{
				return (_spriteSheet);
			}

			/**
			 * @brief Gets the collision mesh associated with this chunk, representing the physical boundaries of the tiles.
			 * 
			 * @return A reference to the mesh used for collision detection in this chunk.
			 */
			const spk::Mesh& collisionMesh() const
			{
				return (_collisionMesh);
			}

			/**
			 * @brief Checks if a specific flag is set for a tile at the given position within this chunk.
			 * 
			 * @param p_position The position of the tile within the chunk.
			 * @param p_flags The flag or combination of flags to check for.
			 * @return True if the specified flag(s) are set, false otherwise.
			 */
			bool isFlag(const spk::Vector2Int& p_position, int p_flags) const
			{
				for (size_t k = 0; k < SizeZ; k++)
				{
					Chunk::NodeIndexType index = content(p_position, static_cast<int>(k));
					if (_tilemapCreator->containsNode(index) == true)
					{
						const Node& tmpNode = _tilemapCreator->node(index);

						if ((tmpNode.flags & p_flags) == p_flags)
							return (true);
					}
				}
				return (false);
			}

			/**
			 * @brief Checks if a specific flag is set for a tile at the given 3D position within this chunk.
			 * 
			 * @param p_position The 3D position of the tile within the chunk.
			 * @param p_flags The flag or combination of flags to check for.
			 * @return True if the specified flag(s) are set, false otherwise.
			 */
			bool isFlag(const spk::Vector3Int& p_position, int p_flags) const
			{
				return (isFlag(p_position.xy(), p_flags));
			}

			/**
			 * @brief Determines if the tile at the specified position is an obstacle.
			 * 
			 * @param p_position The position of the tile within the chunk.
			 * @return True if the tile is an obstacle, false otherwise.
			 */
			bool isObstacle(const spk::Vector2Int& p_position) const
			{
				return (isFlag(p_position, Node::OBSTACLE));
			}

			/**
			 * @brief Determines if the tile at the specified 3D position is an obstacle.
			 * 
			 * @param p_position The 3D position of the tile within the chunk.
			 * @return True if the tile is an obstacle, false otherwise.
			 */
			bool isObstacle(const spk::Vector3Int& p_position) const
			{
				return (isFlag(p_position.xy(), Node::OBSTACLE));
			}
			
			/**
			 * @brief Retrieves the combined flag values for the tile at a specified position within this chunk.
			 * 
			 * @param p_position The position of the tile within the chunk.
			 * @return An integer representing the combined flag values for the tile.
			 */
			int flag(const spk::Vector2Int& p_position) const
			{
				int result = 0;

				for (size_t k = 0; k < SizeZ; k++)
				{
					Chunk::NodeIndexType index = content(p_position, static_cast<int>(k));
					if (_tilemapCreator->containsNode(index) == true)
					{
						const Node& tmpNode = _tilemapCreator->node(index);

						result |= tmpNode.flags;
					}
				}

				return (result);
			}

			/**
			 * @brief Retrieves the combined flag values for the tile at a specified 3D position within this chunk.
			 * 
			 * @param p_position The 3D position of the tile within the chunk.
			 * @return An integer representing the combined flag values for the tile.
			 */
			int flag(const spk::Vector3Int& p_position) const
			{
				return (flag(p_position.xy()));
			}

		};

	private:
		IChunk* _instanciateNewChunk(const spk::Vector2Int& p_chunkPosition) override
		{
			Chunk* result = new Chunk(this, p_chunkPosition);

			result->setSpriteSheet(_spriteSheet);

			return (result);
		}

		std::vector<IChunk*> _activeChunks;
		
		const spk::SpriteSheet* _spriteSheet;
		spk::Vector2Int _activeChunkStart;
		spk::Vector2Int _activeChunkEnd;

		std::map<Chunk::NodeIndexType, Node> _nodes;

	public:
		/**
		 * @brief Constructs a Tilemap2D object with a specified name and an optional parent GameObject.
		 *
		 * @param p_name The name of the tilemap.
		 * @param p_parent An optional pointer to a parent GameObject.
		 */
		Tilemap2D(const std::string& p_name, spk::GameObject* p_parent = nullptr) :
			ITilemap(p_name, p_parent)
		{

		}

		/**
		 * @brief Sets the sprite sheet used by the tilemap for rendering tiles.
		 *
		 * @param p_spriteSheet A pointer to the sprite sheet object.
		 */
		void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
		{
			_spriteSheet = p_spriteSheet;
			for (auto& [key, element] : chunks())
			{
				dynamic_cast<Chunk*>(element.get())->setSpriteSheet(_spriteSheet);
			}
		}

		/**
		 * @brief Checks if a node with the specified index exists within the tilemap.
		 *
		 * @param p_nodeIndex The index of the node to check.
		 * @return True if the node exists, otherwise false.
		 */
		bool containsNode(const Chunk::NodeIndexType& p_nodeIndex) const
		{
			return (_nodes.contains(p_nodeIndex));
		}

		/**
		 * @brief Adds a node type to the tilemap, allowing it to be used in subsequent tile definitions.
		 *
		 * @param p_nodeIndex The index to associate with the node.
		 * @param p_node The node object that defines the tile's properties.
		 */
		void insertNodeType(const Chunk::NodeIndexType& p_nodeIndex, const Node& p_node)
		{
			_nodes[p_nodeIndex] = p_node;
		}

		/**
		 * @brief Retrieves the node associated with the given index in the tilemap.
		 *
		 * @param p_nodeIndex The index of the node to retrieve.
		 * @return A reference to the node object.
		 */
		const Node& node(const Chunk::NodeIndexType& p_nodeIndex) const
		{
			return (_nodes.at(p_nodeIndex));
		}

		/**
		 * @brief Sets the range of active chunks in the tilemap, defining the area that should be updated and rendered.
		 *
		 * @param p_activeChunkStart The starting position of the active chunk range.
		 * @param p_activeChunkEnd The ending position of the active chunk range.
		 */
		void setActiveChunkRange(const spk::Vector2Int& p_activeChunkStart, const spk::Vector2Int& p_activeChunkEnd)
		{
			_activeChunkStart = p_activeChunkStart;
			_activeChunkEnd = p_activeChunkEnd;
		}

		/**
		 * @brief Calculates which chunks are missing within the active range and should be loaded or generated.
		 *
		 * @return A vector containing the positions of the missing chunks.
		 */
		std::vector<spk::Vector2Int> missingChunks() const
		{
			std::vector<spk::Vector2Int> result;

			for (int x = _activeChunkStart.x; x <= _activeChunkEnd.x; x++)
			{
				for (int y = _activeChunkStart.y; y <= _activeChunkEnd.y; y++)
				{
					spk::Vector2Int chunkPosition = spk::Vector2Int(x, y);
					if (containsChunk(chunkPosition) == false)
						result.push_back(chunkPosition);
				}
			}

			return (result);
		}

		/**
		 * @brief Updates the active status of chunks within the specified range, activating or deactivating them as needed.
		 */
		void updateActiveChunks()
		{
			std::vector<IChunk*> chunksToActivate;

			for (int x = _activeChunkStart.x; x <= _activeChunkEnd.x; x++)
			{
				for (int y = _activeChunkStart.y; y <= _activeChunkEnd.y; y++)
				{
					IChunk* chunkToActivate = chunk(spk::Vector2Int(x, y));

					if (chunkToActivate != nullptr)
					{
						chunksToActivate.push_back(chunkToActivate);
					}
				}
			}

			for (auto& element : _activeChunks)
			{
				if (std::find(chunksToActivate.begin(), chunksToActivate.end(), element) == chunksToActivate.end())
					element->deactivate();
			}

			_activeChunks = chunksToActivate;
			for (auto& element : _activeChunks)
			{
				if (element->isActive() == false)
				{
					element->activate();
				}
			}
		}

		/**
		 * @brief Determines if a specific flag is set for a tile at a given absolute position in the tilemap.
		 *
		 * @param p_absolutePosition The absolute position of the tile in the tilemap.
		 * @param p_flags The flag or combination of flags to check for.
		 * @return True if the specified flag(s) are set for the tile, otherwise false.
		 */
		bool isFlag(const spk::Vector2Int& p_absolutePosition, int p_flags) const
		{
			spk::Vector2Int chunkPosition = convertWorldToChunkPosition(p_absolutePosition);

			if (containsChunk(chunkPosition) == true)
			{
				const Chunk* tmpChunk = dynamic_cast<const Chunk*>(chunk(chunkPosition));

				return (tmpChunk->isFlag(tmpChunk->convertAbsoluteToRelativePosition(p_absolutePosition), p_flags));
			}
			return (true);
		}

		/**
		 * @brief Determines if a specific flag is set for a tile at a given absolute 3D position in the tilemap.
		 *
		 * @param p_absolutePosition The absolute 3D position of the tile in the tilemap.
		 * @param p_flags The flag or combination of flags to check for.
		 * @return True if the specified flag(s) are set for the tile, otherwise false.
		 */
		bool isFlag(const spk::Vector3Int& p_absolutePosition, int p_flags) const
		{
			return (isFlag(p_absolutePosition.xy(), p_flags));
		}

        /**
         * @brief Checks if the tile at the specified 3D absolute position is an obstacle.
         *
         * This method determines whether the specified tile has the OBSTACLE flag set, indicating
         * that it should be considered an impassable or solid object within the game world.
         *
         * @param p_absolutePosition The 3D absolute position of the tile within the tilemap.
         * @return True if the tile is an obstacle, otherwise false.
         */
		bool isObstacle(const spk::Vector3Int& p_absolutePosition) const
		{
			return (isFlag(p_absolutePosition.xy(), Node::OBSTACLE));
		}

        /**
         * @brief Checks if the tile at the specified 2D absolute position is an obstacle.
         *
         * Similar to the 3D version, this method checks if the specified 2D position in the tilemap
         * corresponds to a tile marked as an obstacle.
         *
         * @param p_absolutePosition The 2D absolute position of the tile within the tilemap.
         * @return True if the tile is an obstacle, otherwise false.
         */
		bool isObstacle(const spk::Vector2Int& p_absolutePosition) const
		{
			return (isFlag(p_absolutePosition, Node::OBSTACLE));
		}

        /**
         * @brief Retrieves the combined flag values for the tile at a specified 2D absolute position.
         *
         * This method aggregates all the flags set for the tile at the given position, providing
         * a comprehensive status that can be used to determine various properties of the tile.
         *
         * @param p_absolutePosition The 2D absolute position of the tile within the tilemap.
         * @return An integer representing the combined flag values for the tile.
         */
		int flag(const spk::Vector2Int& p_absolutePosition) const
		{
			spk::Vector2Int chunkPosition = convertWorldToChunkPosition(p_absolutePosition);

			if (containsChunk(chunkPosition) == true)
			{
				const Chunk* tmpChunk = dynamic_cast<const Chunk*>(chunk(chunkPosition));

				return (tmpChunk->flag(tmpChunk->convertAbsoluteToRelativePosition(p_absolutePosition)));
			}
			return (Node::OBSTACLE);
		}

		/**
         * @brief Retrieves the combined flag values for the tile at a specified 3D absolute position.
         *
         * This is the 3D variant of the flag retrieval method, which internally converts the 3D position
         * to 2D to access the relevant tile data.
         *
         * @param p_absolutePosition The 3D absolute position of the tile within the tilemap.
         * @return An integer representing the combined flag values for the tile.
         */
		int flag(const spk::Vector3Int& p_absolutePosition) const
		{
			return (flag(p_absolutePosition.xy()));
		}
	};
}