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

	static inline const std::filesystem::path _chunkFolderPath = "playground/resources/chunks/";

	InnerComponent& _innerComponent;
	spk::Vector2Int _chunkPosition;
	BakableChunk _chunk;

	std::string _composeFilePath(const spk::Vector2Int& p_chunkPosition);

public:
	ChunkEntity(const spk::Vector2Int& p_chunkPosition = spk::Vector2Int(0, 0), spk::SafePointer<spk::Entity> p_owner = nullptr);

	void load();
	void save();

	spk::SafePointer<Chunk> chunk();
	spk::Vector2Int chunkPosition();
};