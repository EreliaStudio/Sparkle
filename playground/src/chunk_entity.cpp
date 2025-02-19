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

std::string ChunkEntity::_composeFilePath(const spk::Vector2Int& p_chunkPosition)
{
	return (_chunkFolderPath.string() + "chunk_" + std::to_string(p_chunkPosition.x) + "_" + std::to_string(p_chunkPosition.y) + ".chunk");
}

ChunkEntity::ChunkEntity(const spk::Vector2Int& p_chunkPosition, spk::SafePointer<spk::Entity> p_owner) :
	spk::Entity(L"Chunk " + std::to_wstring(p_chunkPosition.x) + L" " + std::to_wstring(p_chunkPosition.y), p_owner),
	_innerComponent(addComponent<InnerComponent>()),
	_chunkPosition(p_chunkPosition)
{
	transform().place(Chunk::convertChunkToWorldPosition(p_chunkPosition));
	load();
	_innerComponent.setChunk(&_chunk);
	_innerComponent.setSpriteSheet(TextureManager::instance()->spriteSheet(L"chunkSpriteSheet"));
}

void ChunkEntity::load()
{
	std::filesystem::path inputFilePath = _composeFilePath(_chunkPosition);
	
	std::ifstream file(inputFilePath, std::ios::binary);

	if (file.is_open() == true)
	{
		_chunk.deserialize(file);
	}
	else
	{
		
	}

	_chunk.invalidate();
}

void ChunkEntity::save()
{
	std::filesystem::path outputFilePath = _composeFilePath(_chunkPosition);
	std::ofstream file(outputFilePath, std::ios::binary);

	if (file.is_open() == false)
	{
		throw std::runtime_error("Can't open file at [" + outputFilePath.string() + "]");
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