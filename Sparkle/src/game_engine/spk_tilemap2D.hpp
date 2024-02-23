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
			const Tilemap2D* _origin = nullptr;
			spk::Vector2Int _position;
			const Chunk* _neightbours[3][3] = {
				{nullptr, nullptr, nullptr},
				{nullptr, nullptr, nullptr},
				{nullptr, nullptr, nullptr}
			};

			const Chunk* _requestNeightbour(const spk::Vector2Int& p_neightbourOffset);

			void _insertNodeData(
				std::vector<VertexData>& p_vertexData,
				std::vector<unsigned int>& p_indexes,
				const spk::Vector3& p_position,
				const spk::Vector3& p_size,
				const spk::Vector2& p_spritePosition, 
				const spk::Vector2& p_spriteSize,
				const spk::Vector2& p_animationOffset,
				int p_animationLenght,
				int p_animationDuration);

			void _insertSimpleNode(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const Node& p_node, int p_x, int p_y, int p_z);
			const Chunk* _getNeightbourChunk(spk::Vector3Int& p_relativePosition);
			spk::Vector2Int _computeSpriteOffset(int p_baseIndex, int p_quarterIndex, int p_x, int p_y, int p_z);
			void _insertAutotiledNode(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const NodeIndexType& p_index, const Node& p_node, int p_x, int p_y, int p_z);
			void _bakeTile(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes, const NodeIndexType& p_index, int p_x, int p_y, int p_z);
			void _bake(std::vector<VertexData>& p_vertexData, std::vector<unsigned int>& p_indexes) override;

		public:
			Chunk(Tilemap2D* p_origin, const spk::Vector2Int& p_chunkPosition);
		};

	private:
		IChunk* _insertChunk(spk::GameObject* p_object, const spk::Vector2Int& p_chunkPosition) override;

		std::vector<spk::GameObject*> _activeChunks;

		void _onUpdate();
		void _onRender();
		
		spk::SpriteSheet* _spriteSheet;
		spk::Vector2Int _start;
		spk::Vector2Int _end;
		std::map<typename IChunk::NodeIndexType, Node> _nodes;

	public:
		Tilemap2D(const std::string& p_name);

		void setSpriteSheet(spk::SpriteSheet* p_spriteSheet);

		bool containsNode(const typename IChunk::NodeIndexType& p_nodeIndex) const;
		void insertNodeType(const typename IChunk::NodeIndexType& p_nodeIndex, const Node& p_node);
		const Node& node(const typename IChunk::NodeIndexType& p_nodeIndex) const;

		void setActiveChunkRange(const spk::Vector2Int& p_start, const spk::Vector2Int& p_end);
		void updateActiveChunks();
	};
}