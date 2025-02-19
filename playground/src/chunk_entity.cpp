#include "chunk_entity.hpp"

#include "texture_manager.hpp"

ChunkEntity::InnerComponent::InnerComponent() :
	spk::Component(L"InnerComponent")
{

}

void ChunkEntity::InnerComponent::setChunk(spk::SafePointer<BakableChunk> p_chunk)
{
	_chunk = p_chunk;
}

void ChunkEntity::InnerComponent::setSpriteSheet(spk::SafePointer<spk::SpriteSheet> p_spriteSheet)
{
	_renderer.updateSpriteSheet(p_spriteSheet);
}

void ChunkEntity::InnerComponent::start()
{
	_onEditionContract = owner()->transform().addOnEditionCallback([&](){
		_renderer.updateTransform(owner()->transform());
	});
	_onEditionContract.trigger();
}

void ChunkEntity::InnerComponent::onPaintEvent(spk::PaintEvent& p_event)
{
	if (_chunk->needBake() == true)
	{
		_renderer.updateChunk(_chunk);
		_renderer.updateChunkData(_chunk);
		_chunk->validate();
	}

	_renderer.render();
}

ChunkEntity::ChunkEntity(const spk::Vector2Int& p_chunkPosition, spk::SafePointer<spk::Entity> p_owner) :
	spk::Entity(L"Chunk " + std::to_wstring(p_chunkPosition.x) + L" " + std::to_wstring(p_chunkPosition.y), p_owner),
	_innerComponent(addComponent<InnerComponent>()),
	_chunkPosition(p_chunkPosition)
{
	transform().place(Chunk::convertChunkToWorldPosition(p_chunkPosition));
	_innerComponent.setChunk(&_chunk);
	_innerComponent.setSpriteSheet(TextureManager::instance()->spriteSheet(L"chunkSpriteSheet"));
}

void ChunkEntity::load(const std::filesystem::path& p_chunkFilePath)
{
	std::ifstream file(p_chunkFilePath, std::ios::binary);

	if (file.is_open() == true)
	{
		_chunk.deserialize(file);
	}

	_chunk.invalidate();
	file.close();
}

void ChunkEntity::save(const std::filesystem::path& p_chunkFilePath)
{
	std::ofstream file(p_chunkFilePath, std::ios::binary);

	if (file.is_open() == false)
	{
		throw std::runtime_error("Can't open file at [" + p_chunkFilePath.string() + "]");
	}

	_chunk.serialize(file);
	file.close();
}

spk::SafePointer<Chunk> ChunkEntity::chunk()
{
	return (&_chunk);
}

spk::Vector2Int ChunkEntity::chunkPosition()
{
	return (_chunkPosition);
}