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
		bool isEven = (_chunkPosition.x % 2 == 0 && _chunkPosition.y % 2 == 0) || (_chunkPosition.x % 2 != 0 && _chunkPosition.y % 2 != 0);
		for (size_t i = 0; i < Chunk::Size; i++)
		{
			for (size_t j = 0; j < Chunk::Size; j++)
			{
				_chunk.setContent(spk::Vector3Int(i, j, 0), (isEven == true ? 0 : 1));

				if (i == spk::positiveModulo(_chunkPosition.x, Chunk::Size) && j == spk::positiveModulo(_chunkPosition.y, Chunk::Size))
				{
					_chunk.setContent(spk::Vector3Int(i, j, 0), 2);
				}
			}
		}

		if (_chunkPosition == spk::Vector2Int(1, 1))
		{
			_chunk.setContent(spk::Vector3Int(3, 3, 1), 3);
			_chunk.setContent(spk::Vector3Int(6, 3, 1), 3);
			_chunk.setContent(spk::Vector3Int(3, 6, 1), 3);
			_chunk.setContent(spk::Vector3Int(6, 6, 1), 3);
		}
	}

	_chunk.invalidate();
}

void ChunkEntity::save()
{
	std::filesystem::path outputFilePath = _composeFilePath(_chunkPosition);
	std::ofstream file(outputFilePath, std::ios::binary);
	_chunk.serialize(file);
}

spk::SafePointer<Chunk> ChunkEntity::chunk()
{
	return (&_chunk);
}

spk::Vector2Int ChunkEntity::chunkPosition()
{
	return (_chunkPosition);
}