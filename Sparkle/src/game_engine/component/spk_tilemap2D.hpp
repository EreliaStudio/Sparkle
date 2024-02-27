#pragma once

#include "game_engine/spk_mesh.hpp"
#include "game_engine/component/spk_tilemap.hpp"

namespace spk
{
	class Tilemap2D : public spk::ITilemap<short, 16, 16, 5>
	{
	public:
		struct Node
		{
			static inline const uint16_t WALKABLE = 		0b0000000000000000;
			static inline const uint16_t OBSTACLE = 		0b0000000000000001;

			spk::Vector2Int sprite = 0;
			bool isAutotiled = false;
			spk::Vector2Int animationFrameOffset = 0;
			int nbFrame = 0;
			int animationDuration = 0;
			int flags = WALKABLE;

			Node();
			Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, bool p_isAutotiled);
			Node(const spk::Vector2Int& p_sprite, uint16_t p_flags, bool p_isAutotiled, const spk::Vector2& p_animationFrameOffset, int p_nbFrame, int p_animationDuration);
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

				VertexData();
				VertexData( const spk::Vector3& p_modelVertex,
							const spk::Vector2& p_modelUVs,
							const spk::Vector2& p_modelAnimationOffset,
							int p_modelAnimationSize,
							int p_modelAnimationDuration );
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

			const Chunk* _getChunkNeighbour(const spk::Vector2Int& p_chunkOffset);
			void _searchChunkNeighbour(const spk::Vector2Int& p_chunkOffset);
			const Tilemap2D::Chunk* _getNeightbourChunk(spk::Vector3Int& p_relativePosition);

			void _insertNodeData(
				const spk::Vector3& p_position,
				const spk::Vector3& p_size,
				const spk::Vector2& p_spritePosition,
				const spk::Vector2& p_spriteSize,
				const spk::Vector2& p_animationOffset,
				int p_animationLenght,
				int p_animationDuration);

			void _bakeStandardTile(const Node& p_node, const spk::Vector3& p_relativePosition);


			spk::Vector2Int _computeSpriteOffset(int p_baseIndex, int p_quarterIndex, const spk::Vector3& p_relativePosition);
			void _bakeAutotile(const Node& p_node, int p_index, const spk::Vector3& p_relativePosition);

			void _bakeTile(int p_index, const spk::Vector3& p_relativePosition);

			void _onBaking();

			bool _isValidSquare(bool (&p_used)[SizeX][SizeY], const spk::Vector2Int& p_start, const spk::Vector2Int& p_end);
			spk::Vector2Int _searchEndPosition(bool (&p_used)[SizeX][SizeY], const spk::Vector2Int& p_start);
			void _createCollisionMesh();
			void _onObjectRendering();

			void _updateTransform();

		public:
			Chunk(const Tilemap2D* p_tilemapCreator, const spk::Vector2Int& p_chunkPosition);

			void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet);
			const spk::SpriteSheet* spriteSheet();

			const spk::Mesh& collisionMesh() const;

			bool isObstacle(const spk::Vector2Int& p_position);
		};

	private:
		IChunk* _insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition) override;

		std::vector<spk::GameObject*> _activeChunks;
		
		const spk::SpriteSheet* _spriteSheet;
		spk::Vector2Int _activeChunkStart;
		spk::Vector2Int _activeChunkEnd;

		std::map<Chunk::NodeIndexType, Node> _nodes;

	public:
		Tilemap2D(const std::string& p_name);

		void setSpriteSheet(const spk::SpriteSheet* p_spriteSheet);

		bool containsNode(const Chunk::NodeIndexType& p_nodeIndex) const;
		void insertNodeType(const Chunk::NodeIndexType& p_nodeIndex, const Node& p_node);
		const Node& node(const Chunk::NodeIndexType& p_nodeIndex) const;

		void setActiveChunkRange(const spk::Vector2Int& p_activeChunkStart, const spk::Vector2Int& p_activeChunkEnd);
		void updateActiveChunks();
		std::vector<spk::Vector2Int> missingChunks() const;
	};
}