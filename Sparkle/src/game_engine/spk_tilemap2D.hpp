#pragma once

#include "game_engine/spk_tilemap.hpp"

namespace spk
{
	class Tilemap2D : public spk::ITilemap<short, 16, 16, 5>
	{
	public:
		struct Node
		{
			spk::Vector2Int sprite = 0;
			bool isAutotiled = false;
			spk::Vector2Int animationFrameOffset = 0;
			int nbFrame = 0;
			int animationDuration = 0;

			Node();
			Node(const spk::Vector2Int& p_sprite, bool p_isAutotiled);
			Node(const spk::Vector2Int& p_sprite, bool p_isAutotiled, const spk::Vector2& p_animationFrameOffset, int p_nbFrame, int p_animationDuration);
		};

		class Chunk : public Tilemap2D::IChunk
		{
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
				//pixelColor = vec4(depth, depth, depth, 1.0f);
				pixelColor = texture(textureID, fragmentUVs);
				if (pixelColor.a == 0)
					discard;
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
					const spk::GameObject* chunkobject = _tilemapCreator->chunkObject(position() + p_chunkOffset);
					_neightbours[relativeChunkPosition.x][relativeChunkPosition.y] = (chunkobject == nullptr ? nullptr : chunkobject->getComponent<Chunk>());
				}
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
									_spriteSheet->sprite(p_node.sprite + _computeSpriteOffset(p_index, i, p_relativePosition)),
									_spriteSheet->unit(),
									static_cast<spk::Vector2>(p_node.animationFrameOffset) * _spriteSheet->unit(), p_node.nbFrame, p_node.animationDuration);
				}
			}

			void _bakeTile(int p_index, const spk::Vector3& p_relativePosition)
			{
				const Node& node = _tilemapCreator->node(p_index);

				if (node.isAutotiled == false)
				{
					_bakeStandardTile(node, p_relativePosition);
				}
				else
				{
					_bakeAutotile(node, p_index, p_relativePosition);
				}
			}

			void _onBaking()
			{
				_vertexData.clear();
				_indexes.clear();

				for (int z = 0; z < SizeZ; z++)
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
			}
			
			void _onObjectRendering()
			{
				_textureObject.attach(_spriteSheet);
				_renderingObject.render();
			}

			void _updateTransform()
			{
				_renderingObjectSelfTransformTranslationElement = owner()->globalPosition();
				_renderingObjectSelfTransformScaleElement = owner()->globalScale();
				_renderingObjectSelfTransformRotationElement = owner()->globalRotation();
				_renderingObjectSelfAttribute.update();
			}

		public:
			Chunk(const Tilemap2D* p_tilemapCreator, const spk::Vector2Int& p_chunkPosition) :
				IChunk(p_chunkPosition),
				_tilemapCreator(p_tilemapCreator),
				_renderingObject(_renderingPipeline.createObject()),
				_textureObject(_renderingPipeline.texture("textureID")),
				_renderingObjectSelfAttribute(_renderingObject.attribute("self")),
				_renderingObjectSelfTransformTranslationElement(_renderingObjectSelfAttribute["transform"]["translation"]),
				_transformTranslationContract(owner()->transform().translation.subscribe([&](){_updateTransform();})),
				_renderingObjectSelfTransformScaleElement(_renderingObjectSelfAttribute["transform"]["scale"]),
				_transformScaleContract(owner()->transform().scale.subscribe([&](){_updateTransform();})),
				_renderingObjectSelfTransformRotationElement(_renderingObjectSelfAttribute["transform"]["rotation"]),
				_transformRotationContract(owner()->transform().rotation.subscribe([&](){_updateTransform();}))
			{
				_updateTransform();

				for (int x = -1; x <= 1; x++)
				{
					for (int y = -1; y <= 1; y++)
					{
						_searchChunkNeighbour(spk::Vector2Int(x, y));
					}
				}

				for (size_t i = 0; i < SizeX; i++)
				{
					for (size_t j = 0; j < SizeY; j++)
					{
						for(size_t k = 0; k < SizeZ; k++)
						{
							setContent(i, j, k, (i == k || i == SizeX - k - 1 || j == k || j == SizeY - k - 1 ? 0 : -1));
						}
					}
				}

				bake();
			}

			void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
			{
				_spriteSheet = p_spriteSheet;
			}
			
			const spk::SpriteSheet* spriteSheet()
			{
				return (_spriteSheet);
			}
		};

	private:
		IChunk* _insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition) override;

		std::vector<spk::GameObject*> _activeChunks;
		
		spk::SpriteSheet* _spriteSheet;
		spk::Vector2Int _activeChunkStart;
		spk::Vector2Int _activeChunkEnd;

		std::map<Chunk::NodeIndexType, Node> _nodes;

	public:
		Tilemap2D(const std::string& p_name);

		void setSpriteSheet(spk::SpriteSheet* p_spriteSheet);

		bool containsNode(const Chunk::NodeIndexType& p_nodeIndex) const;
		void insertNodeType(const Chunk::NodeIndexType& p_nodeIndex, const Node& p_node);
		const Node& node(const Chunk::NodeIndexType& p_nodeIndex) const;

		void setActiveChunkRange(const spk::Vector2Int& p_activeChunkStart, const spk::Vector2Int& p_activeChunkEnd);
		void updateActiveChunks();
	};
}