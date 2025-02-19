#pragma once

#include "structure/engine/spk_entity.hpp"
#include "structure/engine/spk_component.hpp"

#include "chunk_renderer.hpp"

struct ChunkEntity : public spk::Entity
{
private:
	class InnerComponent : public spk::Component
	{
	private:
		spk::SafePointer<BakableChunk> _chunk;
		ChunkRenderer _renderer;

		spk::Entity::Contract _onEditionContract;

	public:
		InnerComponent();

		void setChunk(spk::SafePointer<BakableChunk> p_chunk);
		void setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet);

		void start() override;

		void onPaintEvent(spk::PaintEvent& p_event) override;
	};

	InnerComponent& _innerComponent;
	spk::Vector2Int _chunkPosition;
	BakableChunk _chunk;

public:
	ChunkEntity(const spk::Vector2Int& p_chunkPosition = spk::Vector2Int(0, 0), spk::SafePointer<spk::Entity> p_owner = nullptr);

	void load(const std::filesystem::path& p_chunkFilePath);
	void save(const std::filesystem::path& p_chunkFilePath);

	spk::SafePointer<Chunk> chunk();
	spk::Vector2Int chunkPosition();
};